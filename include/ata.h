#ifndef ATA_H
#define ATA_H

#ifdef __cplusplus
extern "C" {
#endif

#define ATA_PRIMARY_IO      0x1F0
#define ATA_SECONDARY_IO    0x170

#define ATA_DRIVE_MASTER    0
#define ATA_DRIVE_SLAVE     1

void init_ata();
int ata_has_dma();
unsigned int ata_get_bmide_base();

int ata_read_pio(unsigned short io_base, unsigned char drive, unsigned int lba, unsigned char count, unsigned short* buffer);
int ata_write_pio(unsigned short io_base, unsigned char drive, unsigned int lba, unsigned char count, const unsigned short* buffer);
int ata_identify_device(unsigned short io_base, unsigned char drive, unsigned short* buffer);

int ata_read_dma(unsigned short io_base, unsigned char drive, unsigned int lba, unsigned char count, unsigned short* buffer);
int ata_write_dma(unsigned short io_base, unsigned char drive, unsigned int lba, unsigned char count, const unsigned short* buffer);

#ifdef __cplusplus
}
#endif

#endif