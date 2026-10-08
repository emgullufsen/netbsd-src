#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

static int
mtk_match(device_t parent, cfdata_t match, void *aux);

static void
mtk_attach(device_t parent, device_t self, void *aux);

static int
mtk_detach(device_t self, int flags);

static int
mtk_activate(device_t self, enum devact act);

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
    struct iwm_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
}


CFATTACH_DECL_NEW(mtk, sizeof(struct mtk_softc), mtk_match, mtk_attach, 
    mtk_detach, mtk_activate)