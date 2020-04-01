/* Do zrobienia: kody bledow */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/msg.h>
#include <unistd.h> /* For usleep */
#include <sys/stat.h>
#include <sys/file.h>
#include <ecspi.h>

#include "ecspi-server.h"

unsigned int ecspi_port;

static struct {
	uint8_t devEnabled[4];
	uint8_t contextRdy[4];
	handle_t cond[4];
	handle_t irqLock;
	ecspi_ctx_t ctx[4];
/*	uint8_t periodicData[ECSPI_MAX_DATA]; */
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

    /* ecspi */

	msg.type = mtCreate;
	msg.i.create.dir = dir;
	msg.i.create.type = otDev;
	msg.i.create.mode = 0;
	msg.i.create.dev.port = ecspi_port;
	msg.i.create.dev.id = 1;
	msg.i.data = "ecspi";
	msg.i.size = strlen("ecspi") + 1;
	msg.o.data = NULL;
	msg.o.size = 0;

	if (msgSend(dir.port, &msg) < 0 || msg.o.create.err != EOK)
		return - 1;

	return 0;
}


static int configureDev(msg_t *msg)
{
	dev_ctl_msg_t *imsg;
	imsg = (dev_ctl_msg_t *)msg->i.raw;
printf("es: Received dev cfg msg.\n");
printf("es: Values acquired:\n");
printf("dev_no: %d   ", imsg->dev_no);
printf("chan_msk: %d   ", imsg->chan_msk);
printf("pre: %d   ", imsg->pre);
printf("post: %d   ", imsg->post);
printf("delayCS: %d   ", imsg->delayCS);
printf("delaySS: %d\n", imsg->delaySS);
	msg->o.io.err = -1;
	if (ecspi_init(imsg->dev_no, imsg->chan_msk) < 0)
		return -1;
	if (ecspi_setClockDiv(imsg->dev_no, imsg->pre, imsg->post) < 0)
		return -1;
	if (ecspi_setCSDelay(imsg->dev_no, imsg->delayCS) < 0)
		return -1;
	if (ecspi_setSSDelay(imsg->dev_no, imsg->delaySS) < 0)
		return -1;

	ecspisrv_common.devEnabled[imsg->dev_no - 1] = 1;

	msg->o.io.err = EOK;
printf("Dev cfg successful\n");
	return 0;
} 


static int configureChan(msg_t *msg)
{
	chan_ctl_msg_t *imsg;
	imsg = (chan_ctl_msg_t *)msg->i.raw;
printf("es: Received chan cfg msg. dev: %d, chan: %d mode: %d\n",imsg->dev_no, imsg->chan, imsg->mode);
	if (ecspi_setMode(imsg->dev_no, imsg->chan, imsg->mode) < 0) {
		msg->o.io.err = -1;
		return -1;
	}

	msg->o.io.err = EOK;
printf("Chan cfg successful\n");
	return 0;
} 


static int selectChan(msg_t *msg)
{
	chan_ctl_msg_t *imsg;
	imsg = (chan_ctl_msg_t *)msg->i.raw;

printf("es: Received chan select msg. dev_no: %d, chan: %d\n", imsg->dev_no, imsg->chan);
	if (ecspi_setChannel(imsg->dev_no, imsg->chan) < 0) {
		msg->o.io.err = -1;
		return -1;
	}

	msg->o.io.err = EOK;
printf("Chan select successful\n");
	return 0;
} 


static int exchange(msg_t *msg)
{

	exchange_msg_t *imsg;
	imsg = (exchange_msg_t *)msg->i.raw;
	
	if (!ecspisrv_common.devEnabled[imsg->dev_no - 1]) {
		msg->o.io.err = EPERM;
		return -1;
	}

printf("es: Received exchange msg. dev_no: %d, len: %d\n", imsg->dev_no, imsg->len);
printf("Data from client: [0]: %x, [1]: %x, [2]: %x\n", imsg->data[0], imsg->data[1], imsg->data[2]);
	if (msg->type == mtExchange) {
printf("BLOCKING\n");
		if (ecspi_exchange(imsg->dev_no, imsg->data, msg->o.raw, imsg->len)< 0) {
			msg->o.io.err = -1;
			return -1;
		}
	}
	else {
printf("BUSY\n");
		if (ecspi_exchangeBusy(imsg->dev_no, imsg->data, msg->o.raw, imsg->len)< 0) {
			msg->o.io.err = -1;
			return -1;
		}
	}
printf("Data from ecspi: [0]: %x, [1]: %x, [2]: %x\n", msg->o.raw[0], msg->o.raw[1], msg->o.raw[2]);
	return 0;
} 


