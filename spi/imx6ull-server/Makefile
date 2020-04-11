$(PREFIX_PROG)imx6ull-server: $(addprefix $(PREFIX_O)spi/imx6ull-server/, ecspi-server.o) $(PREFIX_A)libecspi.a
	$(LINK)

$(PREFIX_A)libspisrv.a: $(PREFIX_O)spi/imx6ull-server/libspisrv.o
	$(ARCH)

$(PREFIX_H)libspisrv.h: spi/imx6ull-server/libspisrv.h
	$(HEADER)

all: $(PREFIX_PROG_STRIPPED)imx6ull-server $(PREFIX_A)libspisrv.a $(PREFIX_H)libspisrv.h
