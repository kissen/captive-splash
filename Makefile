CC = xtensa-lx106-elf-gcc
CFLAGS = -I. -mlongcalls
LDLIBS = -nostdlib -Wl,--start-group -lmain -lnet80211 -lwpa -llwip -lpp \
	-lphy -lc -Wl,--end-group -lgcc
LDFLAGS = -Teagle.app.v6.ld

captive-splash-0x00000.bin: captive-splash
	esptool.py elf2image $^

captive-splash: captive-splash.o error.o

flash: captive-splash-0x00000.bin
	esptool.py write_flash 0 captive-splash-0x00000.bin 0x10000 captive-splash-0x10000.bin

clean:
	rm -f captive-splash captive-splash.o captive-splash-0x00000.bin captive-splash-0x10000.bin
