
#include <stdio.h>
#include <sys/msg.h>
#include <string.h>
#include <libspisrv.h>

#include "../imx6ull-server/ecspi-server.h"

#define MODULE_NAME "ecspi-client"
#define LOG_ERROR(str, ...) do { if (1) fprintf(stdout, MODULE_NAME ": ERROR: " str "\n", ##__VA_ARGS__); } while (0)
#define TRACE(str, ...) do { if (1) fprintf(stdout, MODULE_NAME ": trace: " str "\n", ##__VA_ARGS__); } while (0)

int main(int argc, char **argv)
{
    oid_t spi4;
    uint8_t in[3] = {0, 0, 0}, out[3] = {0xAE, 0xFF, 0x33};
    int len = 3;
    printf("KOMPILACJA KLIENTA: 11\n");

    spi_openDev(&spi4, 4);
    spi_devInit(&spi4, 3, 5, 7, 9, 11);
    spi_chanCtl(&spi4, 1, 1);
    spi_chanSelect(&spi4, 1);
    spi_exchange(&spi4, spi_exchange_busy, out, in, len);
    spi_exchange(&spi4, spi_exchange_blocking, out, in, len);
    spi_exchangeAsync(&spi4, spi_exchange_async, out, len);
    spi_exchangeAsync(&spi4, spi_write_async, out, len);
    spi_readAsync(&spi4, in, len);
    spi_readAsync(&spi4, in, len + 1);
    return 0;
}