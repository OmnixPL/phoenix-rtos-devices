
#include <stdio.h>
#include <sys/msg.h>
#include <string.h>
#include <unistd.h> /* For usleep */

#include <libspisrv.h>

#include "../imx6ull-server/ecspi-server.h"

#define MODULE_NAME "ecspi-client"
#define LOG_ERROR(str, ...) do { if (1) fprintf(stdout, MODULE_NAME ": ERROR: " str "\n", ##__VA_ARGS__); } while (0)
#define TRACE(str, ...) do { if (1) fprintf(stdout, MODULE_NAME ": trace: " str "\n", ##__VA_ARGS__); } while (0)

#define WRITE 0x0
#define READ 0xC0
#define WHO_AM_I 0x0F
#define CTRL1 0x20
#define ACC_X 0x28

#define CFG_MSG 2
#define DATA_MSG 7

static double rawToDegrees(uint8_t low, uint8_t hi)
{
    uint16_t raw = (hi << 8) + low;
    TRACE("RAW: %2x LOW: %2x HI: %2x", raw, low, hi);
    return -180.0 + (raw * 360.0 / 0xffff);

}

static double rawToG(uint8_t low, uint8_t hi)
{
    int16_t raw = (hi << 8) + low;
    double g = -((double)raw / 0x4000);
    return g;
}

int main(int argc, char **argv)
{
    oid_t spi4;
    uint8_t in[CFG_MSG] = {0}, out[CFG_MSG] = {0};
    uint8_t inAcc[DATA_MSG] = {0}, outAcc[DATA_MSG] = {0};
printf("KOMPILACJA KLIENTA: 11\n");

    spi_openDev(&spi4, 4);
    /* IMU WORKS WITH PRE 3+ */
    spi_devInit(&spi4, 1, 0, 2, 0, 0);
    spi_chanCtl(&spi4, 1, 3);
    spi_chanSelect(&spi4, 0);
    out[0] = READ + WHO_AM_I;
    spi_exchange(&spi4, spi_exchange_busy, out, in, CFG_MSG);
    TRACE("Device should answer 49: %x %x", in[0], in[1]);

    /* enable accelerometer */
    out[0] = WRITE + CTRL1;
    out[1] = 0x57;
    spi_exchange(&spi4, spi_exchange_busy, out, in, CFG_MSG);

    outAcc[0] = READ + ACC_X;
    while (1) {
        spi_exchange(&spi4, spi_exchange_busy, outAcc, inAcc, DATA_MSG);
        printf("Acceleration [G]: X: % 3.3f Y: % 3.3f, Z: % 3.3f\n", rawToG(inAcc[1], inAcc[2]), rawToG(inAcc[3], inAcc[4]), rawToG(inAcc[5], inAcc[6]));
        TRACE("ACC RAW X: %2x%02x Y: %2x%02x Z: %2x%02x", inAcc[2], inAcc[1], inAcc[4], inAcc[3], inAcc[6], inAcc[5]);
        usleep(100000);
    }
    return 0;
}