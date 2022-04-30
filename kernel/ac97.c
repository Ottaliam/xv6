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

#define PCI_BASE            0x30000000L
#define PCI_CONFIG_STATUS_COMMAND  0x1
#define PCI_CONFIG_BAR0     0x4
#define PCI_CONFIG_BAR1     0x5

uint64 NAMBA;               // BAR0 - Native Audio Mixer Base Address
#define RESET               0x00

uint64 NABMBA;              // BAR1 - Native Audio Bus Master Base Address
#define PO_TRANSFER_STATUS  0x16
    #define LVE_INTR        0x4
    #define IOC_INTR        0x8
    #define FIFO_INTR       0x10
    #define ALL_INTR        0x1C
#define GLOBAL_CONTROL      0x2C
#define GLOBAL_STATUS       0x30

// buffer descriptor list
#define BDL_SIZE            32
uint64 BDL[BDL_SIZE];
uint BDL_w, BDL_r, BDL_slot;
struct spinlock BDL_lock;

// search for ac97 device among PCI buses
// and call for initialization
void
ac97_init()
{
    uint32 bus = 0, device, func = 0, vedeID;
    uint32 *address;

    // Checking every device in bus 0
    for(bus = 0; bus < 256; bus++)
        for(device = 0; device < 32; device++)
        {
            address = (uint32 *)(PCI_BASE + ((bus << 16) | (device << 11) | (func << 8) | 0));
            vedeID = address[0];
            if(vedeID == 0x24158086)
            {
                printf("Successfully Find AC97\n");
                ac97_device_init(address);
                return;
            }
        }
    panic("ac97 device not found.\n");
}

// initialize ac97 device
void
ac97_device_init(uint32* pci_config)
{
    pci_config[PCI_CONFIG_STATUS_COMMAND] |= 0x5;

    NAMBA = pci_config[PCI_CONFIG_BAR0];
    NABMBA = pci_config[PCI_CONFIG_BAR1];

    printf("%p\n", NAMBA);
    printf("%p\n", NABMBA);

    // set with interrupt
    sw(0x3, GLOBAL_CONTROL + NABMBA);


    // reset NAM
    sh(1, RESET + NAMBA);

    // wait for primary codec ready
    while(!(lw(GLOBAL_STATUS + NABMBA) & 0x100));

    BDL_w = 0;
    BDL_r = 0;
    BDL_slot = 32;
}

// add a entry to BDL
// if already full, sleep until available slot
int
BDL_enqueue(uint32 addr, uint16 nsample, uint8 BUP)
{
    uint64 laddr = (uint64)addr,
           lnsample = (uint64)nsample,
           entry;
    
    entry = (laddr & 0xFFFFFFFE) | ((lnsample & 0xFFFE) << 32) | ((uint64)(1) << 63);
    if(BUP)
        entry |= ((uint64)(1) << 62);

    acquire(&BDL_lock);

    while(BDL_slot == 32)
    {/*
        if(myproc()->killed)
        {
            release(&BDL_lock);
            return -1;
        }*/
        sleep(&BDL_r, &BDL_lock);
    }

    BDL[BDL_w++ % BDL_SIZE] = entry;
    BDL_slot--;
    // wakeup(&BDL_w);

    release(&BDL_lock);

    return nsample;
}

// interrupt handler
void
ac97_interrupt()
{
    uint16 PO_status = lh(PO_TRANSFER_STATUS + NABMBA);

    // last valid entry reached
    if(PO_status & LVE_INTR)
    {
        // todo: add correspondence
    }

    // one entry is consumed
    if(PO_status & IOC_INTR)
    {
        acquire(&BDL_lock);
        BDL_slot++;
        BDL_r++;
        wakeup(&BDL_r);
        release(&BDL_lock);
    }

    // BDL error
    if(PO_status & FIFO_INTR)
    {
        // todo: add correspondence
    }

    sh(ALL_INTR, PO_TRANSFER_STATUS + NABMBA);
}