# board_app/Makefile —— IMX6ULL 板级 LVGL 应用
# 用法: make           编译出 build/main_arm (ARM 静态可执行)
#       make clean     清理 build/
#
# LVGL 源码复用 lv_port_pc_vscode 里的 lvgl/ (不复制、不修改库)

CROSS    ?= arm-linux-gnueabihf-
CC        = $(CROSS)gcc

LVGL_DIR := ../lv_port_pc_vscode/lvgl

BUILD     = build
OBJ_DIR   = $(BUILD)/obj

# lv_conf.h 在本目录；-I include 供用户代码 include "lvgl/lvgl.h"
CFLAGS   += -O2 -g -Wall
CFLAGS   += -std=gnu99
CFLAGS   += -I. -I$(LVGL_DIR)/include -I$(LVGL_DIR)/src -I$(LVGL_DIR)
CFLAGS   += -DLV_CONF_INCLUDE_SIMPLE=1
CFLAGS   += -Wno-unused-function -Wno-unused-variable
CFLAGS   += -I. -I../usrcode   

LDFLAGS  += -static -lm -lpthread

TARGET    = $(BUILD)/main_arm

# 生成的 .o 规则在 all: 之前，需显式指定默认目标是 all
.DEFAULT_GOAL := all

# 应用层源码：GUI 主程序 + 字体 + 设备状态表 + 真实 GPIO 驱动
APP_SRC   = main.c \
            fonts/heiti_32.c \
            ../usrcode/data/dev_status.c \
            ../usrcode/hardware/gpio.c \
            ../usrcode/hardware/env_status.c

LVGL_SRC := $(shell find $(LVGL_DIR)/src -name '*.c')
ALL_SRC   = $(APP_SRC) $(LVGL_SRC)

# 目标文件统一放在 obj/ 下、并镜像源码目录结构。
# 源路径(如 ../lv_port_pc_vscode/lvgl/src/...)开头的 ../ 会被去掉，
# 否则 $(OBJ_DIR)/%.o 会变成 obj/../lv_port_pc_vscode/...，
# 让 make 在 build/ 根目录“越界”重建一棵伪源码树（旧版 bug）。
OBJS      = $(foreach s,$(ALL_SRC),$(OBJ_DIR)/$(patsubst ../%,%,$(s:.c=.o)))
DEPS      = $(OBJS:.o=.d)

# 目标与源路径不再一一对应，无法用 %.o:%.c 通配规则，
# 改为给每个源文件显式生成一条编译规则
define compile_rule
$(1): $(2)
	@mkdir -p $$(dir $(1))
	$$(CC) $$(CFLAGS) -MMD -MP -c -o $(1) $(2)
endef

$(foreach s,$(ALL_SRC),$(eval $(call compile_rule,$(OBJ_DIR)/$(patsubst ../%,%,$(s:.c=.o)),$(s))))

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)
	@echo "Build OK: $(TARGET)"
	@file $(TARGET)

-include $(DEPS)

clean:
	rm -rf $(BUILD)

.PHONY: all clean
