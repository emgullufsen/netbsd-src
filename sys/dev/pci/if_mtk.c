#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/mbuf.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#include <sys/cpu.h>
#include <sys/bus.h>
#include <sys/workqueue.h>
#include <machine/endian.h>
#include <sys/intr.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/firmload.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <netinet/in.h>
#include <netinet/ip.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_amrr.h>
#include <net80211/ieee80211_radiotap.h>

// #include <sys/param.h>
// #include <sys/device.h>
// #include <sys/errno.h>
// #include <dev/pci/pcireg.h>
// #include <dev/pci/pcivar.h>
// #include <dev/pci/pcidevs.h>

// I am sure I should use another define from higher up under pci/
#define MTK_MMREG_BAR   0x10
#define PCI_COMMAND		0x04
#define PCI_COMMAND_MEMORY  0x2
#define OFFSET_DIS 0x204

struct mtk_softc {
    device_t sc_dev;
    bus_space_tag_t sc_regt;
    bus_space_handle_t sc_regh;
    bus_addr_t sc_reg_pa;
    bus_size_t sc_sz;

    pci_intr_handle_t *sc_pihp;
    pci_chipset_tag_t sc_pct;
    void *sc_ih;
    void *sc_soft_ih;

    struct ieee80211com sc_ic;
};

void
printf_uint32_binary(uint32_t val);

void
printf_uint16_binary(uint16_t val);

static int
mtk_match(device_t parent, cfdata_t match, void *aux);

static void
mtk_attach(device_t parent, device_t self, void *aux);

static void	mtk_softintr(void *);

// static int
// mtk_detach(device_t self, int flags);

// static int
// mtk_activate(device_t self, enum devact act);

static int
mtk_match(device_t parent, cfdata_t match, void *aux)
{
    struct pci_attach_args *pa = aux;

    if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_MEDIATEK)
        return 0;

    if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_MEDIATEK_MT7921)
        return 1;

    return 0;
}

static void
mtk_softintr(void *arg){
    return;
}

// interrupt handler
static int
mtk_intr(void *arg)
{
	struct mtk_softc *sc = arg;

	/* Disable interrupts. */
	/* 0xd4000 + 0x204 */
    /* I'm pretty sure 0x204 is the offset*/
    /*
    void
     bus_space_write_4(bus_space_tag_t space, bus_space_handle_t handle,
         bus_size_t offset, uint32_t value);
    */
    bus_space_write_4(sc->sc_regt, sc->sc_regh, OFFSET_DIS, 0);

    softint_schedule(sc->sc_soft_ih);
	return 1;
}

void
printf_uint32_binary(uint32_t val)
{
    char buf[33];
    int i;

    for (i = 0; i < 32; i++) {
        buf[i] = (val & __BIT(31 - i)) ? '1' : '0';
    }
    buf[32] = '\0';

    printf("Value in binary: %s\n", buf);
}

void
printf_uint16_binary(uint16_t val)
{
    char buf[17];
    int i;

    for (i = 0; i < 16; i++) {
        buf[i] = (val & __BIT(15 - i)) ? '1' : '0';
    }
    buf[16] = '\0';

    printf("Value in binary: %s\n", buf);
}

static void
mtk_attach(device_t parent, device_t self, void *aux){
    printf("mtk: hey hey hey...\n");
    struct mtk_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
    /*struct ieee80211com *ic = &sc->sc_ic;*/
    pcireg_t reg;
    pci_chipset_tag_t pc = pa->pa_pc;
    pcitag_t tag = pa->pa_tag;
    sc->sc_dev = self;
    int error;
    const char *intrstr;
	char intrbuf[PCI_INTRSTR_LEN];
    
    pci_aprint_devinfo(pa, NULL);

	reg = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
    // we need just the lower 16 bits for the command reg (upper is status)
    uint16_t cmd = reg & 0xffff;
    
    printf("mtk: reg value: 0x%ux\n", (uint32_t)reg);
    printf("mtk: printing reg value in binary now...\n");
    printf_uint32_binary((uint32_t)reg);
    printf("mtk: printing cmd (lower 16 of reg now)...\n");
    printf_uint16_binary((uint16_t)cmd);
    printf("mtk: by the way, PCI_COMMAND_MEMORY in binary is...\n");
    printf_uint16_binary((uint16_t)PCI_COMMAND_MEMORY);

    if (!(cmd & PCI_COMMAND_MEMORY)) {
        printf("mtk: ok we need to write back cmd...\n");
        cmd |= PCI_COMMAND_MEMORY;
        reg = (reg & 0xffff0000) | cmd;
        pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, reg);
	}

    //ok now lets read back and check...
    printf("mtk: ok now lets read back and check...\n");
    reg = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
    // we need just the lower 16 bits for the command reg (upper is status)
    cmd = reg & 0xffff;
    printf("mtk: re-printing reg value in binary now...\n");
    printf_uint32_binary((uint32_t)reg);
    printf("mtk: re-printing cmd (lower 16 of reg now)...\n");
    printf_uint16_binary((uint16_t)cmd);

    // map BAR0 (physical address) to kernel virtual addr
    if (pci_mapreg_map(pa, PCI_BAR0, PCI_MAPREG_TYPE_MEM, 0, 
            &sc->sc_regt, &sc->sc_regh, &sc->sc_reg_pa, &sc->sc_sz) != 0 ) {
            aprint_error_dev(sc->sc_dev, "mtk: can't map the BAR\n");
            return;
    }

    printf("below should be another mtk message from aprint_normal_dev\n");
    aprint_normal_dev(sc->sc_dev, "mtk: regs at 0x%08x\n", (uint32_t) sc->sc_reg_pa);

    // establish bottom half
    sc->sc_soft_ih = softint_establish(SOFTINT_NET, mtk_softintr, sc);
	if (sc->sc_soft_ih == NULL) {
		aprint_error_dev(self, "could not establish softint\n");
		goto unmap;
	}

    // alloc irq vectors
    error = pci_intr_alloc(pa, &sc->sc_pihp, NULL, 0);
	if (error) {
		aprint_error_dev(self, "can't allocate interrupt\n");
		goto failsi;
	}
    
    intrstr = pci_intr_string(sc->sc_pct, sc->sc_pihp[0], intrbuf,
	    sizeof(intrbuf));
	sc->sc_ih = pci_intr_establish_xname(sc->sc_pct, sc->sc_pihp[0],
	    IPL_NET, mtk_intr, sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "can't establish interrupt");
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		goto failia;
	}

	aprint_normal_dev(self, "interrupting at %s\n", intrstr);
    return;

failia:	pci_intr_release(sc->sc_pct, sc->sc_pihp, 1);
	sc->sc_pihp = NULL;
failsi:	softint_disestablish(sc->sc_soft_ih);
	sc->sc_soft_ih = NULL;
unmap:	bus_space_unmap(sc->sc_regt, sc->sc_regh, sc->sc_sz);
}


CFATTACH_DECL_NEW(mtk, sizeof(struct mtk_softc), mtk_match, mtk_attach, 
    NULL, NULL);