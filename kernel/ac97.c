//
// driver for 80801AA AC'97 Audio Controller
// Vendor ID: 8086
// Device ID: 2415
//

#include "types.h"
#include "defs.h"
#include "riscv.h"

#define PCI_CONFIG_STATUS_COMMAND  0x4

uint32 NAMBA;               // BAR0 - Native Audio Mixer Base Address
#define PCI_BAR_0           0x10
#define RESET               0x00

uint32 NABMBA;              // BAR1 - Native Audio Bus Master Base Address
#define PCI_BAR_1           0x14
#define GLOBAL_CONTROL      0x2C
#define GLOBAL_STATUS       0x30

// search for ac97 device among PCI buses
// and call for initialization
void
ac97_init()
{
    uint16 bus, vendorID, deviceID;
    uint8 device;
    for(bus = 0; bus < 256; bus++)
        for(device = 0; device < 32; device++)
        {
            deviceID = pci_config_read_word(bus, device, 0, 0);
            vendorID = pci_config_read_word(bus, device, 0, 2);
            if(vendorID == 0x8086 && deviceID == 0x2415)
            {
                ac97_device_init(bus, device);
                return;
            }
        }
    panic("ac97 device not found.\n");
}

// initialize ac97 device
void
ac97_device_init(uint8 bus, uint8 device)
{
    uint32 tmp;
    
    tmp = pci_config_read_32(bus, device, 0, PCI_CONFIG_STATUS_COMMAND);
    pci_config_write_32(tmp | 0x5, bus, device, 0, PCI_CONFIG_STATUS_COMMAND);

    NAMBA = pci_config_read_32(bus, device, 0, PCI_BAR_0);
    NABMBA = pci_config_read_32(bus, device, 0, PCI_BAR_1);

    // set with interrupt
    sw(0x3, GLOBAL_CONTROL, NABMBA);

    // reset NAM
    sh(1, RESET, NAMBA);

    // wait for primary codec ready
    while(!(lw(GLOBAL_STATUS, NABMBA) & 0x100));
}