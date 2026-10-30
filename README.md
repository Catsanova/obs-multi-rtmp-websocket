# [Homepage / 主页](https://sorayuki.github.io/obs-multi-rtmp)

## 为什么首页是日语？ / Why is the homepage in Japanese?

因为最初是做给管人用的。

Because it's originally made for virtual Youtubers (VTubers).

# 声明 

近日发现百度贴吧有个叫 maggot 的用户在售卖此插件。咸鱼上也有，没得救了。 

本插件免费使用，作者不收取费用。 

举报之后百度贴吧找我要软件著作权证明，累不爱。 


# Announcement

This plugin is provided for free, without a fee. 

Recently a Baidu Tieba account 'maggot' is selling this plugin. Please, don't buy it.


# お知らせ

本プラグインは無償で提供されるものです。

最近、Baidu Tiebaに「maggot」というアカウント名のユーザーがこのプラグインを販売する行為をしています。

決して購入はしないでください。


# Donate

如果你觉得这个工具很有用想要捐赠，这里是链接。注意：这不是提需求的渠道。

このツールの開発に支援もとい投げ銭をしたいと思った方は以下のリンクからお願いします。(機能のリクエストは受け付けていません)

If you find this tool useful and want to doante, here is the link. (Please do not donate for feature requests.)

## [paypal / 贝宝](https://paypal.me/sorayuki0)

## alipay / 支付宝

![alipay](./docs/zhi.png) 

## wechat / 微信
![wechat](./docs/wechat.jpg)

## Build

This project uses obs-plugintemplate.   
Please refer to obs-plugintemplate to understand how it works.


# WebSocket API (fork additions)

This fork adds an obs-websocket vendor API (`romestylez.multi_rtmp`) for remote control of targets. Requests that start a target respect the per-target "enabled" flag: disabled targets are not started and `StartTarget`/`ToggleTarget` report an error.

### 🚀 Features
- Full control over RTMP targets via WebSocket
- Start / Stop individual or all targets
- Dynamic target management (add, clone, delete)
- Sync with OBS streaming state
- Retrieve detailed stats per target

------------------------------------------------------------------------

### 🔌 WebSocket API

**Vendor Name:** `romestylez.multi_rtmp`

#### 📡 Target Management
- `ListTargets` – Get all configured RTMP targets
- `GetTargetState` – Get current state of a specific target
- `AddTarget` – Add a new target
- `CloneTarget` – Clone an existing target
- `DeleteTarget` – Delete a target

#### ▶️ Stream Control
- `StartTarget` – Start a specific target
- `StopTarget` – Stop a specific target
- `ToggleTarget` – Toggle start/stop state
- `StartAll` – Start all targets
- `StopAll` – Stop all targets

#### ⚙️ Configuration
- `UpdateTargetName` – Rename a target
- `UpdateStreamKey` – Update stream key
- `UpdateServiceParam` – Update service parameters
- `UpdateSyncStart` – Sync start with OBS
- `UpdateSyncStop` – Sync stop with OBS

#### 📊 Monitoring
- `GetTargetStats` – Retrieve stats for a target

------------------------------------------------------------------------

### 🧪 WebSocket Tester

A standalone HTML tester is included (`obs-websocket-tester.html`) that lets you test all API calls directly from your browser – no extra tools needed.

**Features:**
- Connects to OBS WebSocket with automatic authentication
- Dropdown for all available requests
- Shows raw sent JSON and received response side by side
- Copy buttons for easy use in Streamer.bot or other tools

**Usage:**
1. Copy `obs-websocket-tester.html` to your OBS machine
2. Open it in any browser (double-click)
3. Enter your OBS WebSocket host, port and password
4. Click **Connect** and start sending requests

**Example – Stop the Kick target:**

```json
{
  "op": 6,
  "d": {
    "requestType": "CallVendorRequest",
    "requestId": "my-request-1",
    "requestData": {
      "vendorName": "romestylez.multi_rtmp",
      "requestType": "StopTarget",
      "requestData": {
        "name": "Kick"
      }
    }
  }
}
```

------------------------------------------------------------------------

### Linux build

Ubuntu 24.04 provides OBS packages new enough to build this plugin.

```bash
apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y \
  cmake ninja-build pkg-config jq git ccache zsh build-essential \
  qt6-base-dev libobs-dev obs-studio

cmake --preset ubuntu-x86_64
cmake --build --preset ubuntu-x86_64 -j2
```

Expected output:

```text
build_x86_64/obs-multi-rtmp.so
```

You can also run the package-based build in Docker:

```bash
docker build -f Dockerfile.linux-build -t obs-multi-rtmp-linux-build .
```

Runtime loading was smoke-tested on OBS 32.1.2 from the official OBS Project
PPA, including websocket vendor registration for `romestylez.multi_rtmp`.

------------------------------------------------------------------------

