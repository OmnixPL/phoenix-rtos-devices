/*
 * Phoenix-RTOS
 *
 * Generic IDE controller driver
 *
 * Copyright 2012-2015, 2019, 2020 Phoenix Systems
 * Author: Marcin Stragowski, Kamil Amanowicz, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _PC_IDEDRV_H_
#define _PC_IDEDRV_H_

#include <stdint.h>
#include <sys/types.h>

#include <phoenix/arch/ia32.h>


/* Driver base definitions */
#define PCI_STORAGE_IDE 0x0101 /* IDE PCI class code */
#define PRIMARY_BASE    0x1F0  /* Primary channel legacy base address */
#define PRIMARY_CTRL    0x3F6  /* Primary channel legacy ctrl address */
#define PRIMARY_IRQ     0x0E   /* Primary channel legacy interrupt number */
#define SECONDARY_BASE  0x170  /* Secondary channel legacy base address */
#define SECONDARY_CTRL  0x376  /* Secondary channel legacy ctrl address */
#define SECONDARY_IRQ   0x0F   /* Secondary channel legacy interrupt number */


/* Driver operation modes */
enum { MODE_POLL, MODE_IRQ, MODE_DMA };


/* Supported IDE devices */
enum { ATA, ATAPI };


/* ATA addressing modes */
enum { AMODE_CHS, AMODE_LBA28, AMODE_LBA48 };


/* ATA commands */
enum {
	CMD_NOP             = 0x00,
	CMD_READ_PIO        = 0x20,
	CMD_READ_PIO_EXT    = 0x24,
	CMD_READ_DMA_EXT    = 0x25,
	CMD_WRITE_PIO       = 0x30,
	CMD_WRITE_PIO_EXT   = 0x34,
	CMD_WRITE_DMA_EXT   = 0x35,
	CMD_PACKET          = 0xA0,
	CMD_IDENTIFY_PACKET = 0xA1,
	CMD_READ_DMA        = 0xC8,
	CMD_WRITE_DMA       = 0xCA,
	CMD_CACHE_FLUSH     = 0xE7,
	CMD_CACHE_FLUSH_EXT = 0xEA,
	CMD_IDENTIFY        = 0xEC
};


/* ATA registers */
enum {
	REG_CTRL      = 0x0,       /* Control register */
	REG_ALTSTATUS = 0x0,       /* Alternate status */
	REG_DATA      = 0x0,       /* R/W PIO data port */
	REG_ERROR     = 0x1,       /* Read error */
	REG_FEATURES  = 0x1,       /* Write features */
	REG_SECNUM    = 0x2,       /* Sectors number */
	REG_SEC       = 0x3,       /* Sector */
	REG_LCYL      = 0x4,       /* Cylinder low */
	REG_HCYL      = 0x5,       /* Cylinder high */
	REG_HEAD      = 0x6,       /* LBA head */
	REG_DEVSEL    = 0x6,       /* Drive select */
	REG_CMD       = 0x7,       /* Write command */
	REG_STATUS    = 0x7        /* Read status */
};


/* REG_CTRL layout */
enum {
	CTRL_NIEN     = 0x02,      /* Not Interrupt ENabled */
	CTRL_SOFTRST  = 0x04,      /* Software reset all ATA devices on the bus */
	CTRL_HOB      = 0x80       /* Read back the High Order Byte of the last LBA48 value sent */
};


/* REG_DEVSEL layout */
enum {
	DEVSEL_HEAD   = 0x0F,      /* LBA head */
	DEVSEL_DEVNUM = 0x10,      /* 0: Master, 1: Slave */
	DEVSEL_SET0   = 0x20,      /* Should always be set */
	DEVSEL_LBA    = 0x40,      /* 0: CHS, 1: LBA */
	DEVSEL_SET1   = 0x80       /* Should always be set */
};


/* REG_STATUS layout */
enum {
	STATUS_ERR    = 0x01,      /* Error occured for last command */
	STATUS_DRQ    = 0x08,      /* PIO data is ready */
	STATUS_SRV    = 0x10,      /* Overlapped mode service request */
	STATUS_DF     = 0x20,      /* Device fault (doesn't set ERR) */
	STATUS_RDY    = 0x40,      /* Device is ready */
	STATUS_BSY    = 0x80       /* Device is busy */
};


typedef struct channel_t channel_t;


typedef struct {
	uint8_t exists;            /* Device detected */
	uint8_t type;              /* 0: ATA, 1: ATAPI */
	uint8_t mode;              /* 0: CHS, 1: LBA28, 2: LBA48 */
	uint8_t dma;               /* DMA support */

	/* Device geometry */
	uint32_t cylinders;        /* Cylinders number */
	uint32_t heads;            /* Heads number */
	uint32_t sectors;          /* Sectors number */
	uint64_t size;             /* Storage size */

	channel_t *channel;        /* IDE channel */
} dev_t;


typedef struct {
	uint32_t addr;
	uint8_t (*readb)(uint32_t, uint32_t);
	uint16_t (*readw)(uint32_t, uint32_t);
	void (*writeb)(uint32_t, uint32_t, uint8_t);
	void (*writew)(uint32_t, uint32_t, uint16_t);
} reg_t;


typedef struct bus_t bus_t;


struct channel_t {
	bus_t *bus;                /* IDE bus */
	dev_t devices[2];          /* IDE devices */

	/* Channel IO-ports / memory mappings */
	reg_t base;                /* Command registers access */
	reg_t ctrl;                /* Control resgister access */
	reg_t dma;                 /* PCI DMA registers access */

	/* Interrupts handling */
	unsigned char irq;         /* Interrupt number */
	handle_t cond;             /* Interrupt condition variable */
	handle_t inth;             /* Interrupt handle */

	/* Synchronization */
	handle_t lock;             /* Channel access mutex */
};


struct bus_t {
	pci_device_t device;       /* IDE bus device */
	channel_t channels[2];     /* IDE bus channels */

	void *prev, *next;         /* Doubly linked list */
};


#endif
