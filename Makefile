CC = xtensa-lx106-elf-gcc
CFLAGS = -I. -DICACHE_FLASH -mlongcalls -std=c99 -Wall -Os -flto
LDLIBS = -nostdlib -Wl,--start-group -lmain -lnet80211 -lwpa -llwip -lpp \
	-lphy -lc -Wl,--end-group -lgcc
LDFLAGS = -Teagle.app.v6.ld
OBJS = captive-splash.o error.o http.o dns.o utils.o conq.o

captive-splash-0x00000.bin: captive-splash
	esptool.py elf2image $^

captive-splash: $(OBJS)

http.o: payload.h

payload.h: payload.html
	./bintohead.py payload.html PAYLOAD > payload.h

flash: captive-splash-0x00000.bin
	esptool.py write_flash 0 captive-splash-0x00000.bin 0x10000 captive-splash-0x10000.bin

clean:
	rm -f captive-splash captive-splash-0x00000.bin captive-splash-0x10000.bin $(OBJS) payload.h

.PHONY: flash clean
