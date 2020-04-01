$(PREFIX_PROG)imx6ull-server: $(addprefix $(PREFIX_O)spi/imx6ull-server/, ecspi-server.o) $(PREFIX_A)libecspi.a
	$(LINK)

#jakbys se kiedys chcial zrobic biblioteke
#$(PREFIX_A)libflashdrv.a: $(PREFIX_O)storage/imx6ull-flash/flashdrv.o
#	$(ARCH)

#$(PREFIX_H)flashsrv.h: spi/imx6ull-server/####tu podaj swoje pliki naglowkowe chyba mordoflashsrv.h
#	$(HEADER)

#jakbys se kiedys chcial zrobic biblioteke
#po imx6ull-server dodaj $(PREFIX_A)libflashdrv.a   $(PREFIX_H)flashsrv.h
all: $(PREFIX_PROG_STRIPPED)imx6ull-server
