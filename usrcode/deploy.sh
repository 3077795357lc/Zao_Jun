#!/bin/bash
#
# 一键部署到 i.MX6ULL 开发板：编译 -> 推送 -> 重启程序
#
# 用法:
#   ./deploy.sh        前台运行，直接看程序打印，Ctrl+C 结束
#   ./deploy.sh -b     后台运行，日志写板子的 /tmp/main_arm.log，脚本立即返回
#
# 依赖: arm-linux-gnueabihf- 交叉工具链、adb、开发板已用 USB 连好

# 任何一条命令失败就立刻退出，不要带着旧二进制继续往下跑
set -e

# 切到脚本自身所在目录，这样在任意路径下执行都能找到 Makefile
cd "$(dirname "$0")"

TARGET=build/main_arm
BOARD_DIR=/root
BOARD_BIN=main_arm

echo "==> [1/4] 编译"
make -j3

echo "==> [2/4] 检查开发板连接"
if ! adb get-state >/dev/null 2>&1; then
    echo "错误: adb 不可用，或没检测到开发板，先执行 adb devices 看看" >&2
    exit 1
fi

echo "==> [3/4] 推送 $TARGET -> $BOARD_DIR/"
adb push "$TARGET" "$BOARD_DIR/"

# 先杀掉可能存在的旧实例，否则两个进程会抢 /dev/fb0 和触摸设备，
# 表现为界面自己在动、按键乱翻转；sleep 1 是等旧进程真正释放 framebuffer
echo "==> [4/4] 重启程序"
if [ "$1" = "-b" ]; then
    adb shell "killall $BOARD_BIN 2>/dev/null; sleep 1; \
               $BOARD_DIR/$BOARD_BIN > /tmp/main_arm.log 2>&1 &"
    echo "已在板子后台启动，查看日志: adb shell cat /tmp/main_arm.log"
else
    adb shell "killall $BOARD_BIN 2>/dev/null; sleep 1; $BOARD_DIR/$BOARD_BIN"
fi
