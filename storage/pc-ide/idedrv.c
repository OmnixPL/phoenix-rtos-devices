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
#include <unistd.h>
#include <sys/list.h>

#include <arch/ia32/io.h>

#include "idedrv.h"


struct {
	unsigned int devices;
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


static uint8_t read_status(channel_t *channel)
{
	return channel->base.readb(channel->base.addr, REG_STATUS);
}


static uint8_t read_altstatus(channel_t *channel)
{
	return channel->ctrl.readb(channel->ctrl.addr, REG_CTRL);
}


static void write_cmd(channel_t *channel, uint8_t cmd)
{
	channel->base.writeb(channel->base.addr, REG_STATUS, cmd);
}


static void delay_400ns(channel_t *channel)
{
	read_altstatus(channel);
	read_altstatus(channel);
	read_altstatus(channel);
	read_altstatus(channel);
}


static void soft_reset(channel_t *channel)
{
	/* Software reset all drives on the bus */
	channel->ctrl.writeb(channel->ctrl.addr, REG_CTRL, CTRL_SOFTRST | CTRL_NIEN);
	/* Short delay */
	usleep(10);
	/* Reset the bus to normal operation */
	channel->ctrl.writeb(channel->ctrl.addr, REG_CTRL, CTRL_NIEN);
	/* Short delay */
	usleep(10);
}


static void enable_irq(channel_t *channel)
{
	channel->ctrl.writeb(channel->ctrl.addr, REG_CTRL, 0);
}


static void disable_irq(channel_t *channel)
{
	channel->ctrl.writeb(channel->ctrl.addr, REG_CTRL, CTRL_NIEN);
}


static uint16_t read_data(channel_t *channel)
{
	uint16_t data = channel->base.readw(channel->base.addr, REG_DATA);
	return ((data & 0xFF) << 8) | ((data >> 8) & 0xFF);
}


static uint8_t read_error(channel_t *channel)
{
	return channel->base.readb(channel->base.addr, REG_ERROR);
}


static uint8_t read_secnum(channel_t *channel)
{
	return channel->base.readw(channel->base.addr, REG_SECNUM);
}


static uint8_t read_sec(channel_t *channel)
{
	return channel->base.readw(channel->base.addr, REG_SEC);
}


static uint8_t read_lcyl(channel_t *channel)
{
	return channel->base.readw(channel->base.addr, REG_LCYL);
}


static uint8_t read_hcyl(channel_t *channel)
{
	return channel->base.readw(channel->base.addr, REG_HCYL);
}


static uint8_t read_head(channel_t *channel)
{
	return channel->base.readw(channel->base.addr, REG_HEAD) & 0x0F;
}


static void select_device(channel_t *channel, uint8_t devnum, uint16_t secnum, uint64_t lba, uint8_t mode)
{
	dev_t *device = channel->devices + devnum;
	uint8_t devsel = DEVSEL_SET0 | DEVSEL_SET1;
	uint32_t c = 0, h = 0, s = 0;

	/* Select device: master or slave */
	if (devnum)
		devsel |= DEVSEL_DEVNUM;

	/* Select addressing mode */
	switch (mode) {
	case AMODE_CHS:
		if (lba != -1) {
			/* Convert LBA to CHS */
			c = lba / ((uint64_t)(device->heads) * device->sectors);
			h = (lba / device->sectors) % device->heads;
			s = (lba % device->sectors) + 1;
		}
		devsel |= h & DEVSEL_HEAD;

		channel->base.writeb(channel->base.addr, REG_SECNUM, secnum & 0xFF);
		channel->base.writeb(channel->base.addr, REG_SEC, s & 0xFF);
		channel->base.writeb(channel->base.addr, REG_LCYL, c & 0xFF);
		channel->base.writeb(channel->base.addr, REG_HCYL, (c >> 8) & 0xFF);
		break;

	case AMODE_LBA28:
		devsel |= DEVSEL_LBA | ((lba >> 24) & DEVSEL_HEAD);

		channel->base.writeb(channel->base.addr, REG_SECNUM, secnum & 0xFF);
		channel->base.writeb(channel->base.addr, REG_SEC, lba & 0xFF);
		channel->base.writeb(channel->base.addr, REG_LCYL, (lba >> 8) & 0xFF);
		channel->base.writeb(channel->base.addr, REG_HCYL, (lba >> 16) & 0xFF);
		break;

	case AMODE_LBA48:
		devsel |= DEVSEL_LBA;

		channel->base.writeb(channel->base.addr, REG_SECNUM, (secnum >> 8) & 0xFF);
		channel->base.writeb(channel->base.addr, REG_SECNUM, secnum & 0xFF);
		channel->base.writeb(channel->base.addr, REG_SEC, (lba >> 24) & 0xFF);
		channel->base.writeb(channel->base.addr, REG_SEC, lba & 0xFF);
		channel->base.writeb(channel->base.addr, REG_LCYL, (lba >> 32) & 0xFF);
		channel->base.writeb(channel->base.addr, REG_LCYL, (lba >> 8) & 0xFF);
		channel->base.writeb(channel->base.addr, REG_HCYL, (lba >> 40) & 0xFF);
		channel->base.writeb(channel->base.addr, REG_HCYL, (lba >> 16) & 0xFF);
		break;
	}

	/* Select the device */
	channel->base.writeb(channel->base.addr, REG_DEVSEL, devsel);
	/* Wait for the device to push its status onto the bus */
	delay_400ns(channel);
	/* Write features */
	channel->base.writeb(channel->base.addr, REG_FEATURES, 0);
}


static void wait_bsy(channel_t *channel)
{
	while (read_status(channel) & STATUS_BSY);
}


static int wait_drq(channel_t *channel)
{
	uint8_t status;

	while (!((status = read_status(channel)) & STATUS_DRQ)) {
		if (status & STATUS_ERR)
			return -1;
	}

	return 0;
}


static void wait_rdy(channel_t *channel)
{
	uint8_t status;

	do {
		status = read_status(channel);
	} while ((status & STATUS_DRQ) || !(status & STATUS_RDY));
}


int ide_init(uint8_t mode)
{
	static pci_id_t pci_ide[] = {
		{ PCI_ANY, PCI_ANY, PCI_ANY, PCI_ANY, PCI_STORAGE_IDE}
	};

	platformctl_t pctl = { 0 };
	bus_t *bus;
	channel_t *channel;
	dev_t *device;
	uint8_t lcyl, hcyl, *info;
	int i, j, k, l;

	memset(&ide_common, 0, sizeof(idedrv_common));
	pctl.action = pctl_get;
	pctl.type = pctl_pci;

	if (!(info = malloc(512)))
		return -ENOMEM;

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
		
			/* Loop over channels */
			for (j = 0; j < 2; j++) {
				channel = bus->channels + j;
				channel->devices[0].exists = 0;
				channel->devices[1].exists = 0;

				/* Floating bus check */
				if (read_status(channel) == 0xFF)
					continue;

				/* Software reset the bus */
				soft_reset(channel);

				/* Loop over attached devices */
				for (k = 0; k < 2; k++) {
					device = channel->devices + k;

					/* Select the device */
					select_device(channel, k, 0, -1, AMODE_CHS);

					/* Floating bus check */
					if (read_status(channel) == 0xFF)
						continue;

					/* Send identify command */
					channel->base.writeb(channel->base.addr, REG_CMD, CMD_IDENTIFY);

					if (!read_status(channel))
						continue;

					/* Wait while busy */
					wait_bsy(channel);

					if (!(lcyl = read_lcyl(channel)) && !(hcyl = read_hcyl(channel)) && !wait_drq(channel)) {
						device->type = ATA;
					}
					else if ((lcyl == 0x14 && hcyl == 0xEB) || (lcyl == 0x69 && hcyl == 0x96)) {
						device->type = ATAPI;

						/* Send identify packet command */
						channel->base.writeb(channel->base.addr, REG_STATUS, CMD_IDENTIFY_PACKET);

						/* Wait while busy */
						wait_bsy(channel);

						if (wait_drq(channel))
							continue;
					}
					else {
						/* Neither ATA nor ATAPI */
						continue;
					}

					/* Read device identification */
					for (l = 0; l < 256; l++)
						((uint16_t *)info)[l] = read_data(channel);

					wait_rdy(channel);

					device->exists = 1;
					device->dma = info[98] & 1;

					if (device->type == ATA) {
						device->mode = ((info[98] >> 1) & 1) + ((info[166] >> 2) & 1);
						device->cylinders = (info[3]  << 0) | (info[2]  << 8);
						device->heads     = (info[7]  << 0) | (info[6]  << 8);
						device->sectors   = (info[13] << 0) | (info[12] << 8);

						switch (device->mode) {
						case AMODE_CHS:
							device->size =
								(uint64_t)(device->cylinders) *
								(uint64_t)(device->heads) *
								(uint64_t)(device->sectors);
							break;

						case AMODE_LBA28:
							device->size =
								(info[121] <<  0)|
								(info[120] <<  8)|
								(info[123] << 16)|
								(info[122] << 24);
							break;

						case AMODE_LBA48:
							device->size =
								(((uint64_t)info[201]) <<  0)|
								(((uint64_t)info[200]) <<  8)|
								(((uint64_t)info[203]) << 16)|
								(((uint64_t)info[202]) << 24)|
								(((uint64_t)info[205]) << 32)|
								(((uint64_t)info[204]) << 40)|
								(((uint64_t)info[207]) << 48)|
								(((uint64_t)info[206]) << 56);
							break;
						}
						device->size *= 512;
					}
					ide_common.devices++;
				}
			}
			LIST_ADD(&ide_common.buses, bus);
		}
	}
	free(info);

	return EOK;
}
