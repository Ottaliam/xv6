//
// support functions for pci access.
//

#include "types.h"
#include "defs.h"
#include "riscv.h"

#define PCI_CONFIG_ADDRESS  0xCF8
#define PCI_CONFIG_DATA     0xCFC

// Using PCI Configuration Space Access Mechanism to read a dword (32bit) from Configuration Space
uint32
pci_config_read_32(uint8 bus, uint8 device, uint8 func, uint8 offset)
{
    uint32 address,
           lbus = (uint32)bus,
           ldevice = (uint32)device,
           lfunc = (uint32)func;
    uint32 res;

    address = (uint32)((lbus << 16) | (ldevice << 11) | (lfunc << 8) | (offset & 0xFC) | (uint32)0x80000000);

    sw(address, 0, PCI_CONFIG_ADDRESS);
    res = lw(0, PCI_CONFIG_DATA);
    return res;
}

// Using PCI Configuration Space Access Mechanism to read a word (16bit) from Configuration Space
uint16
pci_config_read_16(uint8 bus, uint8 device, uint8 func, uint8 offset)
{
    uint32 tmp;
    uint16 res;

    tmp = pci_config_read_dword(bus, device, func, offset);
    res = (uint16)((tmp >> ((offset & 2) * 8)) & 0xFFFF);
    return tmp;
}

// Using PCI Configuration Space Access Mechanism to write a dword (32bit) from Configuration Space
uint32
pci_config_write_32(uint32 data, uint8 bus, uint8 device, uint8 func, uint8 offset)
{
    uint32 address,
           lbus = (uint32)bus,
           ldevice = (uint32)device,
           lfunc = (uint32)func;

    address = (uint32)((lbus << 16) | (ldevice << 11) | (lfunc << 8) | (offset & 0xFC) | (uint32)0x80000000);

    sw(address, 0, PCI_CONFIG_ADDRESS);
    sw(data, 0, PCI_CONFIG_DATA);
}