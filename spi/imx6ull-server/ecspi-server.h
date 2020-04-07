typedef struct {
	int id;

	enum {
		spi_devCtl, spi_chanCtl, spi_chanSelect,
		spi_exchange, spi_exchangeBusy,
		spi_readAsync, spi_writeAsync, spi_exchangeAsync
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