#include "ws_vendor.hpp"
#include "push-widget.h"
#include "multi-output-widget.h"
#include "plugin-support.h"
#include <obs-websocket-api.h>
#include <functional>
#include <QCoreApplication>
#include <QMetaObject>
#include <QRegularExpression>

namespace {

constexpr const char* VENDOR_NAME = "romestylez.multi_rtmp";
constexpr int MAX_INIT_ATTEMPTS = 30;
constexpr int INIT_RETRY_INTERVAL_MS = 1000;

// Runs fn on the Qt UI thread and returns its result. Blocks the caller
// until fn has finished when called from another thread.
bool RunOnUi(const std::function<bool()>& fn)
{
    if (QThread::currentThread() == QCoreApplication::instance()->thread())
        return fn();

    bool result = false;
    QMetaObject::invokeMethod(QCoreApplication::instance(), [&]() { result = fn(); }, Qt::BlockingQueuedConnection);
    return result;
}

// UI thread only. Sets "error" on resp and returns nullptr on failure.
MultiOutputWidget* GetMultiOutputWidget(obs_data_t* resp)
{
    auto mainwin = static_cast<QMainWindow*>(obs_frontend_get_main_window());
    if (!mainwin) {
        obs_data_set_string(resp, "error", "Cannot find main window");
        return nullptr;
    }

    auto dock = mainwin->findChild<QDockWidget*>(MULTI_RTMP_DOCK_ID);
    if (!dock) {
        obs_data_set_string(resp, "error", "Multi RTMP dock not found");
        return nullptr;
    }

    auto widget = qobject_cast<MultiOutputWidget*>(dock->widget());
    if (!widget)
        obs_data_set_string(resp, "error", "Failed to cast to MultiOutputWidget");
    return widget;
}

// UI thread only. Finds the target named by the request's "id" or "name".
PushWidget* FindTarget(obs_data_t* req, obs_data_t* resp)
{
    auto widget = GetMultiOutputWidget(resp);
    if (!widget)
        return nullptr;

    auto target = widget->FindByIdOrName(
        QString::fromUtf8(obs_data_get_string(req, "id")),
        QString::fromUtf8(obs_data_get_string(req, "name")));
    if (!target)
        obs_data_set_string(resp, "error", "Target not found");
    return target;
}

void SerializeTarget(PushWidget* target, obs_data_t* data)
{
    auto config = target->GetConfig();
    obs_data_set_string(data, "id", config->id.c_str());
    obs_data_set_string(data, "name", config->name.c_str());
    obs_data_set_string(data, "state", target->IsRunning() ? "running" : "stopped");
    obs_data_set_bool(data, "enabled", target->GetEnabled());
}

double ParseBitrateValue(const QString& bitrateStr)
{
    static const QRegularExpression regex("([\\d.]+)\\s*(\\w+)ps");
    QRegularExpressionMatch match = regex.match(bitrateStr);
    if (!match.hasMatch())
        return 0;

    double value = match.captured(1).toDouble();
    QString unit = match.captured(2).toLower();
    if (unit == "g") return value * 1000000000;
    if (unit == "m") return value * 1000000;
    if (unit == "k") return value * 1000;
    return value;
}

double ParseFpsValue(const QString& fpsStr)
{
    static const QRegularExpression regex("(\\d+)\\s*FPS");
    QRegularExpressionMatch match = regex.match(fpsStr);
    return match.hasMatch() ? match.captured(1).toDouble() : 0;
}

void ParseStatusText(const QString& statusText, obs_data_t* resp)
{
    if (statusText.isEmpty()) {
        obs_data_set_string(resp, "duration", "00:00:00");
        obs_data_set_string(resp, "bitrate", "0 bps");
        obs_data_set_string(resp, "fps", "0 FPS");
        obs_data_set_double(resp, "bitrateValue", 0);
        obs_data_set_double(resp, "fpsValue", 0);
        return;
    }

    QStringList parts = statusText.split("  ", Qt::SplitBehaviorFlags::SkipEmptyParts);
    if (parts.size() < 3)
        return;

    obs_data_set_string(resp, "duration", parts[0].toUtf8().constData());
    obs_data_set_string(resp, "bitrate", parts[1].toUtf8().constData());
    obs_data_set_double(resp, "bitrateValue", ParseBitrateValue(parts[1]));
    obs_data_set_string(resp, "fps", parts[2].toUtf8().constData());
    obs_data_set_double(resp, "fpsValue", ParseFpsValue(parts[2]));
}

bool HandleListTargets(obs_data_t*, obs_data_t* resp)
{
    return RunOnUi([&]() {
        auto widget = GetMultiOutputWidget(resp);
        if (!widget)
            return false;

        auto targets = obs_data_array_create();
        for (auto* pushWidget : widget->GetAllPushWidgets()) {
            auto item = obs_data_create();
            SerializeTarget(pushWidget, item);
            obs_data_array_push_back(targets, item);
            obs_data_release(item);
        }
        obs_data_set_array(resp, "targets", targets);
        obs_data_array_release(targets);
        return true;
    });
}

bool HandleGetTargetState(obs_data_t* req, obs_data_t* resp)
{
    return RunOnUi([&]() {
        auto target = FindTarget(req, resp);
        if (!target)
            return false;
        SerializeTarget(target, resp);
        return true;
    });
}

bool HandleGetTargetStats(obs_data_t* req, obs_data_t* resp)
{
    return RunOnUi([&]() {
        auto target = FindTarget(req, resp);
        if (!target)
            return false;

        QString statusText = target->GetStatusText();
        ParseStatusText(statusText, resp);
        SerializeTarget(target, resp);
        obs_data_set_bool(resp, "isRunning", target->IsRunning());
        obs_data_set_string(resp, "rawStatus", statusText.toUtf8().constData());
        return true;
    });
}

bool HandleStartTarget(obs_data_t* req, obs_data_t* resp)
{
    return RunOnUi([&]() {
        auto target = FindTarget(req, resp);
        if (!target)
            return false;

        // StartStreaming() refuses to start disabled targets; report that
        // instead of claiming a start was requested.
        if (!target->GetEnabled()) {
            obs_data_set_string(resp, "error", "Target is disabled");
            return false;
        }

        target->StartStreaming();
        obs_data_set_string(resp, "status", "start_requested");
        obs_data_set_string(resp, "id", target->GetConfig()->id.c_str());
        return true;
    });
}

bool HandleStopTarget(obs_data_t* req, obs_data_t* resp)
{
    return RunOnUi([&]() {
        auto target = FindTarget(req, resp);
        if (!target)
            return false;

        target->StopStreaming();
        obs_data_set_string(resp, "status", "stop_requested");
        obs_data_set_string(resp, "id", target->GetConfig()->id.c_str());
        return true;
    });
}

bool HandleToggleTarget(obs_data_t* req, obs_data_t* resp)
{
    return RunOnUi([&]() {
        auto target = FindTarget(req, resp);
        if (!target)
            return false;

        const bool isRunning = target->IsRunning();
        if (!isRunning && !target->GetEnabled()) {
            obs_data_set_string(resp, "error", "Target is disabled");
            return false;
        }

        if (isRunning)
            target->StopStreaming();
        else
            target->StartStreaming();

        obs_data_set_string(resp, "status", isRunning ? "stop_requested" : "start_requested");
        obs_data_set_string(resp, "id", target->GetConfig()->id.c_str());
        return true;
    });
}

bool HandleStartAll(obs_data_t*, obs_data_t* resp)
{
    return RunOnUi([&]() {
        auto widget = GetMultiOutputWidget(resp);
        if (!widget)
            return false;

        // Disabled targets are skipped by StartStreaming() itself.
        int targetCount = 0;
        int enabledCount = 0;
        for (auto* target : widget->GetAllPushWidgets()) {
            ++targetCount;
            if (target->GetEnabled())
                ++enabledCount;
            target->StartStreaming();
        }

        obs_data_set_string(resp, "status", "start_all_requested");
        obs_data_set_int(resp, "target_count", targetCount);
        obs_data_set_int(resp, "enabled_count", enabledCount);
        return true;
    });
}

bool HandleStopAll(obs_data_t*, obs_data_t* resp)
{
    return RunOnUi([&]() {
        auto widget = GetMultiOutputWidget(resp);
        if (!widget)
            return false;

        int targetCount = 0;
        for (auto* target : widget->GetAllPushWidgets()) {
            ++targetCount;
            target->StopStreaming();
        }

        obs_data_set_string(resp, "status", "stop_all_requested");
        obs_data_set_int(resp, "target_count", targetCount);
        return true;
    });
}

// Runs a configuration change against the dock widget on the UI thread and
// reports okStatus or failError on resp.
bool RunConfigChange(obs_data_t* resp, const char* okStatus, const char* failError,
                     const std::function<bool(MultiOutputWidget*)>& change)
{
    return RunOnUi([&]() {
        auto widget = GetMultiOutputWidget(resp);
        if (!widget)
            return false;

        const bool ok = change(widget);
        if (ok)
            obs_data_set_string(resp, "status", okStatus);
        else
            obs_data_set_string(resp, "error", failError);
        return ok;
    });
}

bool HandleAddTarget(obs_data_t* req, obs_data_t* resp)
{
    const char* name = obs_data_get_string(req, "name");
    const char* protocol = obs_data_get_string(req, "protocol");

    if (!*name) {
        obs_data_set_string(resp, "error", "Missing name parameter");
        return false;
    }

    const QString nameStr = QString::fromUtf8(name);
    const QString protocolStr = *protocol ? QString::fromUtf8(protocol) : QString("RTMP");
    return RunConfigChange(resp, "target_added", "Failed to add target", [&](MultiOutputWidget* w) {
        return w->AddNewTarget(nameStr, protocolStr);
    });
}

bool HandleCloneTarget(obs_data_t* req, obs_data_t* resp)
{
    const char* sourceId = obs_data_get_string(req, "sourceId");
    const char* newName = obs_data_get_string(req, "newName");
    const char* newStreamKey = obs_data_get_string(req, "newStreamKey");

    if (!*sourceId || !*newName) {
        obs_data_set_string(resp, "error", "Missing sourceId or newName parameter");
        return false;
    }

    const QString sourceIdStr = QString::fromUtf8(sourceId);
    const QString newNameStr = QString::fromUtf8(newName);
    const QString streamKeyStr = QString::fromUtf8(newStreamKey);
    return RunConfigChange(resp, "target_cloned", "Failed to clone target", [&](MultiOutputWidget* w) {
        return w->CloneTarget(sourceIdStr, newNameStr, streamKeyStr);
    });
}

bool HandleUpdateTargetName(obs_data_t* req, obs_data_t* resp)
{
    const char* targetId = obs_data_get_string(req, "targetId");
    const char* newName = obs_data_get_string(req, "newName");

    if (!*targetId || !*newName) {
        obs_data_set_string(resp, "error", "Missing targetId or newName parameter");
        return false;
    }

    const QString targetIdStr = QString::fromUtf8(targetId);
    const QString newNameStr = QString::fromUtf8(newName);
    return RunConfigChange(resp, "name_updated", "Failed to update name", [&](MultiOutputWidget* w) {
        return w->UpdateTargetName(targetIdStr, newNameStr);
    });
}

bool HandleUpdateStreamKey(obs_data_t* req, obs_data_t* resp)
{
    const char* targetId = obs_data_get_string(req, "targetId");
    const char* streamKey = obs_data_get_string(req, "streamKey");

    if (!*targetId || !*streamKey) {
        obs_data_set_string(resp, "error", "Missing targetId or streamKey parameter");
        return false;
    }

    const QString targetIdStr = QString::fromUtf8(targetId);
    const QString streamKeyStr = QString::fromUtf8(streamKey);
    return RunConfigChange(resp, "stream_key_updated", "Failed to update stream key", [&](MultiOutputWidget* w) {
        return w->UpdateTargetStreamKey(targetIdStr, streamKeyStr);
    });
}

bool HandleUpdateServiceParam(obs_data_t* req, obs_data_t* resp)
{
    const char* targetId = obs_data_get_string(req, "targetId");
    const char* key = obs_data_get_string(req, "key");
    const char* value = obs_data_get_string(req, "value");

    if (!*targetId || !*key || !*value) {
        obs_data_set_string(resp, "error", "Missing targetId, key, or value parameter");
        return false;
    }

    const QString targetIdStr = QString::fromUtf8(targetId);
    const QString keyStr = QString::fromUtf8(key);
    const QString valueStr = QString::fromUtf8(value);
    return RunConfigChange(resp, "service_param_updated", "Failed to update service parameter", [&](MultiOutputWidget* w) {
        return w->UpdateTargetServiceParam(targetIdStr, keyStr, valueStr);
    });
}

bool HandleDeleteTarget(obs_data_t* req, obs_data_t* resp)
{
    const char* targetId = obs_data_get_string(req, "targetId");

    if (!*targetId) {
        obs_data_set_string(resp, "error", "Missing targetId parameter");
        return false;
    }

    const QString targetIdStr = QString::fromUtf8(targetId);
    return RunConfigChange(resp, "target_deleted", "Failed to delete target", [&](MultiOutputWidget* w) {
        return w->DeleteTarget(targetIdStr);
    });
}

bool HandleUpdateSyncStart(obs_data_t* req, obs_data_t* resp)
{
    const char* targetId = obs_data_get_string(req, "targetId");
    const bool syncStart = obs_data_get_bool(req, "syncStart");

    if (!*targetId) {
        obs_data_set_string(resp, "error", "Missing targetId parameter");
        return false;
    }

    const QString targetIdStr = QString::fromUtf8(targetId);
    return RunOnUi([&]() {
        auto widget = GetMultiOutputWidget(resp);
        return widget && widget->UpdateSyncStart(targetIdStr, syncStart);
    });
}

bool HandleUpdateSyncStop(obs_data_t* req, obs_data_t* resp)
{
    const char* targetId = obs_data_get_string(req, "targetId");
    const bool syncStop = obs_data_get_bool(req, "syncStop");

    if (!*targetId) {
        obs_data_set_string(resp, "error", "Missing targetId parameter");
        return false;
    }

    const QString targetIdStr = QString::fromUtf8(targetId);
    return RunOnUi([&]() {
        auto widget = GetMultiOutputWidget(resp);
        return widget && widget->UpdateSyncStop(targetIdStr, syncStop);
    });
}

using RequestHandler = bool (*)(obs_data_t* req, obs_data_t* resp);

// Adapts a uniform handler to the obs-websocket callback signature and
// reports its result as the "success" field.
template <RequestHandler Handler>
void Trampoline(obs_data_t* req, obs_data_t* resp, void*)
{
    obs_data_set_bool(resp, "success", Handler(req, resp));
}

struct VendorRequest {
    const char* name;
    obs_websocket_request_callback_function callback;
};

const VendorRequest VENDOR_REQUESTS[] = {
    {"ListTargets", &Trampoline<HandleListTargets>},
    {"GetTargetState", &Trampoline<HandleGetTargetState>},
    {"StartTarget", &Trampoline<HandleStartTarget>},
    {"StopTarget", &Trampoline<HandleStopTarget>},
    {"ToggleTarget", &Trampoline<HandleToggleTarget>},
    {"StartAll", &Trampoline<HandleStartAll>},
    {"StopAll", &Trampoline<HandleStopAll>},
    {"AddTarget", &Trampoline<HandleAddTarget>},
    {"CloneTarget", &Trampoline<HandleCloneTarget>},
    {"UpdateTargetName", &Trampoline<HandleUpdateTargetName>},
    {"UpdateStreamKey", &Trampoline<HandleUpdateStreamKey>},
    {"UpdateServiceParam", &Trampoline<HandleUpdateServiceParam>},
    {"DeleteTarget", &Trampoline<HandleDeleteTarget>},
    {"UpdateSyncStart", &Trampoline<HandleUpdateSyncStart>},
    {"UpdateSyncStop", &Trampoline<HandleUpdateSyncStop>},
    {"GetTargetStats", &Trampoline<HandleGetTargetStats>},
};

} // namespace

