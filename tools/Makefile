CC := gcc
OBJCOPY := objcopy
CFLAGS := -g -Os -static -nostdlib -nostdinc -fno-pie -no-pie -mno-red-zone -fno-omit-frame-pointer -pg -mnop-mcount -DCOSMO
LDFLAGS := -fuse-ld=bfd -Wl,-T,cosmo/ape.lds -include cosmo/cosmopolitan.h cosmo/crt.o cosmo/ape.o cosmo/cosmopolitan.a

all: datread datpack datstrip

.PHONY: cosmo
cosmo: datread.com datpack.com datstrip.com

.PHONY: native
cosmo: datread datpack datstrip

datread.com: reader.c
	@echo "$(CC) $<"
	@$(CC) $(CFLAGS) -o $@.dbg $^ $(LDFLAGS)
	@$(OBJCOPY) -S -O binary $@.dbg $@
	@echo "$(OBJCOPY) $@.dbg -> $@"

datpack.com: packer.c
	@echo "$(CC) $<"
	@$(CC) $(CFLAGS) -o $@.dbg $^ $(LDFLAGS)
	@$(OBJCOPY) -S -O binary $@.dbg $@
	@echo "$(OBJCOPY) $@.dbg -> $@"

datstrip.com: stripper.c ../backend/gd_list.c ../backend/ini.c
	@echo "$(CC) $<"
	@$(CC) $(CFLAGS) -o $@.dbg $^ $(LDFLAGS)
	@$(OBJCOPY) -S -O binary $@.dbg $@
	@echo "$(OBJCOPY) $@.dbg -> $@"

datread: reader.c
	@echo "$(CC) $<"
	@$(CC) -O2 -s -o $@ $^
	@echo "> $@"

datpack: packer.c
	@echo "$(CC) $<"
	@$(CC) -O2 -s -o $@ $^
	@echo "> $@"

datstrip: stripper.c ../backend/gd_list.c ../backend/ini.c
	@echo "$(CC) $<"
	@$(CC) -O2 -s -o $@ $^
	@echo "> $@"

.PHONY: clean
clean:
	-rm -f datread.com.dbg datpack.com.dbg datstrip.com.dbg

