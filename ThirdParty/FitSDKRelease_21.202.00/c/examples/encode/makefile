CC=gcc
CFLAGS=-I"." -I"../../"

# FIT_USE_STDINT_H can also be defined in fit_config.h
ifneq ($(OS),Windows_NT)
	CFLAGS+=-DFIT_USE_STDINT_H
endif

encode: encode.o fit.o fit_convert.o fit_crc.o fit_product.o
	$(CC) $(CFLAGS) encode.o fit.o fit_convert.o fit_crc.o fit_product.o -o encode -lm

encode.o: encode.c
	$(CC) $(CFLAGS) -c encode.c -o encode.o

fit.o: ../../fit.c ../../fit_config.h ../../fit_include.h
	$(CC) $(CFLAGS) -c ../../fit.c -o fit.o

fit_convert.o: ../../fit_convert.c ../../fit_config.h ../../fit_include.h
	$(CC) $(CFLAGS) -c ../../fit_convert.c -o fit_convert.o

fit_crc.o: ../../fit_crc.c ../../fit_config.h ../../fit_include.h
	$(CC) $(CFLAGS) -c ../../fit_crc.c -o fit_crc.o

fit_product.o: ../../fit_product.c ../../fit_config.h ../../fit_include.h
	$(CC) $(CFLAGS) -c ../../fit_product.c -o fit_product.o

clean:
	rm -f encode *.o test.fit
