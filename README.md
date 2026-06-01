# 🎥 obs-multi-rtmp (WebSocket Edition)
Adds **OBS WebSocket API support** to the [obs-multi-rtmp](https://github.com/sorayuki/obs-multi-rtmp) plugin,
enabling full remote control of RTMP targets.

------------------------------------------------------------------------

## 🚀 Features
- Full control over RTMP targets via WebSocket
- Start / Stop individual or all targets
- Dynamic target management (add, clone, delete)
- Sync with OBS streaming state
- Retrieve detailed stats per target

------------------------------------------------------------------------

## 🔌 WebSocket API

**Vendor Name:** `romestylez.multi_rtmp`

### 📡 Target Management
- `ListTargets` – Get all configured RTMP targets
- `GetTargetState` – Get current state of a specific target
- `AddTarget` – Add a new target
- `CloneTarget` – Clone an existing target
- `DeleteTarget` – Delete a target

### ▶️ Stream Control
- `StartTarget` – Start a specific target
- `StopTarget` – Stop a specific target
- `ToggleTarget` – Toggle start/stop state
- `StartAll` – Start all targets
- `StopAll` – Stop all targets

### ⚙️ Configuration
- `UpdateTargetName` – Rename a target
- `UpdateStreamKey` – Update stream key
- `UpdateServiceParam` – Update service parameters
- `UpdateSyncStart` – Sync start with OBS
- `UpdateSyncStop` – Sync stop with OBS

### 📊 Monitoring
- `GetTargetStats` – Retrieve stats for a target

------------------------------------------------------------------------

## 🧪 WebSocket Tester

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

## 📦 Installation

1. Download the latest installer (`.exe`) from [Releases](../../releases)
2. Run the installer – it handles everything automatically
3. Restart OBS
4. Check OBS log for: `Vendor registered successfully: romestylez.multi_rtmp`

> Alternatively, download the `.zip` and manually copy `obs-plugins/64bit/obs-multi-rtmp.dll` to `C:\ProgramData\obs-studio\plugins\obs-multi-rtmp\bin\64bit\`

------------------------------------------------------------------------

## 🛠️ Build Instructions (via GitHub Actions)

Push to this repo and run the **Push** workflow manually via GitHub Actions.
The built ZIP and installer will appear as workflow artifacts.

------------------------------------------------------------------------

## Linux build

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

## 📚 Based on

- [obs-multi-rtmp](https://github.com/sorayuki/obs-multi-rtmp) by SoraYuki
- [obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate)
- WebSocket fork originally by [davidcool](https://github.com/davidcool/obs-multi-rtmp-websocket-support)

------------------------------------------------------------------------

## 💡 Notes

- WebSocket API extends OBS functionality significantly
- Designed for automation & multi-stream setups
- Compatible with existing obs-multi-rtmp workflows
- Works great with Streamer.bot, PocketChat, or any OBS WebSocket client
