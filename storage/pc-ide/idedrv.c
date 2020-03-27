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

#include <stdlib.h>
#include <string.h>
#include <sys/list.h>

#include <arch/ia32/io.h>

#include "idedrv.h"


struct {
	bus_t *buses;
} ide_common;


static uint8_t read_portb(uint32_t addr, uint32_t offset)
{
	return inb((void *)(addr + offset));
}


static uint16_t read_portw(uint32_t addr, uint32_t offset)
{
	return inw((void *)(addr + offset))
}


static uint8_t read_memb(uint32_t addr, uint32_t offset)
{
	return *((uint8_t *)(addr + offset));
}


static uint16_t read_memw(uint32_t addr, uint32_t offset)
{
	return *((uint16_t *)(addr + offset));
}


static void write_portb(uint32_t addr, uint32_t offset, uint8_t val)
{
	outb((void *)(addr + offset), val);
}


static void write_portw(uint32_t addr, uint32_t offset, uint16_t val)
{
	outw((void *)(addr + offset), val)
}


static void write_memb(uint32_t addr, uint32_t offset, uint8_t val)
{
	*((uint8_t *)(addr + offset)) = val;
}


static void write_memw(uint32_t addr, uint32_t offset, uint16_t val)
{
	*((uint16_t *)(addr + offset)) = val;
}


static void detect_reg(reg_t *reg, pci_resource_t *res)
{
	if (res->flags & 0x1)
		*reg = { res->base, read_portb, write_portb, read_portw, write_portw };
	else
		*reg = { res->base, read_memb, write_memb, read_memw, write_memw };
}


int ide_init(uint8_t mode)
{
	static pci_id_t pci_ide[] = {
		{ PCI_ANY, PCI_ANY, PCI_ANY, PCI_ANY, PCI_STORAGE_IDE}
	};

	platformctl_t pctl = { 0 };
	bus_t *bus;
	int i, devices = 0;

	memset(&idedrv_common, 0, sizeof(idedrv_common));
	pctl.action = pctl_get;
	pctl.type = pctl_pci;

	/* Find IDE bus devices */
	for (i = 0; i < sizeof(pci_ide) / sizeof(pci_id_t); i++) {
		pctl.pci.id = pci_ide[i];

		if (platformctl(&pctl) == EOK) {
			if (!(bus = malloc(sizeof(bus_t))))
				return -ENOMEM;

			bus->device = pctl.pci.dev;
			bus->channels[0].bus = bus;
			bus->channels[1].bus = bus;

			/* Detect the I/O address ranges */
			if (bus->device.progif & 0x1) {
				/* Primary channel not in compatibility mode */
				detect_reg(&(bus->channels[0].base), bus->device.resources);
				detect_reg(&(bus->channels[0].ctrl), bus->device.resources + 1);
				bus->channels[0].irq = bus->device.irq;
			}
			else {
				/* Primary channel compatibility mode */
				bus->channels[0].base = { PRIMARY_BASE, read_portb, write_portb, read_portw, write_portw };
				bus->channels[0].ctrl = { PRIMARY_CTRL, read_portb, write_portb, read_portw, write_portw };
				bus->channels[0].irq = PRIMARY_IRQ;
			}
			detect_reg(&(bus->channels[0].dma), bus->device.resources + 4);

			if (bus->device.progif & 0x4) {
				/* Secondary channel not in compatibility mode */
				detect_reg(&(bus->channels[1].base), bus->device.resources + 2);
				detect_reg(&(bus->channels[1].ctrl), bus->device.resources + 3);
				bus->channels[1].irq = bus->device.irq;
			}
			else {
				/* Secondary channel compatibility mode */
				bus->channels[1].base = { SECONDARY_BASE, read_portb, write_portb, read_portw, write_portw };
				bus->channels[1].ctrl = { SECONDARY_CTRL, read_portb, write_portb, read_portw, write_portw };
				bus->channels[1].irq = SECONDARY_IRQ;
			}
			detect_reg(&(bus->channels[1].dma), bus->device.resources + 4);
		}
	}
}
