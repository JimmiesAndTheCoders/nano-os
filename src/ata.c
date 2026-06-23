#include "ata.h"
#include "ports.h"
#include "cpu.h"
#include "pci.h"
#include "util.h"
#include "screen.h"

#define ATA_REG_DATA            0x0
#define ATA_REG_ERROR           0x1
#define ATA_REG_FEATURES        0x1
#define ATA_REG_SEC_COUNT       0x2
#define ATA_REG_LBA_LO          0x3
#define ATA_REG_LBA_MID         0x4
#define ATA_REG_LBA_HI          0x5
#define ATA_REG_DRIVE           0x6
#define ATA_REG_STATUS          0x7
#define ATA_REG_COMMAND         0x7

#define ATA_CMD_READ_PIO        0x20
#define ATA_CMD_WRITE_PIO       0x30
#define ATA_CMD_IDENTIFY        0xEC
#define ATA_CMD_READ_DMA        0xC8
#define ATA_CMD_WRITE_DMA       0xCA

#define ATA_STATUS_ERR          0x01
#define ATA_STATUS_DRQ          0x08
#define ATA_STATUS_SRV          0x10
#define ATA_STATUS_DF           0x20
#define ATA_STATUS_RDY          0x40
#define ATA_STATUS_BSY          0x80

#define PRD_EOT                 0x8000

typedef struct {
    unsigned int phys_addr;
    unsigned short byte_count;
    unsigned short reserved_eot;
} __attribute__((packed)) prd_t;

static volatile int ata_irq_fired = 0;
static unsigned int bmide_io_base = 0;
static int has_dma_support = 0;

#define DMA_BOUNCE_SIZE (32 * 1024)
__attribute__((aligned(4096))) static unsigned char dma_bounce_buffer[DMA_BOUNCE_SIZE];
__attribute__((aligned(4))) static prd_t prd_table[1];

static void ata_io_wait(unsigned short control_base) {
    port_byte_in(control_base);
    port_byte_in(control_base);
    port_byte_in(control_base);
    port_byte_in(control_base);
}

static unsigned int ata_primary_callback(registers_t *regs) {
    ata_irq_fired = 1;
    port_byte_in(0x1F7);
    return (unsigned int)regs;
}

static unsigned int ata_secondary_callback(registers_t *regs) {
    ata_irq_fired = 1;
    port_byte_in(0x177);
    return (unsigned int)regs;
}

void init_ata() {
    int count = pci_get_device_count();
    for (int i = 0; i < count; i++) {
        pci_device_t* dev = pci_get_device(i);
        if (dev->class_code == 0x01 && dev->subclass == 0x01) {
            if (dev->bar4 & 1) {
                bmide_io_base = dev->bar4 & ~1;
                has_dma_support = 1;
                break;
            }
        }
    }
    
    register_interrupt_handler(46, ata_primary_callback);   // IRQ 14
    register_interrupt_handler(47, ata_secondary_callback); // IRQ 15
}

int ata_has_dma() {
    return has_dma_support;
}

unsigned int ata_get_bmide_base() {
    return bmide_io_base;
}

