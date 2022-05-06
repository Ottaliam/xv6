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

#define PCIE_ECAM                   0x30000000L
#define PCIE_MMIO                   0x40000000L
#define PCI_CONFIG_STATUS_COMMAND   0x4
#define PCI_CONFIG_BAR0             0x10
#define PCI_CONFIG_BAR1             0x14

uint32 NAMBA;               // BAR0 - Native Audio Mixer Base Address
#define RESET               0x00

uint32 NABMBA;              // BAR1 - Native Audio Bus Master Base Address
#define PO_TRANSFER_STATUS  0x16
    #define LVE_INTR        0x4
    #define IOC_INTR        0x8
    #define FIFO_INTR       0x10
    #define ALL_INTR        0x1C
#define GLOBAL_CONTROL      0x2C
#define GLOBAL_STATUS       0x30

// initialize ac97 device
void
ac97_device_init(uint32 pci_config)
{
    printf("%x\n", pci_config);

    sw(0x0001, PCI_BASE | pci_config | PCI_CONFIG_BAR0);
    NAMBA = lw(PCI_BASE | pci_config | PCI_CONFIG_BAR0) & (~0x1);
    __sync_synchronize();

    sw(0x0401, PCI_BASE | pci_config | PCI_CONFIG_BAR1);
    NABMBA = lw(PCI_BASE | pci_config | PCI_CONFIG_BAR1) & (~0x1);
    __sync_synchronize();

    printf("BAR0: %x\nBAR1: %x\n", NAMBA, NABMBA);

    // cold reset without interrupt
    sw(0x2, NABMBA + GLOBAL_CONTROL);

    // reset NAM
    sh(1, NAMBA + RESET);

    // wait for primary codec ready
    while(!(lw(NABMBA + GLOBAL_STATUS) & 0x100));
}

// search for ac97 device among PCI buses
// and call for initialization
void
soundinit()
{
    uint32 bus = 0, device, func = 0, vedeID, addr;

    // Checking every device
    for(bus = 0; bus < 256; bus++)
        for(device = 0; device < 32; device++)
        {
            addr = (PCI_BASE + ((bus << 20) | (device << 15) | (func << 12) | 0));
            vedeID = lw(addr);
            if(vedeID == 0x24158086)
            {
                printf("Successfully Find AC97\n");
                ac97_device_init(addr);
                return;
            }
        }
    panic("ac97 device not found.\n");
}

/*
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
//#include "sound.h"

#define PCIE_ECAM 0x30000000L
#define PCIE_PIO  0x3000000L

#define FOR(i, a, b) for (uint32 i = (a), i##_END_ = (b); i <= i##_END_; ++i)

// all registers address can be found in https://wiki.osdev.org/AC97


static struct spinlock sound_lock;
ushort namba; // native audio mixer base address
ushort nabmba; // native audio bus mastering base address

volatile uchar* RegByte(uint64 reg) {return (volatile uchar *)(reg);}
volatile uint32* RegInt(uint64 reg) {return (volatile uint32 *)(reg);}
volatile ushort* RegShort(uint64 reg) {return (volatile ushort *)(reg);}
uchar ReadRegByte(uint64 reg) {return *RegByte(reg);}
void WriteRegByte(uint64 reg, uchar v) {*RegByte(reg) = v;}
uint32 ReadRegInt(uint64 reg) {return *RegInt(reg);}
void WriteRegInt(uint64 reg, uint32 v) {*RegInt(reg) = v;}
ushort ReadRegShort(uint64 reg) {return *RegShort(reg);}
void WriteRegShort(uint64 reg, ushort v) {*RegShort(reg) = v;}

// Ecam: PCI Configuration Space
uint32 read_pci_config_int(uint32 bus, uint32 slot, uint32 func, uint32 offset) {
  return ReadRegInt((bus << 20) | (slot << 15) | (func << 12) | (offset) | PCIE_ECAM);
}

uint32 read_pci_config_byte(uint32 bus, uint32 slot, uint32 func, uint32 offset) {
  return ReadRegByte((bus << 20) | (slot << 15) | (func << 12) | (offset) | PCIE_ECAM);
}

void write_pci_config_byte(uint32 bus, uint32 slot, uint32 func, uint32 offset, uchar val) {
  WriteRegByte((bus << 20) | (slot << 15) | (func << 12) | (offset) | PCIE_ECAM, val);
}

void write_pci_config_int(uint32 bus, uint32 slot, uint32 func, uint32 offset, uint32 val) {
  WriteRegInt((bus << 20) | (slot << 15) | (func << 12) | (offset) | PCIE_ECAM, val);
}

void write_pci_config_short(uint32 bus, uint32 slot, uint32 func, uint32 offset, ushort val) {
  WriteRegShort((bus << 20) | (slot << 15) | (func << 12) | (offset) | PCIE_ECAM, val);
}


void soundcard_init(uint32 bus, uint32 slot, uint32 func) {
  initlock(&sound_lock, "sound");

  // Initializing the Audio I/O Space
  write_pci_config_byte(bus, slot, func, 0x4, 0x5);

  // Write Native Audio Mixer Base Address
  write_pci_config_int(bus, slot, func, 0x10, 0x0001);
  namba = read_pci_config_int(bus, slot, func, 0x10) & (~0x1);
  
  // Write Native Audio Bus Mastering Base Address
  write_pci_config_int(bus, slot, func, 0x14, 0x0401);
  nabmba = read_pci_config_int(bus, slot, func, 0x14) & (~0x1);

  printf("BAR SUCCESS\n");
  printf("%x\n%x\n", namba, nabmba);
  
  // Hardware Interrupt Routing
  // write_pci_config_byte(bus, slot, func, 0x3c, PCI_IRQ); no effect !!!

  // Removing AC_RESET#
  WriteRegByte(PCIE_PIO | (nabmba + 0x2c), 0x2);

  // Check until codec ready
  uint32 wait_time = 1000;
  while (!(ReadRegShort(PCIE_PIO | (nabmba + 0x30)) & (0x100)) && wait_time) {
    --wait_time;
  }
  if (!wait_time) {
    panic("Audio Init failed 0.");
    return;
  }
  // Determining the Audio Codec
  WriteRegShort(PCIE_PIO | (namba + 0x2), 0x8000);
  if ((ReadRegShort(PCIE_PIO | (namba + 0x2))) != 0x8000) {
    panic("Audio Init failed 1.");
    return;
  }
  // Reading the Audio Codec Vendor ID
  uint32 vendorID1 = ReadRegShort(PCIE_PIO | (namba + 0x7c));
  uint32 vendorID2 = ReadRegShort(PCIE_PIO | (namba + 0x7e));
  // Programming the PCI Audio Subsystem ID
  uint32 vendorID = (vendorID2 << 16) + vendorID1;
  write_pci_config_int(bus, slot, func, 0x2c, vendorID);
}

void soundinit(void) {
  // scan the pci configuration space
  FOR(bus, 0, 1) FOR(slot, 0, 31) FOR(func, 0, 7) {
    uint32 res = read_pci_config_int(bus, slot, func, 0);
    uint32 vendor = res & 0xffff;
    uint32 device = (res >> 16) & 0xffff;
    if (vendor == 0x8086 && device == 0x2415) {
      soundcard_init(bus, slot, func);
      return;
    }
  }
}
*/