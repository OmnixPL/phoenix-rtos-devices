#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/msg.h>
#include <unistd.h> /* For usleep */
#include <sys/stat.h>
#include <sys/file.h>
#include <ecspi.h>

#include "ecspi-server.h"

#define MODULE_NAME "ecspi-server"
#define LOG_ERROR(str, ...) do { if (1) fprintf(stdout, MODULE_NAME ": ERROR: " str "\n", ##__VA_ARGS__); } while (0)
#define TRACE(str, ...) do { if (1) fprintf(stdout, MODULE_NAME ": trace: " str "\n", ##__VA_ARGS__); } while (0)
#define ID_TO_INDEX(x) (x-1)

#define THREADS_PRIORITY 2
#define STACKSZ 512
#define SPI_THREADS_NO 4
#define BURSTSZ 256
#define RETRIES_MAX 10

enum { spi1 = 1, spi2, spi3, spi4 };

static struct {
	uint8_t devEnabled;
	uint8_t contextRdy;
	handle_t cond;
	handle_t irqLock;
	ecspi_ctx_t ctx;
} ecspisrv_dev_common[4];

static struct {
	unsigned int ecspi_port;
	char stack[SPI_THREADS_NO][STACKSZ] __attribute__ ((aligned(8))); /* should be THREADS - 1, but see main */
} ecspisrv_common;


static int createDevFiles(void)
{
    int err;
	oid_t dir;
    msg_t msg;

	while (lookup("/", NULL, &dir) < 0)
		usleep(100000);

	/* /dev */

	err = mkdir("/dev", 0);

	if (err < 0 && err != -EEXIST)
		return -1;

	if (lookup("/dev", NULL, &dir) < 0)
		return -1;

    /* ecspis */

	msg.type = mtCreate;
	msg.i.create.dir = dir;
	msg.i.create.type = otDev;
	msg.i.create.mode = 0;
	msg.i.create.dev.port = ecspisrv_common.ecspi_port;
	msg.i.create.dev.id = spi1;
	msg.i.data = "spi1";
	msg.i.size = strlen(msg.i.data) + 1;
	msg.o.data = NULL;
	msg.o.size = 0;
	if (msgSend(dir.port, &msg) < 0 || msg.o.create.err != EOK)
		return - 1;

	msg.i.create.dev.id = spi2;
	msg.i.data = "spi2";
	msg.i.size = strlen(msg.i.data) + 1;
	if (msgSend(dir.port, &msg) < 0 || msg.o.create.err != EOK)
		return - 1;

	msg.i.create.dev.id = spi3;
	msg.i.data = "spi3";
	msg.i.size = strlen(msg.i.data) + 1;
	if (msgSend(dir.port, &msg) < 0 || msg.o.create.err != EOK)
		return - 1;

	msg.i.create.dev.id = spi4;
	msg.i.data = "spi4";
	msg.i.size = strlen(msg.i.data) + 1;
	if (msgSend(dir.port, &msg) < 0 || msg.o.create.err != EOK)
		return - 1;

	return 0;
}


static int configureDev(msg_t *msg)
{
	spi_i_devctl_t *imsg;
	int id;
	imsg = (spi_i_devctl_t *)msg->i.raw;
	id = imsg->id;
TRACE("dev msg   id: %d   chan_msk: %d   pre: %d   post: %d   delayCS: %d   delaySS: %d", id, imsg->dev.chan_msk, imsg->dev.pre, imsg->dev.post, imsg->dev.delayCS, imsg->dev.delaySS);
	msg->o.io.err = -EIO;

	mutexLock(ecspisrv_dev_common[ID_TO_INDEX(id)].irqLock);
	if (ecspi_init(imsg->id, imsg->dev.chan_msk) < 0) {
		mutexUnlock(ecspisrv_dev_common[ID_TO_INDEX(id)].irqLock);
		return -1;
	}
	if (ecspi_setClockDiv(imsg->id, imsg->dev.pre, imsg->dev.post) < 0) {
		mutexUnlock(ecspisrv_dev_common[ID_TO_INDEX(id)].irqLock);
		return -1;
	}
	if (ecspi_setCSDelay(imsg->id, imsg->dev.delayCS) < 0) {
		mutexUnlock(ecspisrv_dev_common[ID_TO_INDEX(id)].irqLock);
		return -1;
	}
	if (ecspi_setSSDelay(imsg->id, imsg->dev.delaySS) < 0) {
		mutexUnlock(ecspisrv_dev_common[ID_TO_INDEX(id)].irqLock);
		return -1;
	}
	mutexUnlock(ecspisrv_dev_common[ID_TO_INDEX(id)].irqLock);
	
	ecspisrv_dev_common[ID_TO_INDEX(imsg->id)].devEnabled = 1;

	msg->o.io.err = EOK;
TRACE("dev cfg successful");
	return 0;
} 


