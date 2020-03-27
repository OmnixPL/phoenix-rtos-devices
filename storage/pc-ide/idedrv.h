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


/* IDE operation modes */
enum { MODE_POLL, MODE_IRQ, MODE_DMA };


/* Driver definitions */
#define PCI_STORAGE_IDE 0x0101
#define PRIMARY_BASE    0x1F0
#define PRIMARY_CTRL    0x3F6
#define PRIMARY_IRQ     0x0E
#define SECONDARY_BASE  0x170
#define SECONDARY_CTRL  0x376
#define SECONDARY_IRQ   0x0F


typedef struct channel_t channel_t;


typedef struct {
	uint8_t exists;        /* Device detected */
	uint8_t type;          /* 0: ATA, 1: ATAPI */
	uint8_t mode;          /* 0: CHS, 1: LBA28, 2: LBA48 */
	uint8_t dma;           /* DMA support */
	uint64_t size;         /* Storage size */
	channel_t *channel;    /* IDE channel */
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
	bus_t *bus;            /* IDE bus */
	dev_t devices[2];      /* IDE devices */

	/* Channel IO-ports / memory mappings */
	reg_t base;            /* Command registers access */
	reg_t ctrl;            /* Control resgister access */
	reg_t dma;             /* PCI DMA registers access */

	/* Interrupts handling */
	unsigned char irq;     /* Interrupt number */
	handle_t cond;         /* Interrupt condition variable */
	handle_t inth;         /* Interrupt handle */

	/* Synchronization */
	handle_t lock;         /* Channel access mutex */
};


struct bus_t {
	pci_device_t device;   /* IDE bus device */
	channel_t channels[2]; /* IDE bus channels */

	void *prev, *next;     /* Doubly linked list */
};


#endif
