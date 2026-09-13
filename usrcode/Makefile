CROSS    ?= arm-linux-gnueabihf-
CC        = $(CROSS)gcc

LVGL_DIR := ../lv_port_pc_vscode/lvgl

# 第三方 MQTT 客户端库
MOSQUITTO_DIR ?= /home/lc/third_party/mosquitto-2.0.18

BUILD     = build
OBJ_DIR   = $(BUILD)/obj

# lv_conf.h 在本目录；
# -I.   供 include "lvgl/lvgl.h" 以及跨模块引用 (ui/, data/, hardware/)
# -Iui  供 ui/ 内部互相引用 (ui_theme.h, pages/page.h)
CFLAGS   += -O2 -g -Wall
CFLAGS   += -std=gnu99
CFLAGS   += -I. -Iui -I$(LVGL_DIR)/include -I$(LVGL_DIR)/src -I$(LVGL_DIR)
CFLAGS   += -I$(MOSQUITTO_DIR)/include
CFLAGS   += -DLV_CONF_INCLUDE_SIMPLE=1
CFLAGS   += -Wno-unused-function -Wno-unused-variable

LDFLAGS  += -static -lm -lpthread
LDFLAGS  += $(MOSQUITTO_DIR)/lib/libmosquitto.a

TARGET    = $(BUILD)/main_arm

# 生成的 .o 规则在 all: 之前，需显式指定默认目标是 all
.DEFAULT_GOAL := all

# 应用层源码：入口 main.c + 界面外壳与页面 + 字体 + 数据层 + 硬件驱动
APP_SRC   = main.c \
            assets/fonts/heiti_32.c \
            assets/fonts/icons_32.c \
            ui/ui_main.c \
            ui/pages/page_dashboard.c \
            ui/pages/page_stub.c \
            data/dev_status.c \
            data/env_status.c \
            hardware/gpio.c \
            net/ntp.c \
            net/mqtt_client.c

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