static int configureChan(msg_t *msg)
{
	spi_i_devctl_t *imsg;
	int id;
	imsg = (spi_i_devctl_t *)msg->i.raw;
	id = imsg->id;
TRACE("chan cfg msg   id: %d   chan: %d   mode: %d", imsg->id, imsg->chan.chan, imsg->chan.mode);
	
	mutexLock(ecspisrv_dev_common[ID_TO_INDEX(id)].irqLock);
	if (ecspi_setMode(imsg->id, imsg->chan.chan, imsg->chan.mode) < 0) {
		msg->o.io.err = -EIO;
		mutexUnlock(ecspisrv_dev_common[ID_TO_INDEX(id)].irqLock);
		return -1;
	}
	mutexUnlock(ecspisrv_dev_common[ID_TO_INDEX(id)].irqLock);

	msg->o.io.err = EOK;
TRACE("chan cfg successful");
	return 0;
} 


static int selectChan(msg_t *msg)
{
	spi_i_devctl_t *imsg;
	int id;
	imsg = (spi_i_devctl_t *)msg->i.raw;
	id = imsg->id;

TRACE("chan select msg   id: %d   chan: %d", imsg->id, imsg->chan.chan);

	mutexLock(ecspisrv_dev_common[ID_TO_INDEX(id)].irqLock);
	if (ecspi_setChannel(imsg->id, imsg->chan.chan) < 0) {
		msg->o.io.err = -1;
		mutexUnlock(ecspisrv_dev_common[ID_TO_INDEX(id)].irqLock);
		return -1;
	}
	mutexUnlock(ecspisrv_dev_common[ID_TO_INDEX(id)].irqLock);

	msg->o.io.err = EOK;
TRACE("chan select successful");
	return 0;
} 


static int exchange(msg_t *msg)
{
	spi_i_devctl_t *imsg;
	int id;
	imsg = (spi_i_devctl_t *)msg->i.raw;
	id = imsg->id;
	uint8_t data[BURSTSZ];

	mutexLock(ecspisrv_dev_common[ID_TO_INDEX(id)].irqLock);
	if (!ecspisrv_dev_common[ID_TO_INDEX(imsg->id)].devEnabled) {
		msg->o.io.err = -ECONNREFUSED;
		mutexUnlock(ecspisrv_dev_common[ID_TO_INDEX(id)].irqLock);
		return -1;
	}

TRACE("exchange msg   id: %d   len: %d\n"
"	data from client: %x %x %x", imsg->id, msg->i.size, *(uint8_t *)msg->i.data, *((uint8_t *)msg->i.data + 1), *((uint8_t *)msg->i.data + 2));
	if (imsg->type == spi_exchange) {
TRACE("BLOCKING");
		if (ecspi_exchange(imsg->id, msg->i.data, data, msg->i.size) < 0) {
			msg->o.io.err = -EIO;
			mutexUnlock(ecspisrv_dev_common[ID_TO_INDEX(id)].irqLock);
			return -1;
		}
	}
	else {
TRACE("BUSY");
		if (ecspi_exchangeBusy(imsg->id, msg->i.data, data, msg->i.size)< 0) {
			msg->o.io.err = -EIO;
			mutexUnlock(ecspisrv_dev_common[ID_TO_INDEX(id)].irqLock);
			return -1;
		}
	}
	mutexUnlock(ecspisrv_dev_common[ID_TO_INDEX(id)].irqLock);
	
	msg->o.data = data;
	msg->o.size = msg->i.size;
TRACE("data from ecspi (first 3 bytes): %x %x %x", *((uint8_t *)msg->o.data), *((uint8_t *)msg->o.data + 1), *((uint8_t *)msg->o.data + 2));
	return 0;
} 


static void regContext(int id)
{
	int i;
	i = ID_TO_INDEX(id);
TRACE("First use of async communication for dev_no: %d. Registering context..", id);
	condCreate(&ecspisrv_dev_common[i].cond);
	ecspi_registerContext(id, &ecspisrv_dev_common[i].ctx, ecspisrv_dev_common[i].cond);
	ecspisrv_dev_common[i].contextRdy = 1;
TRACE("context registered   cond: %d, dev_no in ctx: %d", ecspisrv_dev_common[i].cond, ecspisrv_dev_common[i].ctx.dev_no);
	return;
}


static int wrtieExchangeAsync(msg_t *msg)
{
	int id;
	spi_i_devctl_t *imsg;

	imsg = (spi_i_devctl_t *)msg->i.raw;
	id = imsg->id;

	mutexLock(ecspisrv_dev_common[ID_TO_INDEX(id)].irqLock);
	if (!ecspisrv_dev_common[ID_TO_INDEX(id)].contextRdy)
		regContext(id);

	if (imsg->type == spi_writeAsync) {
TRACE("writeAsync msg   id: %d   len: %d   data(first 3 bytes): %x %x %x", imsg->id, msg->i.size, *(uint8_t *)msg->i.data, *((uint8_t *)msg->i.data + 1), *((uint8_t *)msg->i.data + 2));
		ecspi_writeAsync(&ecspisrv_dev_common[ID_TO_INDEX(id)].ctx, msg->i.data, msg->i.size);
	}
	else {
TRACE("exchangeAsync msg   id: %d   len: %d   data(first 3 bytes): %x %x %x", imsg->id, msg->i.size, *(uint8_t *)msg->i.data, *((uint8_t *)msg->i.data + 1), *((uint8_t *)msg->i.data + 2));
		ecspi_exchangeAsync(&ecspisrv_dev_common[ID_TO_INDEX(id)].ctx, msg->i.data, msg->i.size);
	}
	mutexUnlock(ecspisrv_dev_common[ID_TO_INDEX(id)].irqLock);

	msg->o.io.err = EOK;
	return 0;
}