static void regContext(int dev_no)
{
printf("First use of async communication for dev_no: %d. Registering context\n", dev_no);
	condCreate(&ecspisrv_common.cond[dev_no - 1]);
	ecspi_registerContext(dev_no, &ecspisrv_common.ctx[dev_no - 1], ecspisrv_common.cond[dev_no - 1]);
	ecspisrv_common.contextRdy[dev_no - 1] = 1;
printf("Context registered. cond: %d, dev_no in ctx: %d\n", ecspisrv_common.cond[dev_no - 1], ecspisrv_common.ctx[dev_no - 1].dev_no);
	return;
}


static int wrtieExchangeAsync(msg_t *msg)
{
	int dev_no;
	exchange_msg_t *imsg;

	imsg = (exchange_msg_t *)msg->i.raw;
	dev_no = imsg->dev_no;
printf("es: Received async msg. dev_no: %d, len: %d, data(3 bytes): %x %x %x\n", imsg->dev_no, imsg->len, imsg->data[0], imsg->data[1], imsg->data[2]);
	if (!ecspisrv_common.contextRdy[dev_no - 1])
		regContext(dev_no);

	if (msg->type == mtWriteAsync) {
printf("Writing\n");
		ecspi_writeAsync(&ecspisrv_common.ctx[dev_no - 1], imsg->data, imsg->len);
	}
	else {
printf("ExchangeAsync\n");
		ecspi_exchangeAsync(&ecspisrv_common.ctx[dev_no - 1], imsg->data, imsg->len);
	}

	msg->o.io.err = EOK;
	return 0;
}


static int readAsync(msg_t *msg)
{
	int dev_no;
	exchange_msg_t *imsg;

	imsg = (exchange_msg_t *)msg->i.raw;
	dev_no = imsg->dev_no;
printf("es: Received async READ msg. dev_no: %d, len: %d\n", imsg->dev_no, imsg->len);
	if (!ecspisrv_common.contextRdy[dev_no - 1]) {
printf("Context not registered.\n");
		msg->o.io.err = EPERM;
		return -1;
	}
printf("!!!!!! MUTEX COMMENTED OUT BECAUSE NO DEVICE WAS AVAILABLE IN TESTING\n");
/* COMMENTED OUT BECAUSE WITHOUT DEVICE IT NEVER GOES THROUGH
	mutexLock(ecspisrv_common.irqLock);
	while (ecspisrv_common.ctx[dev_no - 1].rx_count < sizeof(imsg->data)) {
		condWait(ecspisrv_common.cond[dev_no - 1], ecspisrv_common.irqLock, 0);
	}
	mutexUnlock(ecspisrv_common.irqLock);
 */
	ecspi_readFifo(&ecspisrv_common.ctx[dev_no - 1], msg->o.raw, imsg->len);
printf("Data from ecspi: [0]: %x, [1]: %x, [2]: %x\n", msg->o.raw[0], msg->o.raw[1], msg->o.raw[2]);

	msg->o.io.err = EOK;
	return 0;
}
/*
static int handler(const uint8_t *rx, size_t len, uint8_t *out)
{
	memcpy(ecspisrv_common.periodicData, rx, len);
	in[rx_len] = rx[1];
	out[2] += 1;
	rx_len += 1;

	return (rx_len < 12) ? len : -1;
}

static int exchangePeriodically(msg_t *msg)
{
	if (!ecspisrv_common.contextRdy[dev_no - 1])
		regContext(dev_no);
	return 0;
}
*/

static void msgProcessing(void)
{
    unsigned int rid;
    msg_t msg;

    while (1) {
		while (msgRecv(ecspi_port, &msg, &rid) < 0)
			;

		switch (msg.type) {
			case mtDevCtl:
				configureDev(&msg);
				break;

			case mtChanCtl:
				configureChan(&msg);
				break;

			case mtChanSelect:
				selectChan(&msg);
				break;

			case mtExchange:
			case mtExchangeBusy:
				exchange(&msg);
				break;

			case mtWriteAsync:
			case mtExchangeAsync:
				wrtieExchangeAsync(&msg);
				break;

			case mtReadAsync:
				readAsync(&msg);
				break;

/*			case mtExchangePeriodically:
				exchangePeriodically(&msg);
*/
			case mtRead:
			case mtWrite:
			case mtGetAttr:
			case mtSetAttr:
			case mtOpen:
			case mtClose:
				msg.o.io.err = EOK;	
/* Dlaczego tu jest brak errora ale tez brak akcji? */
				break;
			case mtCreate:
			case mtTruncate:
			case mtDestroy:
			case mtLookup:
			case mtLink:
			case mtUnlink:
			case mtReaddir:
			default:
				msg.o.io.err = -ENOSYS;
				break;
		}

		msgRespond(ecspi_port, &msg, rid);
	}
}


int main(int argc, char **argv)
{
printf("ecspi-server: starting\n");

	mutexCreate(&ecspisrv_common.irqLock);
    portCreate(&ecspi_port);

    if (createDevFiles() < 0) {
		printf("ecspi-server: createSpecialFiles failed\n");
		return -1;
	}
printf("Kompilacja sie udala 29\n");
	msgProcessing();

    return 0;
}