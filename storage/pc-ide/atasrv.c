/*
 * Phoenix-RTOS
 *
 * PC ATA server
 *
 * Copyright 2019, 2020 Phoenix Systems
 * Author: Kamil Amanowicz, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdint.h>

#include <posix/idtree.h>
#include <phoenix/msg.h>

#include "atasrv.h"
#include "atadrv.h"


typedef struct atasrv_partition_t atasrv_partition_t;


typedef struct {
	idnode_t node;
	uint8_t type;
	union {
		atasrv_partition_t *partition;
		ata_dev_t *ataDev;
	};
} atasrv_device_t;


typedef struct atasrv_filesystem_t atasrv_filesystem_t;


struct atasrv_filesystem_t {
	char name[16];
	uint8_t type;
	atasrv_filesystem_t *next;
	int (*handler)(void *, msg_t *);
	int (*mount)(id_t *, void **);
	int (*unmount)(void *);
};


struct atasrv_partition_t {
	int portfd;
	uint32_t start;
	uint32_t size;
	uint32_t refs;
	uint8_t type;

	atasrv_device_t *dev;
	atasrv_filesystem_t *fs;

	void *fsData;
	char *stack;
};


typedef struct {
	void *next, *prev;
	msg_t msg;
	int portfd;
	unsigned rid;
	atasrv_filesystem_t *fs;
	void *fsData;
} atasrv_request_t;


struct {
	idtree_t devices;
	atasrv_filesystem_t *filesystems;
	atasrv_request_t *queue;
	int portfd;
	int devices_cnt;
	handle_t lock, cond;

	char poolStacks[1][4 * 4096] __attribute__((aligned(8)));
} atasrv_common;
