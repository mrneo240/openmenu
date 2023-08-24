TARGET := themeMenu.elf

EXT_SRCS := external/easing.c
TXR_SRCS := texture/block_pool.c texture/lru.c texture/txr_manager.c texture/dat_reader.c texture/serial_sanitize.c texture/simple_texture_allocator.c
BACKEND_SRCS := external/ini.c backend/gd_list.c backend/gdemu_sdk.c backend/gdemu_control.c backend/db_list.c
UI_MENUS := ui/ui_scroll.c ui/ui_line_large.c ui/ui_line_desc.c ui/ui_grid.c ui/global_settings.c ui/ui_menu_credits.c ui/theme_manager.c
UI_SRCS := ui/dc/font_bmf.c ui/dc/font_bitmap.c ui/dc/pvr_texture.c ui/dc/input.c ui/draw_kos.c ui/animation.c

SRCS := $(BACKEND_SRCS) $(UI_SRCS) $(UI_MENUS) $(TXR_SRCS) $(EXT_SRCS) main.c
OBJS = $(subst .c,.o,$(SRCS))

CC := kos-cc
AS := kos-as
OBJCOPY := $(KOS_OBJCOPY)
RM := rm

CFLAGS := -I. -ffunction-sections -fdata-sections -std=c11 -O2 -g -Wno-unknown-pragmas -Wall -Wextra $(OPTIONS)
LDFLAGS := -Wl,--gc-sections
LIBS := -lm ./lib/libcrayon_vmu.a

all: clean-elf $(TARGET)

%.o: %.s
	@echo $(AS) $<
	@$(CC) -x assembler-with-cpp $(CFLAGS) -c $< -o $@

%.o: %.c
	@echo $(CC) $<
	@$(CC) -c $< -o $@ $(CFLAGS)

$(TARGET): $(OBJS)
	@echo \> $(CC) -o $(TARGET)
	@$(CC) -o $(TARGET) $(LDFLAGS) $(CFLAGS) $(OBJS) $(LIBS) $(MAP)
	@echo $(notdir $(OBJCOPY)) -R .stack -O binary $@ $(basename $@).bin
	@$(OBJCOPY) -R .stack -O binary  $@ 1ST_READ.BIN

.PHONY: clean
.PHONY: clean-elf

clean:
	-@$(RM) -f $(TARGET) $(OBJS) *.bin *.BIN

clean-elf:
	-@$(RM) -f $(TARGET)
