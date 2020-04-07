
#include <stdio.h>
#include <sys/msg.h>
#include <string.h>

#include "../imx6ull-server/ecspi-server.h"

#define MODULE_NAME "ecspi-client"
#define LOG_ERROR(str, ...) do { if (1) fprintf(stdout, MODULE_NAME ": ERROR: " str "\n", ##__VA_ARGS__); } while (0)
#define TRACE(str, ...) do { if (1) fprintf(stdout, MODULE_NAME ": trace: " str "\n", ##__VA_ARGS__); } while (0)

msg_t msg;

static int devCtlTest(oid_t oid)
{
    spi_i_devctl_t imsg;

    imsg.id = oid.id;
    imsg.type = spi_devCtl;
    imsg.dev.chan_msk = 3;
    imsg.dev.pre = 7;
    imsg.dev.post = 6;
    imsg.dev.delayCS = 11;
    imsg.dev.delaySS = 13;

	msg.type = mtDevCtl;
    memcpy(msg.i.raw, (unsigned char*)&imsg, sizeof(imsg));

TRACE("initalizing device");
    msgSend(oid.port, &msg);
printf("ec: Returning error code: >%d<\n\n", msg.o.io.err);

    return 0;
}

int chanCtlTest(oid_t oid)
{  
    spi_i_devctl_t imsg;

    imsg.id = oid.id;
    imsg.type = spi_chanCtl;
    imsg.chan.chan = 1;
    imsg.chan.mode = 1;
    
    msg.type = mtDevCtl;
    memcpy(msg.i.raw, (unsigned char*)&imsg, sizeof(imsg));


printf("ec: Setting mode. Sending...\n");
    msgSend(oid.port, &msg);
printf("ec: Returning error code: >%d<\n\n", msg.o.io.err);

    return 0;
}


int chanSelectTest(oid_t oid)
{   
    spi_i_devctl_t imsg;

    imsg.id = oid.id;
    imsg.type = spi_chanSelect;
    imsg.chan.chan = 1;

    msg.type = mtDevCtl;
    memcpy(msg.i.raw, (unsigned char*)&imsg, sizeof(imsg));

printf("ec: Selecting channel. Sending...\n");
    msgSend(oid.port, &msg);
printf("ec: Returning error code: >%d<\n\n", msg.o.io.err);

    return 0;
}


int exchangeDataBlockingTest(oid_t oid)
{   
    spi_i_devctl_t exchange;
    uint8_t data[3] = {0xAE, 0x12, 0x34};

    exchange.id = oid.id;
    exchange.type = spi_exchange;
    msg.i.data = data;
    msg.i.size = 3;

    msg.type = mtDevCtl;
    memcpy(msg.i.raw, (unsigned char*)&exchange, sizeof(exchange));

printf("ec: Exchanging data BLOCKING. Sending...\n");
    msgSend(oid.port, &msg);
printf("ec: Returning error code: >%d<\n\n", msg.o.io.err);

    return 0;
}


int exchangeDataBusyTest(oid_t oid)
{   
    spi_i_devctl_t exchange;
    uint8_t data[3] = {0x13, 0x57, 0x91};

    exchange.id = oid.id;
    exchange.type = spi_exchangeBusy;
    msg.i.data = data;
    msg.i.size = 3;

    msg.type = mtDevCtl;
    memcpy(msg.i.raw, (unsigned char*)&exchange, sizeof(exchange));

printf("ec: Exchanging data BUSY. Sending...\n");
    msgSend(oid.port, &msg);
printf("ec: Returning error code: >%d<\n\n", msg.o.io.err);

    return 0;
}


int writeAsyncTest(oid_t oid)
{   
    spi_i_devctl_t exchange;
    uint8_t data[3] = {0x66, 0x55, 0xAA};
    
    exchange.id = oid.id;
    exchange.type = spi_writeAsync;
    msg.i.data = data;
    msg.i.size = 3;

    msg.type = mtDevCtl;
    memcpy(msg.i.raw, (unsigned char*)&exchange, sizeof(exchange));

printf("ec: Writing data ASYNC. Sending...\n");
    msgSend(oid.port, &msg);
printf("ec: Returning error code: >%d<\n\n", msg.o.io.err);

    return 0;
}


int exchangeAsyncTest(oid_t oid)
{   
    spi_i_devctl_t exchange;
    uint8_t data[3] = {0x13, 0x57, 0x91};

    exchange.id = oid.id;
    exchange.type = spi_exchangeAsync;
    msg.i.data = data;
    msg.i.size = 3;

    msg.type = mtDevCtl;
    memcpy(msg.i.raw, (unsigned char*)&exchange, sizeof(exchange));

printf("ec: Exchanging data ASYNC. Sending...\n");
    msgSend(oid.port, &msg);
printf("ec: Returning error code: >%d<\n\n", msg.o.io.err);

    return 0;
}


int readAsyncTest(oid_t oid)
{   
    spi_i_devctl_t exchange;

    exchange.id = oid.id;
    exchange.type = spi_readAsync;
    msg.i.size = 3;

    msg.type = mtDevCtl;
    memcpy(msg.i.raw, (unsigned char*)&exchange, sizeof(exchange));

printf("ec: Reading data ASYNC. Sending...\n");
    msgSend(oid.port, &msg);
printf("ec: Returning error code: >%d<\n\n", msg.o.io.err);

    return 0;
}


int readAsyncTest2(oid_t oid)
{   
    spi_i_devctl_t exchange;

    exchange.id = oid.id;
    exchange.type = spi_readAsync;
    msg.i.size = 4;

    msg.type = mtDevCtl;
    memcpy(msg.i.raw, (unsigned char*)&exchange, sizeof(exchange));

printf("ec: Reading data ASYNC. Sending...\n");
    msgSend(oid.port, &msg);
printf("ec: Returning error code: >%d<\n\n", msg.o.io.err);

    return 0;
}


int main(int argc, char **argv)
{
/* CONNECTING TO SERVER */
    oid_t oid;
    int lookup_tries = 0;
    printf("ecspi-client (ec)\n");
    printf("KOMPILACJA KLIENTA: 9\n");
    while(lookup_tries < 1000)
    {   
        if (lookup("/dev/spi1", NULL, &oid) < 0)
        {
            printf("ec: lookup failed: %d\n", lookup_tries);
            ++lookup_tries;
        }
        else
        {
            printf("ec: lookup successful: port: %d, id: %llu\n", oid.port, oid.id);
            break;
        }
    }
    if (lookup_tries == 1000)
        return -1;

    devCtlTest(oid);
    chanCtlTest(oid);
    chanSelectTest(oid);

    exchangeDataBlockingTest(oid);
    exchangeDataBusyTest(oid);

    readAsyncTest(oid);
    writeAsyncTest(oid);
    exchangeAsyncTest(oid);
    readAsyncTest(oid);
    readAsyncTest2(oid);

    lookup("/dev/spi2", NULL, &oid);
    printf("ec: lookup successful: port: %d, id: %llu\n", oid.port, oid.id);
    chanSelectTest(oid);


    return 0;
}