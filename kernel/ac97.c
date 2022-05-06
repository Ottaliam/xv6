//
// driver for 80801AA AC'97 Audio Controller
// Vendor ID: 8086
// Device ID: 2415
//

#include "types.h"
#include "riscv.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

volatile uchar* RegByte(uint64 reg) {return (volatile uchar *)(reg);}
volatile uint32* RegInt(uint64 reg) {return (volatile uint32 *)(reg);}
volatile ushort* RegShort(uint64 reg) {return (volatile ushort *)(reg);}
uchar ReadRegByte(uint64 reg) {return *RegByte(reg);}
void WriteRegByte(uint64 reg, uchar v) {*RegByte(reg) = v;}
uint32 ReadRegInt(uint64 reg) {return *RegInt(reg);}
void WriteRegInt(uint64 reg, uint32 v) {*RegInt(reg) = v;}
ushort ReadRegShort(uint64 reg) {return *RegShort(reg);}
void WriteRegShort(uint64 reg, ushort v) {*RegShort(reg) = v;}

#define PCIE_PIO                    0x03000000L
#define PCIE_ECAM                   0x30000000L
#define PCIE_MMIO                   0x40000000L
#define PCI_CONFIG_COMMAND          0x4
#define PCI_CONFIG_BAR0             0x10
#define PCI_CONFIG_BAR1             0x14

uint64 NAMBA;               // BAR0 - Native Audio Mixer Base Address
#define RESET               0x00
#define MA_VOLUME           0x02

uint64 NABMBA;              // BAR1 - Native Audio Bus Master Base Address
#define GLOBAL_CONTROL      0x2C
#define GLOBAL_STATUS       0x30

struct spinlock sound_lock;

// search for ac97 device and call for initialization
void ac97_init()
{
    uint32 bus = 0, device, func = 0, vendorID;
    uint64 addr;
    for(bus = 0; bus < 32; bus++)
        for(device = 0; device < 32; device++)
        {
            addr = PCIE_ECAM | (bus << 20) | (device << 15) | (func << 12);
            vendorID = ReadRegInt(addr);
            if(vendorID == 0x24158086)
            {
                printf("Successfully Found AC97\n");
                ac97_device_init(addr);
                return;
            }
        }
    panic("AC97 not found\n");
}

// initialize ac97 device
void ac97_device_init(uint64 pci_addr)
{
    initlock(&sound_lock, "sound");

    // Enable Bus Master & I/O Space to rightfully function
    WriteRegShort(pci_addr + PCI_CONFIG_COMMAND, 0x5);

    // BAR set
    WriteRegInt(pci_addr + PCI_CONFIG_BAR0, 0x0001);
    NAMBA = ReadRegInt(pci_addr + PCI_CONFIG_BAR0) & (~0x1);

    WriteRegInt(pci_addr + PCI_CONFIG_BAR1, 0x0401);
    NABMBA = ReadRegInt(pci_addr + PCI_CONFIG_BAR1) & (~0x1);

    __sync_synchronize();

    printf("BAR Successfully Set\n");
    printf("BAR0: %p\nBAR1: %p\n", NAMBA, NABMBA);

    // Cold Reset Without Interrupt
    WriteRegInt(PCIE_PIO | (NABMBA + GLOBAL_CONTROL), 0x2);
    
    printf("Reset Success\n");

    // Wait For Codec to be Ready
    while(!(ReadRegShort(PCIE_PIO | (NABMBA + GLOBAL_STATUS)) & 0x100));

    printf("Codec Ready\n");

    // Check Codec
    WriteRegShort(PCIE_PIO | (NAMBA + MA_VOLUME), 0x8000);
    if(ReadRegShort(PCIE_PIO | (NAMBA + MA_VOLUME)) != 0x8000)
    {
        panic("Codec Fail\n");
        return;
    }

    

    // Reset NAM Registers
    // WriteRegShort(PCIE_PIO | (NAMBA + RESET), 0x1);
    // Reset Register Not Available due to Unknown Reason, Manually Reset:
    // todo: reset here
}