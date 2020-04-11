#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/msg.h>

#include "ecspi-server.h"

#define MODULE_NAME "libspisrv"
#define LOG_ERROR(str, ...) do { if (1) fprintf(stdout, MODULE_NAME ": ERROR: " str "\n", ##__VA_ARGS__); } while (0)
#define TRACE(str, ...) do { if (1) fprintf(stdout, MODULE_NAME ": trace: " str "\n", ##__VA_ARGS__); } while (0)



int spi_openDev(oid_t *oid, int dev_no)
{
    if (dev_no < 1 || dev_no > 4)
        return -EINVAL;

    char path[] = "/dev/spi0";
    path[8] += dev_no;
    if (lookup(path, NULL, oid) < 0)
    {
        LOG_ERROR("lookup failed");
        return -ENODEV;
    }
    TRACE("dev file ok");
    return EOK;
}


int spi_devInit(oid_t *oid, uint8_t chan_msk, uint8_t pre, uint8_t post, uint8_t delayCS, uint16_t delaySS)
{
    msg_t msg = {0};
    spi_i_devctl_t imsg;

    imsg.id = oid->id;
    imsg.type = spi_dev_ctl;
    imsg.dev.chan_msk = chan_msk;
    imsg.dev.pre = pre;
    imsg.dev.post = post;
    imsg.dev.delayCS = delayCS;
    imsg.dev.delaySS = delaySS;

    msg.type = mtDevCtl;
    memcpy(msg.i.raw, (unsigned char*)&imsg, sizeof(imsg));

    msgSend(oid->port, &msg);
    return 0;
}


int spi_chanCtl(oid_t *oid, uint8_t chan, uint8_t mode)
{
    msg_t msg = {0};
    spi_i_devctl_t imsg;

    imsg.id = oid->id;
    imsg.type = spi_chan_ctl;
    imsg.chan.chan = chan;
    imsg.chan.mode = mode;
        
    msg.type = mtDevCtl;
    memcpy(msg.i.raw, (unsigned char*)&imsg, sizeof(imsg));

    msgSend(oid->port, &msg);
    return 0;
}


int spi_chanSelect(oid_t *oid, uint8_t chan)
{
    msg_t msg = {0};
    spi_i_devctl_t imsg;

    imsg.id = oid->id;
    imsg.type = spi_chan_select;
    imsg.chan.chan = chan;
        
    msg.type = mtDevCtl;
    memcpy(msg.i.raw, (unsigned char*)&imsg, sizeof(imsg));

    msgSend(oid->port, &msg);
    return 0;
}


int spi_exchange(oid_t *oid, uint8_t type, uint8_t *out, uint8_t *in, int len)
{
    msg_t msg = {0};
    spi_i_devctl_t imsg;

    if (type != spi_exchange_blocking && type != spi_exchange_busy) {
        return -EINVAL;
    }

    imsg.id = oid->id;
    imsg.type = type;

    msg.i.data = out;
    msg.i.size = len;
    msg.o.data = in;
    msg.o.size = len;

    msg.type = mtDevCtl;
    memcpy(msg.i.raw, (unsigned char*)&imsg, sizeof(imsg));
    
    msgSend(oid->port, &msg);
    memcpy(in, msg.o.data, len);
    TRACE("*in: %x %x %x", in[0], in[1], in[2]);
    return 0;
}


int spi_exchangeAsync(oid_t *oid, uint8_t type, uint8_t *out, int len)
{
    msg_t msg = {0};
    spi_i_devctl_t imsg;

    if (type != spi_exchange_async && type != spi_write_async) {
        return -EINVAL;
    }

    imsg.id = oid->id;
    imsg.type = type;

    msg.i.data = out;
    msg.i.size = len;

    msg.type = mtDevCtl;
    memcpy(msg.i.raw, (unsigned char*)&imsg, sizeof(imsg));
    
    msgSend(oid->port, &msg);
    return 0;
}

int spi_readAsync(oid_t *oid, uint8_t *in, int len)
{   
    msg_t msg = {0};
    spi_i_devctl_t imsg;

    imsg.id = oid->id;
    imsg.type = spi_read_async;

    msg.o.data = in;
    msg.o.size = len;

    msg.type = mtDevCtl;
    memcpy(msg.i.raw, (unsigned char*)&imsg, sizeof(imsg));

    msgSend(oid->port, &msg);
    TRACE("*in: %x %x %x", in[0], in[1], in[2]);
    return 0;
}