int ata_identify_device(unsigned short io_base, unsigned char drive, unsigned short* buffer) {
    port_byte_out(io_base + ATA_REG_DRIVE, 0xA0 | (drive << 4));
    port_byte_out(io_base + ATA_REG_SEC_COUNT, 0);
    port_byte_out(io_base + ATA_REG_LBA_LO, 0);
    port_byte_out(io_base + ATA_REG_LBA_MID, 0);
    port_byte_out(io_base + ATA_REG_LBA_HI, 0);
    port_byte_out(io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    
    unsigned char status = port_byte_in(io_base + ATA_REG_STATUS);
    if (status == 0) return -1;
    
    while (status & ATA_STATUS_BSY) {
        status = port_byte_in(io_base + ATA_REG_STATUS);
    }
    
    unsigned char cl = port_byte_in(io_base + ATA_REG_LBA_MID);
    unsigned char ch = port_byte_in(io_base + ATA_REG_LBA_HI);
    if (cl == 0x14 && ch == 0xEB) return -2;
    if (cl == 0x69 && ch == 0x96) return -2;
    
    while (!(status & ATA_STATUS_DRQ)) {
        if (status & ATA_STATUS_ERR) return -3;
        status = port_byte_in(io_base + ATA_REG_STATUS);
    }
    
    for (int i = 0; i < 256; i++) {
        buffer[i] = port_word_in(io_base + ATA_REG_DATA);
    }
    return 0;
}

int ata_read_pio(unsigned short io_base, unsigned char drive, unsigned int lba, unsigned char count, unsigned short* buffer) {
    port_byte_out(io_base + ATA_REG_DRIVE, 0xE0 | (drive << 4) | ((lba >> 24) & 0x0F));
    port_byte_out(io_base + ATA_REG_FEATURES, 0x00);
    port_byte_out(io_base + ATA_REG_SEC_COUNT, count);
    port_byte_out(io_base + ATA_REG_LBA_LO, (unsigned char)(lba & 0xFF));
    port_byte_out(io_base + ATA_REG_LBA_MID, (unsigned char)((lba >> 8) & 0xFF));
    port_byte_out(io_base + ATA_REG_LBA_HI, (unsigned char)((lba >> 16) & 0xFF));
    
    port_byte_out(io_base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);
    
    for (int sector = 0; sector < count; sector++) {
        unsigned char status = port_byte_in(io_base + ATA_REG_STATUS);
        while ((status & ATA_STATUS_BSY) && !(status & ATA_STATUS_ERR)) {
            status = port_byte_in(io_base + ATA_REG_STATUS);
        }
        if (status & ATA_STATUS_ERR) {
            return -1;
        }
        while (!(status & ATA_STATUS_DRQ)) {
            status = port_byte_in(io_base + ATA_REG_STATUS);
        }
        
        for (int i = 0; i < 256; i++) {
            buffer[sector * 256 + i] = port_word_in(io_base + ATA_REG_DATA);
        }
    }
    return 0;
}

int ata_write_pio(unsigned short io_base, unsigned char drive, unsigned int lba, unsigned char count, const unsigned short* buffer) {
    port_byte_out(io_base + ATA_REG_DRIVE, 0xE0 | (drive << 4) | ((lba >> 24) & 0x0F));
    port_byte_out(io_base + ATA_REG_FEATURES, 0x00);
    port_byte_out(io_base + ATA_REG_SEC_COUNT, count);
    port_byte_out(io_base + ATA_REG_LBA_LO, (unsigned char)(lba & 0xFF));
    port_byte_out(io_base + ATA_REG_LBA_MID, (unsigned char)((lba >> 8) & 0xFF));
    port_byte_out(io_base + ATA_REG_LBA_HI, (unsigned char)((lba >> 16) & 0xFF));
    
    port_byte_out(io_base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
    
    for (int sector = 0; sector < count; sector++) {
        unsigned char status = port_byte_in(io_base + ATA_REG_STATUS);
        while ((status & ATA_STATUS_BSY) && !(status & ATA_STATUS_ERR)) {
            status = port_byte_in(io_base + ATA_REG_STATUS);
        }
        if (status & ATA_STATUS_ERR) {
            return -1;
        }
        while (!(status & ATA_STATUS_DRQ)) {
            status = port_byte_in(io_base + ATA_REG_STATUS);
        }
        
        for (int i = 0; i < 256; i++) {
            port_word_out(io_base + ATA_REG_DATA, buffer[sector * 256 + i]);
        }
    }
    
    port_byte_out(io_base + ATA_REG_COMMAND, 0xE7); // Cache Flush
    unsigned char status = port_byte_in(io_base + ATA_REG_STATUS);
    while (status & ATA_STATUS_BSY) {
        status = port_byte_in(io_base + ATA_REG_STATUS);
    }
    
    return 0;
}

int ata_read_dma(unsigned short io_base, unsigned char drive, unsigned int lba, unsigned char count, unsigned short* buffer) {
    if (!has_dma_support || bmide_io_base == 0) {
        return -2;
    }
    
    unsigned int bytes = count * 512;
    if (bytes > DMA_BOUNCE_SIZE) {
        return -3;
    }
    
    unsigned short bm_base = (io_base == ATA_PRIMARY_IO) ? bmide_io_base : (bmide_io_base + 8);
    
    port_byte_out(bm_base + 0x00, 0x00);
    port_byte_out(bm_base + 0x02, 0x06);
    
    prd_table[0].phys_addr = (unsigned int)dma_bounce_buffer;
    prd_table[0].byte_count = bytes;
    prd_table[0].reserved_eot = PRD_EOT;
    
    port_dword_out(bm_base + 0x04, (unsigned int)prd_table);
    port_byte_out(bm_base + 0x00, 0x08);
    
    port_byte_out(io_base + ATA_REG_DRIVE, 0xE0 | (drive << 4) | ((lba >> 24) & 0x0F));
    ata_io_wait(io_base + 0x206);
    
    port_byte_out(io_base + ATA_REG_FEATURES, 0x00);
    port_byte_out(io_base + ATA_REG_SEC_COUNT, count);
    port_byte_out(io_base + ATA_REG_LBA_LO, lba & 0xFF);
    port_byte_out(io_base + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
    port_byte_out(io_base + ATA_REG_LBA_HI, (lba >> 16) & 0xFF);
    
    port_byte_out(io_base + ATA_REG_COMMAND, ATA_CMD_READ_DMA);
    
    ata_irq_fired = 0;
    port_byte_out(bm_base + 0x00, 0x08 | 0x01);
    
    unsigned int timeout = 10000000;
    while (!ata_irq_fired && timeout > 0) {
        unsigned char bm_status = port_byte_in(bm_base + 0x02);
        if (!(bm_status & 0x01)) {
            break;
        }
        if (bm_status & 0x02) {
            break;
        }
        timeout--;
    }
    
    port_byte_out(bm_base + 0x00, 0x00);
    
    unsigned char bm_status = port_byte_in(bm_base + 0x02);
    unsigned char ata_status = port_byte_in(io_base + ATA_REG_STATUS);
    
    port_byte_out(bm_base + 0x02, 0x06);
    
    if (bm_status & 0x02) {
        return -1;
    }
    if (ata_status & ATA_STATUS_ERR) {
        return -1;
    }
    
    memory_copy((const char*)dma_bounce_buffer, (char*)buffer, bytes);
    return 0;
}

int ata_write_dma(unsigned short io_base, unsigned char drive, unsigned int lba, unsigned char count, const unsigned short* buffer) {
    if (!has_dma_support || bmide_io_base == 0) {
        return -2;
    }
    
    unsigned int bytes = count * 512;
    if (bytes > DMA_BOUNCE_SIZE) {
        return -3;
    }
    
    memory_copy((const char*)buffer, (char*)dma_bounce_buffer, bytes);
    
    unsigned short bm_base = (io_base == ATA_PRIMARY_IO) ? bmide_io_base : (bmide_io_base + 8);
    
    port_byte_out(bm_base + 0x00, 0x00);
    port_byte_out(bm_base + 0x02, 0x06);
    
    prd_table[0].phys_addr = (unsigned int)dma_bounce_buffer;
    prd_table[0].byte_count = bytes;
    prd_table[0].reserved_eot = PRD_EOT;
    
    port_dword_out(bm_base + 0x04, (unsigned int)prd_table);
    port_byte_out(bm_base + 0x00, 0x00);
    
    port_byte_out(io_base + ATA_REG_DRIVE, 0xE0 | (drive << 4) | ((lba >> 24) & 0x0F));
    ata_io_wait(io_base + 0x206);
    
    port_byte_out(io_base + ATA_REG_FEATURES, 0x00);
    port_byte_out(io_base + ATA_REG_SEC_COUNT, count);
    port_byte_out(io_base + ATA_REG_LBA_LO, lba & 0xFF);
    port_byte_out(io_base + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
    port_byte_out(io_base + ATA_REG_LBA_HI, (lba >> 16) & 0xFF);
    
    port_byte_out(io_base + ATA_REG_COMMAND, ATA_CMD_WRITE_DMA);
    
    ata_irq_fired = 0;
    port_byte_out(bm_base + 0x00, 0x01);
    
    unsigned int timeout = 10000000;
    while (!ata_irq_fired && timeout > 0) {
        unsigned char bm_status = port_byte_in(bm_base + 0x02);
        if (!(bm_status & 0x01)) {
            break;
        }
        if (bm_status & 0x02) {
            break;
        }
        timeout--;
    }
    
    port_byte_out(bm_base + 0x00, 0x00);
    
    unsigned char bm_status = port_byte_in(bm_base + 0x02);
    unsigned char ata_status = port_byte_in(io_base + ATA_REG_STATUS);
    
    port_byte_out(bm_base + 0x02, 0x06);
    
    if (bm_status & 0x02) {
        return -1;
    }
    if (ata_status & ATA_STATUS_ERR) {
        return -1;
    }
    
    port_byte_out(io_base + ATA_REG_COMMAND, 0xE7); // Cache Flush
    unsigned char status = port_byte_in(io_base + ATA_REG_STATUS);
    while (status & ATA_STATUS_BSY) {
        status = port_byte_in(io_base + ATA_REG_STATUS);
    }
    
    return 0;
}