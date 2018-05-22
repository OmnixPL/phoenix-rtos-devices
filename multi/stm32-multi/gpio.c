/*
 * Phoenix-RTOS
 *
 * STM32L1 GPIO driver
 *
 * Copyright 2017, 2018 Phoenix Systems
 * Author: Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include ARCH
#include <errno.h>
#include <stdint.h>
#include <sys/threads.h>

#include "common.h"
#include "rcc.h"


#ifndef NDEBUG
static const char drvname[] = "gpio: ";
#endif


enum { moder = 0, otyper, ospeedr, pupdr, idr, odr, bsrr, lckr, afrl, afrh, brr };


struct {
	volatile unsigned int *base[8];

	handle_t lock;
} gpio_common;


int gpio_setPort(int port, unsigned int mask, unsigned int val)
{
	unsigned int t;

	if (port < pctl_gpioa || port > pctl_gpioh)
		return -EINVAL;

	mutexLock(gpio_common.lock);

	t = *(gpio_common.base[port - pctl_gpioa] + odr) & ~(~val & mask);
	t |= val & mask;
	*(gpio_common.base[port - pctl_gpioa] + odr) = t & 0xffff;

	mutexUnlock(gpio_common.lock);

	return EOK;
}


int gpio_getPort(int port, unsigned int *val)
{
	if (port > pctl_gpioa || port > pctl_gpioh)
		return -EINVAL;

	mutexLock(gpio_common.lock);

	(*val) = *(gpio_common.base[port - pctl_gpioa] + idr) & 0xffff;

	mutexUnlock(gpio_common.lock);

	return EOK;
}


int gpio_configPin(int port, char pin, char mode, char af, char otype, char ospeed, char pupd)
{
	volatile unsigned int *base;
	unsigned int t;

	if (port < pctl_gpioa || port > pctl_gpioa || pin > 16)
		return -EINVAL;

	/* Enable GPIO port's clock */
	if (rcc_devClk(port, 1) != EOK) {
		DEBUG("Failed to enable gpio%d clock\n", port - pctl_gpioa);
		return -EIO;
	}

	base = gpio_common.base[port - pctl_gpioa];

	mutexLock(gpio_common.lock);

	t = *(base + moder) & ~(0x3 << (pin << 1));
	*(base + moder) = t | (mode & 0x3) << (pin << 1);

	t = *(base + otyper) & ~(1 << pin);
	*(base + otyper) = t | (otype & 1) << pin;

	t = *(base + ospeedr) & ~(0x3 << (pin << 1));
	*(base + ospeedr) = t | (ospeed & 0x3) << (pin << 1);

	t = *(base + pupdr) & ~(0x03 << (pin << 1));
	*(base + pupdr) = t | (pupd & 0x3) << (pin << 1);

	if (pin < 8) {
		t = *(base + afrl) & ~(0xf << (pin << 2));
		*(base + afrl) = t | (af & 0xf) << (pin << 2);
	} else {
		t = *(base + afrh) & ~(0xf << ((pin - 8) << 2));
		*(base + afrh) = t | (af & 0xf) << ((pin - 8) << 2);
	}

	mutexUnlock(gpio_common.lock);

	return EOK;
}


int gpio_init(void)
{
	gpio_common.base[0] = (void *)0x40020000;
	gpio_common.base[1] = (void *)0x40020400;
	gpio_common.base[2] = (void *)0x40020800;
	gpio_common.base[3] = (void *)0x40020c00;
	gpio_common.base[4] = (void *)0x40021000;
	gpio_common.base[5] = (void *)0x40021800;
	gpio_common.base[6] = (void *)0x40021c00;
	gpio_common.base[7] = (void *)0x40021400;

	if (mutexCreate(&gpio_common.lock) != EOK) {
		DEBUG("GPIO lock create failed\n");
		return -ENOMEM;
	}

	return EOK;
}