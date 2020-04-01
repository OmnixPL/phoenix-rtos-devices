#define ECSPI_MAX_DATA 56

enum {
	/* mtDevCtl = 5, - already exists */
	mtChanCtl = 15,
	mtChanSelect,
	mtExchange, mtExchangeBusy,
	mtReadAsync, mtWriteAsync, mtExchangeAsync,
	mtExchangePeriodically
};

typedef struct {
	int dev_no;
	uint8_t chan_msk;
	uint8_t pre;
	uint8_t post;
	uint8_t delayCS;
	uint16_t delaySS;
} dev_ctl_msg_t;

typedef struct {
	int dev_no;
	uint8_t chan;
	uint8_t mode;
} chan_ctl_msg_t;

typedef struct {
	int dev_no;
	size_t len;
	uint8_t data[ECSPI_MAX_DATA];
} exchange_msg_t;