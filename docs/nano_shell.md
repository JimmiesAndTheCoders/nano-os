# Using Nano Shell

Once booted, interact with the system using the following commands:

## General

- `help`: Displays a list of available commands.
- `clear`: Clears the screen.
- `echo [text]`: Print a line of text.
- `pwd`: Print the current working directory.
- `cd [dir]`: Change directory (validated path check).
- `ls`: List available files in this directory.
- `cat [file]`: Display the contents of a file.
- `grep [pat] [f]`: Find lines matching a pattern in a file.
- `touch [file]`: Create an empty file.
- `mkdir [dir]`: Create a directory.
- `cnode [file]`: Run the terminal text/code editor.
- `date`: View current Real-Time Clock date and time.
- `sync`: Flush all dirty pages and blocks to disk.
- `cache-stats`: View system page and buffer cache statistics.
- `status`: Displays kernel operational metrics.
- `nano --status`: View system mascot information.
- `halt`: Safely stops the CPU.

## PCI

- `pci`: List all detected PCI bus devices.
    - `msi-enable [index] [vector]`: Enable MSI on specified device.
    - `msi-disable [index]`: Disable MSI on specified device.
    - `msix-enable [index] [vector]`: Enable MSI on specified device.
    - `msi-disable [index]`: Disable MSI on specified device.
    
## ATA

- `ata-identify`: Identify primary master ATA drive.
- `ata-read [lba] [count]`: Read sectors using PIO.
- `ata-write [lba] [text]`: Write text to sector using PIO.
- `ata-dma-read [lba] [count]`: Read sectors using DMA.
- `ata-dma-write [lba] [text]`: Write text to sector using DMA.