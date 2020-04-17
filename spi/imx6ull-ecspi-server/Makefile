$(PREFIX_PROG)imx6ull-server: $(addprefix $(PREFIX_O)spi/imx6ull-ecspi-server/, ecspi-server.o) $(PREFIX_A)libecspi.a
	$(LINK)

$(PREFIX_H)ecspi-server.h: spi/imx6ull-ecspi-server/ecspi-server.h
	$(HEADER)

all: $(PREFIX_PROG_STRIPPED)imx6ull-server $(PREFIX_H)ecspi-server.h
