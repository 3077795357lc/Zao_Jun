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

项目当前处于**开发阶段**：系统架构已确定，板端 LVGL 界面、温湿度采集、灯具与空调的开关控制及状态显示已跑通；日志管理、NTP、MQTT 与多端联动开发中。

### 🔥 名字由来

「灶君」取自中国民间信仰中的**灶神**——他是唯一住在凡人家里的神，主管家家户户饮食，后演变为保佑家庭合欢平安。项目以此命名，寓意这款中控系统就像一位住在你家、守护全屋设备的"灶王爷"，帮助你监控家庭设备情况、提供及时的预警并使你可以远程控制家庭设备。

***

## ⚙️ 核心功能

| 功能模块       | 说明                                   |
| :--------- | :----------------------------------- |
| 多源传感器数据采集  | 采集监控摄像头、温湿度传感器、空调、音乐播放、各房间灯具等设备的状态数据 |
| 家电设备状态监测   | 实时监测上述家电设备的运行状态并在界面呈现                |
| 本地日志管理     | 记录系统运行日志、错误日志、事件日志等（具体日志类型规划中）       |
| NTP 网络时间同步 | 与互联网 NTP 服务器同步系统时间，确保时间准确可靠          |
| MQTT 物联网通信 | 基于 MQTT 协议实现设备端与云端的双向数据通信            |
| 多端远程控制     | 通过微信小程序、浏览器网页、开发板显示屏等终端远程监测与控制设备     |

***

## 🛠️ 技术架构

| 分类     | 选型                 |
| :----- | :----------------- |
| 硬件平台   | IMX6ULL（嵌入式 Linux） |
| GUI 框架 | LVGL               |
| 通信协议   | MQTT               |
| 时间同步   | NTP                |
| 开发语言   | C                  |

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

| 终端         | 能力                     |
| :--------- | :--------------------- |
| 🖥️ 开发板显示屏 | LVGL 本地可视化界面，支持本地查看与控制 |
| 💬 微信小程序   | 远程监测设备状态、远程控制设备        |
| 🌐 浏览器网页   | 远程监测设备状态、远程控制设备        |

***

## 📁 项目结构

```
Smart_home_central/
├── README.md             # 项目说明（本文件）
├── architect_design.png  # 系统架构图
├── lv_port_pc_vscode/    # LVGL 图形库源码
└── usrcode/              # 应用源码
    ├── Makefile          # 交叉编译脚本，产出 build/main_arm
    ├── lv_conf.h         # LVGL 裁剪配置
    ├── main.c            # 程序入口：硬件初始化 + 启动界面 + 主循环
    ├── common_type.h/.c  # 公共类型定义（设备 ID、设备状态）
    ├── ui/               # 界面层
    │   ├── ui_main.c     #   界面外壳：顶栏时钟 + 侧栏导航 + 页面切换
    │   ├── ui_theme.h    #   配色与尺寸常量
    │   ├── ui_icons.h    #   图标字形宏
    │   └── pages/        #   各页面：主界面、设置、日志、摄像头、音频、定时任务
    ├── data/             # 数据结构层：设备状态表
    ├── hardware/         # 硬件驱动层：GPIO、DHT11 温湿度
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
- [ ] 多源传感器数据采集模块
- [ ] 家电设备状态监测模块
- [ ] 本地日志管理模块
- [ ] NTP 网络时间同步
- [ ] MQTT 云平台接入与多端联动
- [ ] 微信小程序 / 网页端适配
- [ ] 整机 7×24 小时稳定性测试

***

## 📄 相关文档

- [architect_design.png](./architect_design.png) — 系统架构图
- `usrcode/` 各子目录下的 `readme` — ui / data / hardware 三层职责速览

