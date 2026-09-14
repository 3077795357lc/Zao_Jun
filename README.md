# 🔥 灶君 · 基于 IMX6ULL 的多功能智能家居中控系统

>  Zao Jun — 可实时监控全屋设备状态并远程控制的智能管家
>
>  技术平台：IMX6ULL 嵌入式 Linux · LVGL · MQTT · NTP

![Platform](https://img.shields.io/badge/Platform-IMX6ULL%20Linux-blue)
![GUI](https://img.shields.io/badge/GUI-LVGL-brightgreen)
![Protocol](https://img.shields.io/badge/Protocol-MQTT-red)
![TimeSync](https://img.shields.io/badge/Time%20Sync-NTP-orange)
![Language](https://img.shields.io/badge/Language-C-blueviolet)
![Status](https://img.shields.io/badge/Status-Developing-yellow)

***

## 📋 目录

- [项目简介](#项目简介)
- [核心功能](#核心功能)
- [技术架构](#技术架构)
- [系统架构图](#系统架构图)
- [多端支持](#多端支持)
- [项目结构](#项目结构)
- [开发进度](#开发进度)
- [Roadmap](#roadmap)
- [相关文档](#相关文档)

***

## 📖 项目简介

本项目基于 **IMX6ULL 嵌入式 Linux 平台**，开发一款多功能智能家居中控系统：

- 采用 **LVGL** 搭建嵌入式可视化 GUI，提供本地人机交互界面；
- 实现**多源传感器数据采集**与**家电设备状态监测**，覆盖监控摄像头、温湿度传感器、空调、音乐播放及各房间灯具等设备；
- 提供**本地日志管理**能力，记录系统运行日志、错误日志、事件日志等；
- 通过 **NTP** 协议与互联网时间服务器同步，保证系统时间准确可靠；
- 依托 **MQTT** 物联网通信协议，打通设备端与云端的数据链路；
- 支持**微信小程序、浏览器网页、开发板显示屏**等多端远程监测与设备控制。

项目当前处于**开发阶段**：板端 LVGL 界面、温湿度采集、灯具与空调的开关控制及状态显示、NTP 对时、MQTT 云平台接入、浏览器网页端远程监控与控制均已跑通；微信小程序端、本地日志管理、摄像头 / 音频播放 / 定时任务页面仍在开发中。

### 🔥 名字由来

「灶君」取自中国民间信仰中的**灶神**——他是唯一住在凡人家里的神，主管家家户户饮食，后演变为保佑家庭合欢平安。项目以此命名，寓意这款中控系统就像一位住在你家、守护全屋设备的"灶王爷"，帮助你监控家庭设备情况、提供及时的预警并使你可以远程控制家庭设备。

***

## ⚙️ 核心功能

| 功能模块       | 说明                                   | 状态  |
| :--------- | :----------------------------------- | :-- |
| 板端可视化界面    | LVGL 界面外壳与主界面      | ✅ 已跑通 |
| 温湿度数据采集    | DHT11 传感器采集                 | ✅ 已跑通 |
| 家电设备控制     | 灯具与空调开关控制（GPIO）          | ✅ 已跑通 |
| 家电设备状态监测   | 状态表统一维护设备开关状态          | ✅ 已跑通 |
| NTP 网络时间同步 | 与互联网 NTP 服务器同步系统时间           | ✅ 已跑通 |
| MQTT 物联网通信 | 设备端与云端双向通信：上报状态 + 接收控制指令              | ✅ 已跑通 |
| 浏览器网页端     | 网页端实时查看温湿度与设备状态，并远程控制灯具 / 空调          | ✅ 已跑通 |
| 本地日志管理     | （具体日志类型规划中）        | 🚧 未开始 |
| 微信小程序端     | 远程监测设备状态、远程控制设备                      | 🚧 未开始 |
| 摄像头 / 音频 / 定时 | 摄像头预览、音乐播放、定时任务等扩展功能                  | 🚧 占位页 |

***

## 🛠️ 技术架构

### 一、硬件与系统

| 项目     | 选型                                                  |
| :----- | :-------------------------------------------------- |
| 主控芯片   | IMX6ULL（ARM Cortex-A7 / ARMv7-A，armhf）               |
| 操作系统   | 嵌入式 Linux 4.9.88                                    |
| 显示     | 1024×600 LCD，走 framebuffer `/dev/fb0`，32 位色深         |
| 触摸输入   | goodix 电容触摸屏，走 evdev `/dev/input/event1`            |
| 传感器    | DHT11 温湿度传感器                           |
| 执行器    | LED 指示灯、直流电机（模拟空调，INA / INB 双 GPIO 控制）               |
| 网络接入   | WiFi（RTL8723BU 驱动 `8723bu.ko`）+ `wpa_supplicant` + `udhcpc` |

### 二、软件与技术栈

| 分类         | 选型                                                              |
| :--------- | :-------------------------------------------------------------- |
| 开发语言       | C（`gnu99`）                                                       |
| 并发模型       | POSIX 多线程       |
| GUI 框架     | LVGL **v9.6.0**          |
| LVGL 后端    | fbdev + evdev； |
| 通信协议       | MQTT（设备状态上报 + 控制指令下发）                              |
| 时间同步       | NTP（开机对时，超时 2s 不阻塞启动）                              |
| MQTT 客户端库  | 设备端 libmosquitto **2.0.18**（静态链接）；网页端 MQTT.js over WebSocket      |

### 三、工具链与工程

| 环节        | 工具 / 做法                                                              |
| :-------- | :------------------------------------------------------------------- |
| 交叉编译工具链   | `arm-linux-gnueabihf-gcc` 11.4.0                     |
| 构建系统      | GNU Make（[usrcode/Makefile](./usrcode/Makefile)，按文件生成编译规则 + `-MMD` 自动头文件依赖）  |
| 编译选项      | `-O2 -g -Wall -std=gnu99`，`LV_CONF_INCLUDE_SIMPLE=1`                   |
| 链接方式      | 静态链接 `-static -lm -lpthread` + `libmosquitto.a`               |
| 内核模块编译    | 依赖板端内核源码树，配置 `KERN_DIR` / `ARCH` / `CROSS_COMPILE` 后交叉编译出 `.ko`      |
| 部署方式      | adb over USB（[deploy.sh](./usrcode/deploy.sh)：编译 → push 到板端 `/root` → 重启进程） |
| 启动脚本    | `/etc/init.d/S09modload`（加载驱动）、`/etc/init.d/S41wifi`（开机自动连热点）        |

> 第三方依赖说明：LVGL 源码已随仓库内置，克隆即可编译；**libmosquitto 需要自行下载并交叉编译出 ARM 静态库**，当前 [Makefile](./usrcode/Makefile) 中 `MOSQUITTO_DIR` 仍指向开发机绝对路径 `/home/lc/third_party/mosquitto-2.0.18`，换机器编译前需先改这一行。

***

## 🏗️ 系统架构图

> 完整的系统架构图见 [architect_design.png](./architect_design.png)。

![系统架构图](./architect_design.png)

```mermaid
flowchart TB
    subgraph 端侧设备
        Sensor[温湿度传感器] --> Core
        Cam[监控摄像头] --> Core
        AC[空调] --> Core
        Music[音乐播放] --> Core
        Light[各房间灯具] --> Core
    end

    subgraph 中控主机
        Core[IMX6ULL 中控] --> GUI[LVGL 可视化界面]
        Core --> Log[本地日志管理]
        Core --> NTP[NTP 时间同步]
        Core --> MQTT[MQTT Client]
    end

    MQTT <--> Broker[(MQTT Broker / 云平台)]
    Broker <--> Web[浏览器网页]
    Broker <--> MiniProg[微信小程序]
```

***

## 📲 多端支持

| 终端         | 能力                     | 状态      |
| :--------- | :--------------------- | :------ |
| 🖥️ 开发板显示屏 | LVGL 本地可视化界面，支持本地查看与控制 | ✅ 已跑通   |
| 🌐 浏览器网页   | 远程监测设备状态、远程控制设备        | ✅ 已跑通   |
| 💬 微信小程序   | 远程监测设备状态、远程控制设备        | 🚧 未开始  |

***

## 📁 项目结构

```
Smart_home_central/
├── README.md             # 项目说明（本文件）
├── architect_design.png  # 系统架构图
├── lv_port_pc_vscode/    # LVGL 源码
├── web/                  # 浏览器网页端（纯静态，MQTT over WebSocket）
└── usrcode/              # 板端应用源码
    ├── Makefile          # 交叉编译脚本，产出 build/main_arm
    ├── deploy.sh         # 一键部署：编译 -> adb 推送 -> 板端重启
    ├── lv_conf.h         # LVGL 裁剪配置
    ├── main.c            # 程序入口：硬件初始化 + 启动界面 + 主循环
    ├── common_type.h/.c  # 公共类型定义（设备 ID、设备状态、温湿度）
    ├── ui/               # 界面层
    ├── data/             # 数据结构层：设备状态表、环境状态表
    ├── hardware/         # 硬件驱动层：GPIO、DHT11
    ├── net/              # 通信层：NTP 对时、MQTT 客户端
    └── assets/fonts/     # 静态资源：中文黑体、FontAwesome 图标字体
```

***

## 🚧 开发进度

| 阶段        | 状态  |
| :-------- | :-- |
| 需求分析与架构梳理 | 已完成 |
| 业务代码开发    | 进行中 |
| 联调与稳定性测试  | 未开始 |
| 演示视频      | 未开始 |

***

## 🗺️ Roadmap

- [√] 需求分析与系统架构梳理
- [√] 板端 LVGL 界面外壳 + 主界面（温湿度 / 设备控制）
- [√] 温湿度数据采集（DHT11 自研内核驱动 + 采集线程）
- [√] 家电设备控制与状态监测（灯具、空调）
- [√] NTP 网络时间同步
- [√] MQTT 云平台接入（状态上报 + 指令下发）
- [√] 浏览器网页端适配（远程监控与控制）
- [ ] 板端设置页 / 日志页 / 摄像头 / 音频 / 定时任务页面
- [ ] 本地日志管理模块
- [ ] 微信小程序端适配


***

## 📄 相关文档

- [architect_design.png](./architect_design.png) — 系统架构图
- `usrcode/` 各子目录下的 `readme` — ui / data / hardware 三层职责速览