static int readAsync(msg_t *msg)
{
	int id;
	int retries;
	spi_i_devctl_t *imsg;
	uint8_t data[BURSTSZ];

	retries = 0;
	imsg = (spi_i_devctl_t *)msg->i.raw;
	id = imsg->id;
TRACE("readAsync msg   id: %d   len: %d", id, msg->i.size);

	mutexLock(ecspisrv_dev_common[ID_TO_INDEX(id)].irqLock);
	if (!ecspisrv_dev_common[ID_TO_INDEX(id)].contextRdy) {
		LOG_ERROR("context not registered");
		msg->o.io.err = -ECONNREFUSED;
		mutexUnlock(ecspisrv_dev_common[ID_TO_INDEX(id)].irqLock);
		return -1;
	}
printf("ecspisrv_dev_common[ID_TO_INDEX(id)].ctx.rx_count: %d\n", ecspisrv_dev_common[ID_TO_INDEX(id)].ctx.rx_count);
printf("msg->i.size: %d\n", msg->i.size);
printf("ecspisrv_dev_common[ID_TO_INDEX(id)].ctx.rx_count < msg->i.size: %d\n", (ecspisrv_dev_common[ID_TO_INDEX(id)].ctx.rx_count < msg->i.size));
printf("(retries < RETRIES_MAX): %d\n", (retries < RETRIES_MAX));
	while ( (ecspisrv_dev_common[ID_TO_INDEX(id)].ctx.rx_count < msg->i.size) && (retries < RETRIES_MAX)) {
		condWait(ecspisrv_dev_common[ID_TO_INDEX(id)].cond, ecspisrv_dev_common[ID_TO_INDEX(id)].irqLock, 5);
		TRACE("readAsync: retry %d", retries);
		++retries;
	}

	ecspi_readFifo(&ecspisrv_dev_common[ID_TO_INDEX(id)].ctx, data, msg->i.size);
	mutexUnlock(ecspisrv_dev_common[ID_TO_INDEX(id)].irqLock);
	msg->o.data = data;
	msg->o.size = msg->i.size;
TRACE("data from ecspi (first 3 bytes): %x %x %x", *((uint8_t *)msg->o.data), *((uint8_t *)msg->o.data + 1), *((uint8_t *)msg->o.data + 2));

	msg->o.io.err = EOK;
	if(retries == RETRIES_MAX){
		LOG_ERROR("readAsync: read failed");
		msg->o.io.err = -EIO;	
	}
	return 0;
}


static int processMsg(msg_t *msg)
{
	spi_i_devctl_t *imsg;
	imsg = (spi_i_devctl_t *)msg->i.raw;
	
	switch (imsg->type) {
		case spi_devCtl:
			configureDev(msg);
			break;

		case spi_chanCtl:
			configureChan(msg);
			break;

		case spi_chanSelect:
			selectChan(msg);
			break;

		case spi_exchange:
		case spi_exchangeBusy:
			exchange(msg);
			break;

		case spi_readAsync:
			readAsync(msg);
			break;
			
		case spi_exchangeAsync:
		case spi_writeAsync:
			wrtieExchangeAsync(msg);
			break;
	}

	return 0;
}


static void dispatchMsg(int i)
{
    unsigned int rid;
    msg_t msg;
TRACE("dispatch msg thread no %d", i);
    while (1) {
		while (msgRecv(ecspisrv_common.ecspi_port, &msg, &rid) < 0)
			;
TRACE("msg recv by thread %d", i);
		switch (msg.type) {
			case mtDevCtl:
				processMsg(&msg);
				break;

			case mtRead:
			case mtWrite:
			case mtGetAttr:
			case mtSetAttr:
			case mtOpen:
			case mtClose:
			case mtCreate:
			case mtTruncate:
			case mtDestroy:
			case mtLookup:
			case mtLink:
			case mtUnlink:
			case mtReaddir:
			default:
				LOG_ERROR("wrong msg type");
				msg.o.io.err = -ENOSYS;
				break;
		}

		msgRespond(ecspisrv_common.ecspi_port, &msg, rid);
	}
}


int main(int argc, char **argv)
{
	int i;

TRACE("starting");
printf("Kompilacja sie udala 44\n");
	for (i = 0; i < 4; ++i)
		mutexCreate(&ecspisrv_dev_common[i].irqLock);

    portCreate(&ecspisrv_common.ecspi_port);
    if (createDevFiles() < 0) {
		LOG_ERROR("creating spi files failed");
		return -1;
	}
	
	for (i = 1; i < SPI_THREADS_NO - 1; ++i) /* for some reason it doesnt work when i = 0 */
		beginthread(dispatchMsg, THREADS_PRIORITY, ecspisrv_common.stack[i], STACKSZ, i);

	dispatchMsg(i);

    return 0;
}