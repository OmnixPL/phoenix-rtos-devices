
#include <stdio.h>
#include <sys/msg.h>
#include <string.h>

#include "../imx6ull-server/ecspi-server.h"

msg_t msg;

static int devCtlTest(unsigned int port)
{
    dev_ctl_msg_t raw;

    raw.dev_no = 1;
    raw.chan_msk = 3;
    raw.pre = 4;
    raw.post = 6;
    raw.delayCS = 11;
    raw.delaySS = 13;

	msg.type = mtDevCtl;
    memcpy(msg.i.raw, (unsigned char*)&raw, sizeof(raw));

printf("ec: Initalizing device. Sending...\n");
printf("Port: %d, &msg: %p\n", port, &msg);
    msgSend(port, &msg);
printf("ec: Returning error code: >%d<\n\n", msg.o.io.err);

    return 0;
}

int chanCtlTest(unsigned int port)
{  
    chan_ctl_msg_t chan;

    chan.dev_no = 1;
    chan.chan = 1;
    chan.mode = 1;
    
    msg.type = mtChanCtl;
    memcpy(msg.i.raw, (unsigned char*)&chan, sizeof(chan));


printf("ec: Setting mode. Sending...\n");
    msgSend(port, &msg);
printf("ec: Returning error code: >%d<\n\n", msg.o.io.err);

    return 0;
}


int chanSelectTest(unsigned int port)
{   
    msg_t msg;
    chan_ctl_msg_t chan;

    chan.dev_no = 1;
    chan.chan = 1;

    msg.type = mtChanSelect;
    memcpy(msg.i.raw, (unsigned char*)&chan, sizeof(chan));

printf("ec: Selecting channel. Sending...\n");
    msgSend(port, &msg);
printf("ec: Returning error code: >%d<\n\n", msg.o.io.err);

    return 0;
}


int exchangeDataBlockingTest(unsigned int port)
{   
    msg_t msg;
    exchange_msg_t exchange;
    uint8_t data[3] = {0xAE, 0x12, 0x34};

    exchange.dev_no = 1;
    exchange.len = 3;
    memcpy(exchange.data, data, 3);

    msg.type = mtExchange;
    memcpy(msg.i.raw, (unsigned char*)&exchange, sizeof(exchange));

printf("ec: Exchanging data BLOCKING. Sending...\n");
    msgSend(port, &msg);
printf("ec: Returning error code: >%d<\n\n", msg.o.io.err);

    return 0;
}


int exchangeDataBusyTest(unsigned int port)
{   
    msg_t msg;
    exchange_msg_t exchange;
    uint8_t data[3] = {0x13, 0x57, 0x91};

    exchange.dev_no = 1;
    exchange.len = 3;
    memcpy(exchange.data, data, 3);

    msg.type = mtExchangeBusy;
    memcpy(msg.i.raw, (unsigned char*)&exchange, sizeof(exchange));

printf("ec: Exchanging data BUSY. Sending...\n");
    msgSend(port, &msg);
printf("ec: Returning error code: >%d<\n\n", msg.o.io.err);

    return 0;
}


int writeAsyncTest(unsigned int port)
{   
    msg_t msg;
    exchange_msg_t exchange;
    uint8_t data[3] = {0x13, 0x57, 0x91};

    exchange.dev_no = 1;
    exchange.len = 3;
    memcpy(exchange.data, data, 3);

    msg.type = mtWriteAsync;
    memcpy(msg.i.raw, (unsigned char*)&exchange, sizeof(exchange));

printf("ec: Writing data ASYNC. Sending...\n");
    msgSend(port, &msg);
printf("ec: Returning error code: >%d<\n\n", msg.o.io.err);

    return 0;
}


int exchangeAsyncTest(unsigned int port)
{   
    msg_t msg;
    exchange_msg_t exchange;
    uint8_t data[3] = {0x13, 0x57, 0x91};

    exchange.dev_no = 1;
    exchange.len = 3;
    memcpy(exchange.data, data, 3);

    msg.type = mtExchangeAsync;
    memcpy(msg.i.raw, (unsigned char*)&exchange, sizeof(exchange));

printf("ec: Exchanging data ASYNC. Sending...\n");
    msgSend(port, &msg);
printf("ec: Returning error code: >%d<\n\n", msg.o.io.err);

    return 0;
}


int readAsyncTest(unsigned int port)
{   
    msg_t msg;
    exchange_msg_t exchange;

    exchange.dev_no = 1;
    exchange.len = 3;

    msg.type = mtReadAsync;
    memcpy(msg.i.raw, (unsigned char*)&exchange, sizeof(exchange));

printf("ec: Reading data ASYNC. Sending...\n");
    msgSend(port, &msg);
printf("ec: Returning error code: >%d<\n\n", msg.o.io.err);

    return 0;
}


int main(int argc, char **argv)
{
/* CONNECTING TO SERVER */
    oid_t dir;
    int lookup_tries = 0;
    printf("ecspi-client (ec)\n");
    printf("KOMPILACJA KLIENTA: 5\n");
    while(lookup_tries < 1000)
    {   
        if (lookup("/dev/ecspi", NULL, &dir) < 0)
        {
            printf("ec: lookup failed: %d\n", lookup_tries);
            ++lookup_tries;
        }
        else
        {
            printf("ec: lookup successful: port: %d\n", dir.port);
            break;
        }
    }
    if (lookup_tries == 1000)
        return -1;

    devCtlTest(dir.port);
    chanCtlTest(dir.port);
    chanSelectTest(dir.port);
    
    exchangeDataBlockingTest(dir.port);
    exchangeDataBusyTest(dir.port);

    readAsyncTest(dir.port);
    writeAsyncTest(dir.port);
    exchangeAsyncTest(dir.port);
    readAsyncTest(dir.port);


    return 0;
}