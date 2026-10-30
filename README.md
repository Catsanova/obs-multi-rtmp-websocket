# obs-multi-rtmp-websocket

This is Catsanova/obs-multi-rtmp-websocket, a fork of
[romestylez/obs-multi-rtmp-websocket](https://github.com/romestylez/obs-multi-rtmp-websocket),
which is itself a fork of [sorayuki/obs-multi-rtmp](https://github.com/sorayuki/obs-multi-rtmp).

This fork adds an obs-websocket vendor API (`romestylez.multi_rtmp`) to control
streaming targets remotely. The plugin is licensed under GPL-2.0, the same as
upstream. Credit goes to romestylez and sorayuki for the original work. The
upstream README follows below.

------------------------------------------------------------------------

# Requirements

- OBS Studio 33.0.1 (see `buildspec.json`).
- The plugin is installed in the OBS 33 legacy plugin location
  (`obs-studio\plugins`). This location still works until OBS 34.
- Third-party plugins and the obs-websocket plugin are not loaded when OBS
  runs in Safe Mode. The WebSocket API is unavailable in Safe Mode.

------------------------------------------------------------------------

# Install

## Windows

- Run `obs-multi-rtmp-setup.exe`, or
- Extract the zip into the OBS folder (for example `C:\Program Files\obs-studio`).

Restart OBS after installing.

## Uninstall

Run the installer again and choose remove, or delete the `obs-multi-rtmp`
folder from the plugins directory.

------------------------------------------------------------------------

# Build

## Windows (local)

Requirements: Visual Studio 2022 with the C++ workload, and CMake.

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64 --config RelWithDebInfo
```

## GitHub Actions

The `Push` workflow builds the plugin. To run it manually, open
Actions, select **Push**, and choose **Run workflow**. Build artifacts are
attached to the run page.

## Linux (Ubuntu 24.04)

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

Runtime loading was last smoke-tested on OBS 32.1.2 from the official OBS
Project PPA, including websocket vendor registration for `romestylez.multi_rtmp`.

------------------------------------------------------------------------

# WebSocket API

The fork registers a vendor named `romestylez.multi_rtmp` with obs-websocket.
Send requests with `CallVendorRequest`.

Target identifiers: requests that take a target accept `id` or `name`. Requests
that modify a target take `targetId`.

Responses set `success` (boolean). On failure, the response includes an
`error` string. The fields below are returned on success unless noted.

## Target state

- Disabled targets are not started. `StartTarget` and `ToggleTarget` (when it
  would start) return `success: false` with `error: "Target is disabled"`.
- `StopTarget` and `StopAll` do not check the enabled flag.
- `StartAll` skips disabled targets and returns `enabled_count`.

## Requests

| Request | Request fields | Response fields |
|---|---|---|
| `ListTargets` | none | `targets`: array of `{id, name, state, enabled}` |
| `GetTargetState` | `id` or `name` | `id`, `name`, `state`, `enabled` |
| `GetTargetStats` | `id` or `name` | `id`, `name`, `state`, `enabled`, `isRunning`, `rawStatus`, `duration`, `bitrate`, `bitrateValue`, `fps`, `fpsValue` |
| `StartTarget` | `id` or `name` | `status: "start_requested"`, `id` |
| `StopTarget` | `id` or `name` | `status: "stop_requested"`, `id` |
| `ToggleTarget` | `id` or `name` | `status: "start_requested"` or `"stop_requested"`, `id` |
| `StartAll` | none | `status: "start_all_requested"`, `target_count`, `enabled_count` |
| `StopAll` | none | `status: "stop_all_requested"`, `target_count` |
| `AddTarget` | `name` (required), `protocol` (optional, default `RTMP`) | `status: "target_added"` |
| `CloneTarget` | `sourceId`, `newName`, `newStreamKey` (optional) | `status: "target_cloned"` |
| `UpdateTargetName` | `targetId`, `newName` | `status: "name_updated"` |
| `UpdateStreamKey` | `targetId`, `streamKey` | `status: "stream_key_updated"` |
| `UpdateServiceParam` | `targetId`, `key`, `value` | `status: "service_param_updated"` |
| `DeleteTarget` | `targetId` | `status: "target_deleted"` |
| `UpdateSyncStart` | `targetId`, `syncStart` (boolean) | none beyond `success` |
| `UpdateSyncStop` | `targetId`, `syncStop` (boolean) | none beyond `success` |

Notes:

- `StartTarget`, `StopTarget`, `ToggleTarget`, `StartAll` and `StopAll` only
  request the change. `status` confirms the request was accepted, not that the
  stream is live.
- `GetTargetStats` parses the status text shown in the UI. The stats fields
  may be missing if that text is not in the expected format.
- Errors include `Target not found`, `Missing ... parameter`, and
  `Failed to ...` messages.

## Protocols

`AddTarget` and `protocol` accept:

| Value (case-insensitive) | Stored as |
|---|---|
| empty or omitted, `rtmp`, `rtmps` | `RTMP` |
| `srt`, `rist`, `srt/rist` | `SRT_RIST` |
| `WHIP` | `WHIP` |

Any other value fails with `Failed to add target`.

The stream key is stored in service parameter `key`, except for `WHIP`, which
uses `bearer_token`. `UpdateStreamKey` and `CloneTarget` (`newStreamKey`) write
to the parameter for the target's protocol. `UpdateServiceParam` writes any
service parameter by name.

## Example

Stop the target named `Kick`:

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

## WebSocket tester

`obs-websocket-tester.html` is a standalone page for sending these requests
from a browser.

1. Open the file in a browser.
2. Enter the OBS WebSocket host, port (default 4455) and password.
3. Click **Connect**, pick a request, and send it.

The raw sent and received JSON are shown side by side, with copy buttons.
The request dropdown lists a subset of the requests above.

------------------------------------------------------------------------

# Upstream README

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
