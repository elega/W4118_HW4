CROSS_COMPILE = arm-none-linux-gnueabi-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
CFLAGS := -Wall -Werror -g -I../tools/gmp/include
INCLUDE = -I. -I../android-tegra-3.1/usr/include -I../android-tegra-3.1/include -I../android-tegra-3.1/arch/arm/include
LDFLAGS := -L../tools/gmp/lib -lgmp -lm -static $(INCLUDE)


all: test infinite

test: test.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

infinite: infinite_loop.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)


clean:
	rm -f test.o test

.PHONY: clean


# -------------------------------------------------------------------------
#  Installation targets below this line

TARGET_DIR = /data/misc

%_emu: ADB_OPT = -e
%_emu: ADB_TGT = Android Emulator
%_dev: ADB_OPT = -d
%_dev: ADB_TGT = Android Device

install_%: $(all)
	@echo [INSTALL] moving test binaries to $(ADB_TGT) [$(TARGET_DIR)]...
	@adb $(ADB_OPT) remount
	@adb $(ADB_OPT) push test $(TARGET_DIR)
	@adb $(ADB_OPT) push infinite $(TARGET_DIR)
	@adb $(ADB_OPT) shell chmod 6755 $(TARGET_DIR)/test
	@adb $(ADB_OPT) shell chmod 6755 $(TARGET_DIR)/infinite
