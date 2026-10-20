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

struct mtk_softc {
    device_t sc_dev;
    bus_space_tag_t sc_regt;
    bus_space_handle_t sc_regh;
    bus_addr_t sc_reg_pa;

    pci_intr_handle_t *sc_pihp;
    pci_chipset_tag_t sc_pct;
    void *sc_ih;
};

static int
mtk_match(device_t parent, cfdata_t match, void *aux);

static void
mtk_attach(device_t parent, device_t self, void *aux);

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

// interrupt handler
static int
mtk_intr(void *arg)
{
	//struct mtk_softc *sc = arg;

	return 1;
}

static void
mtk_attach(device_t parent, device_t self, void *aux){
    printf("mtk: hey hey hey...\n");
    struct mtk_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
    pcireg_t reg;
    pci_chipset_tag_t pc = pa->pa_pc;
    pcitag_t tag = pa->pa_tag;
    sc->sc_dev = self;
    int error;
    const char *intrstr;
	char intrbuf[PCI_INTRSTR_LEN];
    
    printf("below should be another mtk message from pci_aprint_devinfo\n");
    pci_aprint_devinfo(pa, NULL);
    // let's do the thing the linux driver does reading from pci mem
    // pci_read_config_word(pdev, PCI_COMMAND, &cmd);
	// if (!(cmd & PCI_COMMAND_MEMORY)) {
	// 	cmd |= PCI_COMMAND_MEMORY;
	// 	pci_write_config_word(pdev, PCI_COMMAND, cmd);
	// }
    // netbsd equivalent using pci_conf_read...
	reg = pci_conf_read(pc, tag, PCI_COMMAND);
    // but I might need to mask this or something...
    printf("mtk: reg value: 0x%ux\n", (uint32_t)reg);
    // map BAR0 (physical address) to kernel virtual addr
    if (pci_mapreg_map(pa, PCI_BAR0, PCI_MAPREG_TYPE_MEM, 0, 
            &sc->sc_regt, &sc->sc_regh, &sc->sc_reg_pa, 0) != 0 ) {
            aprint_error_dev(sc->sc_dev, "mtk: can't map the BAR\n");
            return;
    }

    printf("below should be another mtk message from aprint_normal_dev\n");
    aprint_normal_dev(sc->sc_dev, "mtk: regs at 0x%08x\n", (uint32_t) sc->sc_reg_pa);

    // alloc irq vectors
    error = pci_intr_alloc(pa, &sc->sc_pihp, NULL, 0);
	if (error) {
		aprint_error_dev(self, "can't allocate interrupt\n");
		return;
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

// looks familiar, iwn?
failia:	pci_intr_release(sc->sc_pct, sc->sc_pihp, 1);
	sc->sc_pihp = NULL;
}


CFATTACH_DECL_NEW(mtk, sizeof(struct mtk_softc), mtk_match, mtk_attach, 
    NULL, NULL);