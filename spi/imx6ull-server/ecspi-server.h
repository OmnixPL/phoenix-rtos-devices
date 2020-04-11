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