MultiRTMPWebsocketVendor* MultiRTMPWebsocketVendor::s_instance = nullptr;

MultiRTMPWebsocketVendor* MultiRTMPWebsocketVendor::Instance() {
    if (!s_instance) {
        s_instance = new MultiRTMPWebsocketVendor();
    }
    return s_instance;
}

bool MultiRTMPWebsocketVendor::Initialize() {
#ifdef ENABLE_WEBSOCKET
    blog(LOG_INFO, TAG "Websocket support enabled, attempting to register vendor");

    if (!obs_websocket_ensure_ph()) {
        if (++m_initAttempts >= MAX_INIT_ATTEMPTS) {
            blog(LOG_ERROR, TAG "obs-websocket not available after %d attempts, giving up", m_initAttempts);
            return false;
        }

        blog(LOG_WARNING, TAG "obs-websocket proc handler not available yet");
        QTimer::singleShot(INIT_RETRY_INTERVAL_MS, []() {
            blog(LOG_INFO, TAG "Retrying vendor registration...");
            MultiRTMPWebsocketVendor::Instance()->Initialize();
        });
        return false;
    }

    obs_websocket_vendor vendor = obs_websocket_register_vendor(VENDOR_NAME);
    if (!vendor) {
        blog(LOG_ERROR, TAG "Failed to register vendor: %s", VENDOR_NAME);
        return false;
    }
    blog(LOG_INFO, TAG "Vendor registered successfully: %s", VENDOR_NAME);

    bool allRegistered = true;
    for (const auto& request : VENDOR_REQUESTS) {
        if (!obs_websocket_vendor_register_request(vendor, request.name, request.callback, nullptr)) {
            blog(LOG_ERROR, TAG "Failed to register vendor request: %s", request.name);
            allRegistered = false;
        }
    }
    if (allRegistered)
        blog(LOG_INFO, TAG "All vendor requests registered successfully");

    return true;
#else
    blog(LOG_WARNING, TAG "Websocket support not compiled in");
    return false;
#endif
}

void MultiRTMPWebsocketVendor::Shutdown() {
    delete s_instance;
    s_instance = nullptr;
}
