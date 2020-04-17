#include <ecspi-server.h>

int spi_openDev(oid_t *oid, int dev_no);
int spi_devInit(oid_t *oid, uint8_t chan_msk, uint8_t pre, uint8_t post, uint8_t delayCS, uint16_t delaySS);
int spi_chanCtl(oid_t *oid, uint8_t chan, uint8_t mode);
int spi_chanSelect(oid_t *oid, uint8_t chan);
int spi_exchange(oid_t *oid, uint8_t type, uint8_t *out, uint8_t *in, int len);
int spi_exchangeAsync(oid_t *oid, uint8_t type, uint8_t *out, int len);
int spi_readAsync(oid_t *oid, uint8_t *in, int len);