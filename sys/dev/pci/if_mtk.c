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

struct mtk_softc {
                   device_t sc_dev;                /* generic device info */
                   /* device-specific state */
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

static void
mtk_attach(device_t parent, device_t self, void *aux){
    //struct mtk_softc *sc = device_private(self);
	//struct pci_attach_args *pa = aux;
    printf("hey hey hey it's mtk...\n");
}


CFATTACH_DECL_NEW(mtk, sizeof(struct mtk_softc), mtk_match, mtk_attach, 
    NULL, NULL);