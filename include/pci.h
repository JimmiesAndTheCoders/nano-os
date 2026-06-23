#ifndef PCI_H
#define PCI_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned short vendor_id;
    unsigned short device_id;
    unsigned char bus;
    unsigned char slot;
    unsigned char func;
    unsigned char class_code;
    unsigned char subclass;
    unsigned char prog_if;
    unsigned char header_type;
    unsigned int bar0;
    unsigned int bar1;
    unsigned int bar2;
    unsigned int bar3;
    unsigned int bar4;
    unsigned int bar5;
    
    // MSI & MSI-X Capabilities
    unsigned char msi_supported;
    unsigned char msix_supported;
    unsigned char msi_enabled;
    unsigned char msix_enabled;
    unsigned char msi_vector;
    unsigned char msix_vector;
} pci_device_t;

void init_pci();
int pci_get_device_count();
pci_device_t* pci_get_device(int index);
const char* pci_class_to_string(unsigned char class_code);

unsigned int pci_read_dword(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset);
void pci_write_dword(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset, unsigned int data);

unsigned char pci_find_capability(unsigned char bus, unsigned char slot, unsigned char func, unsigned char cap_id);
int pci_enable_msi(pci_device_t* dev, unsigned char vector);
int pci_disable_msi(pci_device_t* dev);
int pci_enable_msix(pci_device_t* dev, unsigned char vector, unsigned int index);
int pci_disable_msix(pci_device_t* dev);

void init_lapic();

#ifdef __cplusplus
}
#endif

#endif