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
#include "ac97.h"

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
#define RESET                       0x00
#define MA_VOLUME                   0x02
#define PCM_OUT_VOLUME              0x18

uint64 NABMBA;              // BAR1 - Native Audio Bus Master Base Address
#define PCM_OUT_BDLBA               0x10
#define PCM_OUT_CUR                 0x14
#define PCM_OUT_LVE                 0x15
#define PCM_OUT_TRANSFER_STATUS     0x16
#define PCM_OUT_TRANSFER_CONTROL    0x1B
#define GLOBAL_CONTROL              0x2C
#define GLOBAL_STATUS               0x30

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
    // Using 0x0001 here will cause Store/AMO access fault, reason unknown
    WriteRegInt(pci_addr + PCI_CONFIG_BAR0, 0x0401);
    NAMBA = ReadRegInt(pci_addr + PCI_CONFIG_BAR0) & (~0x1);

    WriteRegInt(pci_addr + PCI_CONFIG_BAR1, 0x0801);
    NABMBA = ReadRegInt(pci_addr + PCI_CONFIG_BAR1) & (~0x1);

    // printf("BAR Successfully Set\n");
    // printf("BAR0: %p\nBAR1: %p\n", NAMBA, NABMBA);

    // Cold Reset Without Interrupt
    WriteRegByte(PCIE_PIO | (NABMBA + GLOBAL_CONTROL), 0x2);
    // printf("Reset Success\n");

    // Wait For Codec to be Read
    uint32 wait_time = 1000;
    while(!(ReadRegShort(PCIE_PIO | (NABMBA + GLOBAL_STATUS)) & 0x100) && wait_time)
        --wait_time;
    if(!wait_time)
    {
        panic("Audio Init Failed 0.\n");
        return;
    }
    // printf("Codec Ready\n");

    // Check Codec
    WriteRegShort(PCIE_PIO | (NAMBA + MA_VOLUME), 0x8000);
    if((ReadRegShort(PCIE_PIO | (NAMBA + MA_VOLUME))) != 0x8000)
    {
        panic("Codec Fail\n");
        return;
    }

    // Update PCIe Audio Subsystem ID
    uint32 v01 = ReadRegShort(PCIE_PIO | (NAMBA + 0x7C));
    uint32 v02 = ReadRegShort(PCIE_PIO | (NAMBA + 0x7E));
    uint32 v00 = (v02 << 16) + v01;
    WriteRegInt(pci_addr + 0x2C, v00);

    // Reset NAM Registers
    WriteRegShort(PCIE_PIO | (NAMBA + RESET), 0x1);

    // 
    picenable();
}

uint64 BDL[32];
struct audionode *shead, *stail;

void BDL_add_entry(struct audionode *node)
{
    acquire(&sound_lock);
    
    node->next = 0;
    node->used = 0;

    if(!shead)
        shead = stail = node;
    else
    {
        stail->next = node;
        stail = node;
    }

    release(&sound_lock);
}

void start_play()
{
    // Set Volume
    WriteRegShort((PCIE_PIO | (NAMBA + MA_VOLUME)), 0x0);
    WriteRegShort((PCIE_PIO | (NAMBA + PCM_OUT_VOLUME)), 0x808);
    
    // BDL initialization
    acquire(&sound_lock);
    if(!shead)
    {
        release(&sound_lock);
        return;
    }
    for(int i = 0; i < 32; i++)
        BDL[i] = ((uint64)(shead->data[i]) | (((uint64)(NUM_SAMPLE)) << 32));
    release(&sound_lock);
    
    // Reset Transfer Control
    WriteRegByte((PCIE_PIO | (NABMBA + PCM_OUT_TRANSFER_CONTROL)), 0x2);
    while((ReadRegByte(PCIE_PIO | (NABMBA + PCM_OUT_TRANSFER_CONTROL))) & 0x2);

    // Write BDL info
    WriteRegInt((PCIE_PIO | (NABMBA + PCM_OUT_BDLBA)), ((uint64)(&BDL) & 0xFFFFFFF8));
    WriteRegByte((PCIE_PIO | (NABMBA + PCM_OUT_LVE)), 0x1F);

    // Start Transfer
    WriteRegByte((PCIE_PIO | (NABMBA + PCM_OUT_TRANSFER_CONTROL)), 0x1);

    // Update BDL
    while(1)
    {
        if(ReadRegShort(PCIE_PIO | (NABMBA + PCM_OUT_TRANSFER_STATUS)) & 0x2)
        {
            acquire(&sound_lock);

            shead->used = 1;
            shead = shead->next;

            if(!shead)
            {
                release(&sound_lock);
                return;
            }
            for(int i = 0; i < 32; i++)
                BDL[i] = ((uint64)(shead->data[i]) | (((uint64)(NUM_SAMPLE)) << 32));
            shead = shead->next;

            release(&sound_lock);
        }
    }
}

void pause_play()
{
    uchar tmp = ReadRegByte(PCIE_PIO | (NABMBA + PCM_OUT_TRANSFER_CONTROL));
    tmp &= 0xFE;    // Clear Bit 0
    WriteRegByte((PCIE_PIO | (NABMBA + PCM_OUT_TRANSFER_CONTROL)), tmp);
}

void continue_play()
{
    uchar tmp = ReadRegByte(PCIE_PIO | (NABMBA + PCM_OUT_TRANSFER_CONTROL));
    tmp |= 0x1;    // Set Bit 0
    WriteRegByte((PCIE_PIO | (NABMBA + PCM_OUT_TRANSFER_CONTROL)), tmp);
}

void stop_play()
{
    pause_play();
    
    // Reset Transfer Control
    WriteRegByte((PCIE_PIO | (NABMBA + PCM_OUT_TRANSFER_CONTROL)), 0x2);
    
    // Release audiolist
    acquire(&sound_lock);
    while(shead)
    {
        shead->used = 1;
        shead = shead->next;
    }
    shead = stail = 0;
    release(&sound_lock);
}

void set_volume(ushort volume)
{
    WriteRegShort((PCIE_PIO | (NAMBA + MA_VOLUME)), volume);
}