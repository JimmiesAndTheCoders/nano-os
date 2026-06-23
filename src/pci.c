#include "pci.h"
#include "ports.h"
#include "screen.h"
#include "util.h"

#define MAX_PCI_DEVICES 64
static pci_device_t pci_devices[MAX_PCI_DEVICES];
static int pci_device_count = 0;

static inline unsigned long long rdmsr(unsigned int msr) {
    unsigned int low, high;
    __asm__ __volatile__("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((unsigned long long)high << 32) | low;
}

static inline void wrmsr(unsigned int msr, unsigned long long val) {
    unsigned int low = val & 0xFFFFFFFF;
    unsigned int high = val >> 32;
    __asm__ __volatile__("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

static void map_bar_mmio(unsigned int physical_addr) {
    unsigned int pd_index = physical_addr >> 22;
    unsigned int* page_directory = (unsigned int*)0x9000;
    
    // Map using 4MB Page Size Extension (PSE) if not already mapped
    if ((page_directory[pd_index] & 0x1) == 0) {
        page_directory[pd_index] = (physical_addr & 0xFFC00000) | 0x87; // Present + R/W + User + 4MB
        
        // Flush TLB
        __asm__ __volatile__(
            "mov %%cr3, %%eax\n\t"
            "mov %%eax, %%cr3"
            : : : "eax"
        );
    }
}

void init_lapic() {
    map_bar_mmio(0xFEE00000);
    
    unsigned long long apic_base_msr = rdmsr(0x1B);
    apic_base_msr |= 0x800; // Enable Global LAPIC flag (Bit 11)
    wrmsr(0x1B, apic_base_msr);
    
    volatile unsigned int* spurious_reg = (volatile unsigned int*)(0xFEE000F0);
    *spurious_reg = 0x1FF; // Enable LAPIC (Bit 8) + Spurious Vector 0xFF
}

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

unsigned char pci_find_capability(unsigned char bus, unsigned char slot, unsigned char func, unsigned char cap_id) {
    unsigned int reg1 = pci_read_dword(bus, slot, func, 0x04);
    unsigned short status = (reg1 >> 16) & 0xFFFF;
    
    if (!(status & (1 << 4))) {
        return 0; // Capabilities List bit not set
    }
    
    unsigned int reg13 = pci_read_dword(bus, slot, func, 0x34);
    unsigned char cap_ptr = reg13 & 0xFF;
    
    while (cap_ptr != 0 && cap_ptr < 256) {
        unsigned int cap_data = pci_read_dword(bus, slot, func, cap_ptr);
        unsigned char current_cap_id = cap_data & 0xFF;
        if (current_cap_id == cap_id) {
            return cap_ptr;
        }
        cap_ptr = (cap_data >> 8) & 0xFF;
    }
    
    return 0;
}

int pci_enable_msi(pci_device_t* dev, unsigned char vector) {
    unsigned char cap_ptr = pci_find_capability(dev->bus, dev->slot, dev->func, 0x05);
    if (cap_ptr == 0) return 0;
    
    unsigned int ctrl_reg = pci_read_dword(dev->bus, dev->slot, dev->func, cap_ptr);
    unsigned short msg_ctrl = (ctrl_reg >> 16) & 0xFFFF;
    
    int is_64bit = (msg_ctrl & (1 << 7)) ? 1 : 0;
    unsigned int msg_addr = 0xFEE00000;
    unsigned int msg_data = 0x4000 | vector; // Edge, active high, fixed delivery to vector
    
    pci_write_dword(dev->bus, dev->slot, dev->func, cap_ptr + 4, msg_addr);
    
    if (is_64bit) {
        pci_write_dword(dev->bus, dev->slot, dev->func, cap_ptr + 8, 0);
        unsigned int data_reg = pci_read_dword(dev->bus, dev->slot, dev->func, cap_ptr + 12);
        data_reg = (data_reg & 0xFFFF0000) | (msg_data & 0xFFFF);
        pci_write_dword(dev->bus, dev->slot, dev->func, cap_ptr + 12, data_reg);
    } else {
        unsigned int data_reg = pci_read_dword(dev->bus, dev->slot, dev->func, cap_ptr + 8);
        data_reg = (data_reg & 0xFFFF0000) | (msg_data & 0xFFFF);
        pci_write_dword(dev->bus, dev->slot, dev->func, cap_ptr + 8, data_reg);
    }
    
    msg_ctrl |= (1 << 0);   // Set enable bit
    msg_ctrl &= ~(7 << 4);  // Configure single vector
    
    ctrl_reg = (ctrl_reg & 0x0000FFFF) | ((unsigned int)msg_ctrl << 16);
    pci_write_dword(dev->bus, dev->slot, dev->func, cap_ptr, ctrl_reg);
    
    dev->msi_enabled = 1;
    dev->msi_vector = vector;
    return 1;
}

int pci_disable_msi(pci_device_t* dev) {
    unsigned char cap_ptr = pci_find_capability(dev->bus, dev->slot, dev->func, 0x05);
    if (cap_ptr == 0) return 0;
    
    unsigned int ctrl_reg = pci_read_dword(dev->bus, dev->slot, dev->func, cap_ptr);
    unsigned short msg_ctrl = (ctrl_reg >> 16) & 0xFFFF;
    
    msg_ctrl &= ~(1 << 0); // Clear enable bit
    
    ctrl_reg = (ctrl_reg & 0x0000FFFF) | ((unsigned int)msg_ctrl << 16);
    pci_write_dword(dev->bus, dev->slot, dev->func, cap_ptr, ctrl_reg);
    
    dev->msi_enabled = 0;
    dev->msi_vector = 0;
    return 1;
}

int pci_enable_msix(pci_device_t* dev, unsigned char vector, unsigned int index) {
    unsigned char cap_ptr = pci_find_capability(dev->bus, dev->slot, dev->func, 0x11);
    if (cap_ptr == 0) return 0;
    
    unsigned int table_offset_bir = pci_read_dword(dev->bus, dev->slot, dev->func, cap_ptr + 4);
    unsigned char bir = table_offset_bir & 0x7;
    unsigned int table_offset = table_offset_bir & 0xFFFFFFF8;
    
    unsigned int bar_value = 0;
    if (bir == 0) bar_value = dev->bar0;
    else if (bir == 1) bar_value = dev->bar1;
    else if (bir == 2) bar_value = dev->bar2;
    else if (bir == 3) bar_value = dev->bar3;
    else if (bir == 4) bar_value = dev->bar4;
    else if (bir == 5) bar_value = dev->bar5;
    
    if (bar_value == 0 || (bar_value & 0x1) != 0) {
        return 0; // Address must map to MMIO register space
    }
    
    unsigned int bar_phys = bar_value & 0xFFFFFFF0;
    map_bar_mmio(bar_phys);
    
    unsigned int table_addr = bar_phys + table_offset;
    volatile unsigned int* entry_ptr = (volatile unsigned int*)(table_addr + (index * 16));
    
    unsigned int msg_addr = 0xFEE00000;
    unsigned int msg_data = 0x4000 | vector;
    
    entry_ptr[0] = msg_addr;
    entry_ptr[1] = 0;
    entry_ptr[2] = msg_data;
    entry_ptr[3] = entry_ptr[3] & ~1; // Clear mask bit (Bit 0)
    
    unsigned int ctrl_reg = pci_read_dword(dev->bus, dev->slot, dev->func, cap_ptr);
    unsigned short msg_ctrl = (ctrl_reg >> 16) & 0xFFFF;
    
    msg_ctrl |= (1 << 15);  // Enable MSI-X
    msg_ctrl &= ~(1 << 14); // Clear global function mask
    
    ctrl_reg = (ctrl_reg & 0x0000FFFF) | ((unsigned int)msg_ctrl << 16);
    pci_write_dword(dev->bus, dev->slot, dev->func, cap_ptr, ctrl_reg);
    
    dev->msix_enabled = 1;
    dev->msix_vector = vector;
    return 1;
}

int pci_disable_msix(pci_device_t* dev) {
    unsigned char cap_ptr = pci_find_capability(dev->bus, dev->slot, dev->func, 0x11);
    if (cap_ptr == 0) return 0;
    
    unsigned int ctrl_reg = pci_read_dword(dev->bus, dev->slot, dev->func, cap_ptr);
    unsigned short msg_ctrl = (ctrl_reg >> 16) & 0xFFFF;
    
    msg_ctrl &= ~(1 << 15); // Clear enable bit
    
    ctrl_reg = (ctrl_reg & 0x0000FFFF) | ((unsigned int)msg_ctrl << 16);
    pci_write_dword(dev->bus, dev->slot, dev->func, cap_ptr, ctrl_reg);
    
    dev->msix_enabled = 0;
    dev->msix_vector = 0;
    return 1;
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
    
    pci_devices[pci_device_count].msi_supported = (pci_find_capability(bus, slot, func, 0x05) != 0);
    pci_devices[pci_device_count].msix_supported = (pci_find_capability(bus, slot, func, 0x11) != 0);
    pci_devices[pci_device_count].msi_enabled = 0;
    pci_devices[pci_device_count].msix_enabled = 0;
    pci_devices[pci_device_count].msi_vector = 0;
    pci_devices[pci_device_count].msix_vector = 0;
    
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