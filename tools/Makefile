CC := gcc
EXT =
OBJCOPY := objcopy
CFLAGS := -g -Os -static -nostdlib -nostdinc -fno-pie -no-pie -mno-red-zone -fno-omit-frame-pointer -pg -mnop-mcount -DCOSMO
LDFLAGS := -fuse-ld=bfd -Wl,-T,cosmo/ape.lds -include cosmo/cosmopolitan.h cosmo/crt.o cosmo/ape.o cosmo/cosmopolitan.a
DEFINES := -DSTANDALONE_BINARY=1

all: datread datpack datstrip

.PHONY: cosmo
cosmo: datread.com datpack.com datstrip.com

.PHONY: native
native: datread$(EXT) datpack$(EXT) datstrip$(EXT)

.PHONY: win
win: CC = x86_64-w64-mingw32-gcc -g
win: EXT = .exe
win: native

datread.com: reader.c ../texture/dat_reader.c
	@echo "$(CC) $<"
	@$(CC) $(DEFINES) $(CFLAGS) -o $@.dbg $^ $(LDFLAGS)
	@$(OBJCOPY) -S -O binary $@.dbg $@
	@echo "$(OBJCOPY) $@.dbg -> $@"

datpack.com: packer.c
	@echo "$(CC) $<"
	@$(CC) $(DEFINES) $(CFLAGS) -o $@.dbg $^ $(LDFLAGS)
	@$(OBJCOPY) -S -O binary $@.dbg $@
	@echo "$(OBJCOPY) $@.dbg -> $@"

datstrip.com: stripper.c ../backend/gd_list.c ../backend/ini.c
	@echo "$(CC) $<"
	@$(CC) $(DEFINES) $(CFLAGS)  -o $@.dbg $^ $(LDFLAGS)
	@$(OBJCOPY) -S -O binary $@.dbg $@
	@echo "$(OBJCOPY) $@.dbg -> $@"

datread$(EXT): reader.c ../texture/dat_reader.c
	@echo "$(CC) $<"
	@$(CC) $(DEFINES) -O2 -s -o $@ $^
	@echo "> $@"

datpack$(EXT): packer.c
	@echo "$(CC) $<"
	@$(CC) $(DEFINES) -O2 -s -o $@ $^
	@echo "> $@"

datstrip$(EXT): stripper.c ../backend/gd_list.c ../backend/ini.c
	@echo "$(CC) $<"
	@$(CC) $(DEFINES) -O2 -s -o $@ $^
	@echo "> $@"

.PHONY: clean
clean:
	-rm -f datread.com.dbg datpack.com.dbg datstrip.com.dbg

.PHONY: win-distclean
win-distclean: EXT = .exe
win-distclean: distclean

.PHONY: distclean
distclean: clean
	-rm -f datread.com datpack.com datstrip.com datread$(EXT) datpack$(EXT) datstrip$(EXT)

