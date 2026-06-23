#include "pci.h"
#include "ports.h"
#include "screen.h"
#include "util.h"

#define MAX_PCI_DEVICES 64
static pci_device_t pci_devices[MAX_PCI_DEVICES];
static int pci_device_count = 0;

unsigned int pci_read_dword(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address;
    unsigned int lbus  = (unsigned int)bus;
    unsigned int lslot = (unsigned int)slot;
    unsigned int lfunc = (unsigned int)func;
    
    address = (unsigned int)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((unsigned int)0x80000000));
              
    port_dword_out(0xCF8, address);
    return port_dword_in(0xCFC);
}

void pci_write_dword(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset, unsigned int data) {
    unsigned int address;
    unsigned int lbus  = (unsigned int)bus;
    unsigned int lslot = (unsigned int)slot;
    unsigned int lfunc = (unsigned int)func;
    
    address = (unsigned int)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((unsigned int)0x80000000));
              
    port_dword_out(0xCF8, address);
    port_dword_out(0xCFC, data);
}

const char* pci_class_to_string(unsigned char class_code) {
    switch (class_code) {
        case 0x00: return "Unclassified";
        case 0x01: return "Mass Storage Controller";
        case 0x02: return "Network Controller";
        case 0x03: return "Display Controller";
        case 0x04: return "Multimedia Controller";
        case 0x05: return "Memory Controller";
        case 0x06: return "Bridge Device";
        case 0x07: return "Simple Communication Controller";
        case 0x08: return "Base System Peripheral";
        case 0x09: return "Input Device Controller";
        case 0x0A: return "Docking Station";
        case 0x0B: return "Processor";
        case 0x0C: return "Serial Bus Controller";
        case 0x0D: return "Wireless Controller";
        case 0x0E: return "Intelligent Controller";
        case 0x0F: return "Satellite Communication Controller";
        case 0x10: return "Encryption Controller";
        case 0x11: return "Signal Processing Controller";
        default:   return "Unknown Device Class";
    }
}

static void pci_add_device(unsigned char bus, unsigned char slot, unsigned char func,
                           unsigned short vendor_id, unsigned short device_id,
                           unsigned char class_code, unsigned char subclass, unsigned char prog_if,
                           unsigned char header_type, unsigned int bar0, unsigned int bar1,
                           unsigned int bar2, unsigned int bar3, unsigned int bar4, unsigned int bar5) {
    if (pci_device_count >= MAX_PCI_DEVICES) return;
    
    pci_devices[pci_device_count].bus = bus;
    pci_devices[pci_device_count].slot = slot;
    pci_devices[pci_device_count].func = func;
    pci_devices[pci_device_count].vendor_id = vendor_id;
    pci_devices[pci_device_count].device_id = device_id;
    pci_devices[pci_device_count].class_code = class_code;
    pci_devices[pci_device_count].subclass = subclass;
    pci_devices[pci_device_count].prog_if = prog_if;
    pci_devices[pci_device_count].header_type = header_type;
    pci_devices[pci_device_count].bar0 = bar0;
    pci_devices[pci_device_count].bar1 = bar1;
    pci_devices[pci_device_count].bar2 = bar2;
    pci_devices[pci_device_count].bar3 = bar3;
    pci_devices[pci_device_count].bar4 = bar4;
    pci_devices[pci_device_count].bar5 = bar5;
    
    pci_device_count++;
}

void init_pci() {
    pci_device_count = 0;
    
    for (int bus = 0; bus < 256; bus++) {
        for (int slot = 0; slot < 32; slot++) {
            unsigned int reg0 = pci_read_dword(bus, slot, 0, 0x00);
            unsigned short vendor_id = reg0 & 0xFFFF;
            if (vendor_id == 0xFFFF) continue;
            
            unsigned int reg3 = pci_read_dword(bus, slot, 0, 0x0C);
            unsigned char header_type = (reg3 >> 16) & 0xFF;
            int num_funcs = (header_type & 0x80) ? 8 : 1;
            
            for (int func = 0; func < num_funcs; func++) {
                unsigned int r0 = pci_read_dword(bus, slot, func, 0x00);
                unsigned short vend = r0 & 0xFFFF;
                if (vend == 0xFFFF) continue;
                
                unsigned short dev = (r0 >> 16) & 0xFFFF;
                
                unsigned int r2 = pci_read_dword(bus, slot, func, 0x08);
                unsigned char class_code = (r2 >> 24) & 0xFF;
                unsigned char subclass   = (r2 >> 16) & 0xFF;
                unsigned char prog_if    = (r2 >> 8) & 0xFF;
                
                unsigned int bar0 = pci_read_dword(bus, slot, func, 0x10);
                unsigned int bar1 = pci_read_dword(bus, slot, func, 0x14);
                unsigned int bar2 = pci_read_dword(bus, slot, func, 0x18);
                unsigned int bar3 = pci_read_dword(bus, slot, func, 0x1C);
                unsigned int bar4 = pci_read_dword(bus, slot, func, 0x20);
                unsigned int bar5 = pci_read_dword(bus, slot, func, 0x24);
                
                pci_add_device(bus, slot, func, vend, dev, class_code, subclass, prog_if, header_type, bar0, bar1, bar2, bar3, bar4, bar5);
            }
        }
    }
}

int pci_get_device_count() {
    return pci_device_count;
}

pci_device_t* pci_get_device(int index) {
    if (index < 0 || index >= pci_device_count) return 0;
    return &pci_devices[index];
}