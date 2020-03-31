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

#ifndef _PC_ATASRV_H_
#define _PC_ATASRV_H_

#include <sys/types.h>

#include <phoenix/types.h>


int atasrv_read(id_t *devId, offs_t offs, char *buff, size_t len, int *err);


int atasrv_write(id_t *devId, offs_t offs, const char *buff, size_t len, int *err);


#endif
