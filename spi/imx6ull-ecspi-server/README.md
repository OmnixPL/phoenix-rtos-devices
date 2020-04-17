# imx6ull-ecspi-server

This server allows communication with i.MX 6ULL SPI.

Communication with server is performed via messages send to ports from */dev/spi1* - */dev/spi4* files, where number corresponds with SPI instance number. Struct `spi_i_devctl_t` is used to pass information and should be copied into kernel message field `i.raw`. Struct `spi_i_devctl_t` is available via `#include <ecspi-server.h>` and can be seen here:
```c
typedef struct {
	int id;

	enum {
		spi_dev_ctl, spi_chan_ctl, spi_chan_select,
		spi_exchange_blocking, spi_exchange_busy,
		spi_read_async, spi_write_async, spi_exchange_async
	} type;
	
	union {
		struct {
			uint8_t chan_msk;
			uint8_t pre;
			uint8_t post;
			uint8_t delayCS;
			uint16_t delaySS;
		} dev;

		struct {
			uint8_t chan;
			uint8_t mode;
		} chan;
	};
} spi_i_devctl_t;
```

`id` field in `spi_i_devctl_t` is used to indicate which SPI instance to use and is obtained from `oid_t` when looking up */dev/spi1* - */dev/spi4* file.

## Initialization

Initialize device with `spi_dev_ctl` type message. Use `dev` struct.

Explanation of variables (same as in driver):
- `chan_msk` - 4-bit bitmask stating which Slave Select channels will be enabled
- `pre` ∊ [0, 15] - divides clock frequency by *value + 1*, e.g. `pre = 0` causes clock to operate at nominal frequency
- `post` ∊ [0, 15] - divides clock frequency exponentially by *2<sup>value</sup>*,  e.g. `post = 2` causes clock to operate at ¼ nominal frequency
- `delayCS` ∊ [0, 63] - delay between asserting CS (Chip Select, Slave Select) and the first SPI clock edge (a CS-to-SCLK delay) in SPI clocks
- `delaySS` ∊ [0, 32767] - delay between consecutive transactions (from the end of the previous transaction to the beginning of the next transaction) in SPI clocks. SS during this time will be inactive.

Next, set mode of channel using `spi_chan_ctl` type message. Use `chan` struct.

Explanation of variables:
- `chan` ∊ [0, 3] - channel to control
- `mode` ∊ [0, 3] - (CPOL, CPHA) pair of values

Next, choose channel using `spi_chan_select` type message. Use `chan` struct.

Explanation of variables:
- `chan` ∊ [0, 3] - channel to select
- `mode` - not used

## Usage

Server offers 4 types of communication: synchronous exchange blocking, synchronous exchange busy, asynchronous write-only and asynchronous exchange. 

Data is passed via kernel message `i/o.data/size` fields.
- `i.data` - pointer to outgoing data
- `i.size` - size of outgoing data
- `o.data` - pointer to memory prepared for incoming data
- `o.size` - size of incoming data buffer (not less than `i.size`)

Types of communication (same as in driver):
- Synchronous exchange blocking `spi_exchange_blocking` - this procedure is blocking but employs sleeping on a conditional variable so it is suitable for long transactions
- Synchronous exchange busy `spi_exchange_busy` - busy-waiting for very short transactions (a few bytes at a relatively high speed), where the overhead of synchronization primitives and interrupt handling is greater than the actual transaction time
- Asynchronous write-only `spi_write_async` - writes to SPI and returns. The received data is discarded. `o.data` and `o.size` can be set to *NULL, 0*
- Asynchronous exchange `spi_exchange_async` - writes to SPI and returns. `o.data` and `o.size` can be set to *NULL, 0*. The received data is availabe to read using `spi_read_async`. For reading, `i.data` and `i.size` can be set to *NULL, 0*

## Example
Assumes correct values and no errors for readability.

```c
oid_t oid;
msg_t msg = {0};
spi_i_devctl_t imsg;

uint8_t in[4] = {0}, out[4] ={0};
int len = sizeof(in);

lookup("/dev/spi1", NULL, oid);

/* dev init */

imsg.id = oid.id;
imsg.type = spi_dev_ctl;
imsg.dev.chan_msk = 3;
imsg.dev.pre = 0;
imsg.dev.post = 0;
imsg.dev.delayCS = 0;
imsg.dev.delaySS = 0;

msg.type = mtDevCtl;
memcpy(msg.i.raw, (unsigned char*)&imsg, sizeof(imsg));

msgSend(oid.port, &msg);

/* chan ctl */

imsg.id = oid.id;
imsg.type = spi_chan_ctl;
imsg.chan.chan = 1;
imsg.chan.mode = 2;
    
msg.type = mtDevCtl;
memcpy(msg.i.raw, (unsigned char*)&imsg, sizeof(imsg));

msgSend(oid.port, &msg);

/* chan select */
imsg.id = oid.id;
imsg.type = spi_chan_select;
imsg.chan.chan = 1;
    
msg.type = mtDevCtl;
memcpy(msg.i.raw, (unsigned char*)&imsg, sizeof(imsg));

msgSend(oid.port, &msg);

/* example transaction - exchange async busy */
imsg.id = oid.id;
imsg.type = spi_exchange_busy;

msg.i.data = &out;
msg.i.size = len;
msg.o.data = &in;
msg.o.size = len;

msg.type = mtDevCtl;
memcpy(msg.i.raw, (unsigned char*)&imsg, sizeof(imsg));

msgSend(oid.port, &msg);
```

## Errors
Return values of messages are stored in kernel message `o.io.err` field. Values used by server:
- `EOK` - no error
- `EIO` - driver returned error, possibly arguments out-of-range.
- `ECONNREFUSED` - trying to use uninitialized instance.
- `EAGAIN` - for async write: If the length is different, there is no space in the hardware queue, or there is an operation other than ecspi_writeAsync() pending, then no data is put. For async exchange: If there is any operation pending, then no data is put.
- `ENOSYS` - wrong kernel message type (must be mtDevCtl)