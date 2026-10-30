#include "pch.h"

#include <functional>
#include <list>

#include "push-widget.h"
#include "plugin-support.h"

#include "output-config.h"
#include "multi-output-widget.h"
#include "ws_vendor.hpp"

#define ConfigSection "obs-multi-rtmp"

static class GlobalServiceImpl : public GlobalService
{
public:
    bool RunInUIThread(std::function<void()> task) override {
        if (uiThread_ == nullptr)
            return false;
        QMetaObject::invokeMethod(uiThread_, [func = std::move(task)]() {
            func();
        });
        return true;
    }

    QThread* uiThread_ = nullptr;
} s_service;


GlobalService& GetGlobalService() {
    return s_service;
}


OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-multi-rtmp", "en-US")
OBS_MODULE_AUTHOR("雷鳴 (@sorayukinoyume)")

bool obs_module_load()
{
    auto mainwin = (QMainWindow*)obs_frontend_get_main_window();
    if (mainwin == nullptr)
        return false;
    QMetaObject::invokeMethod(mainwin, []() {
        s_service.uiThread_ = QThread::currentThread();
    });

    // Only create the instance here; vendor registration has to wait until
    // obs-websocket is loaded (see OBS_FRONTEND_EVENT_FINISHED_LOADING below).
    MultiRTMPWebsocketVendor::Instance();

    auto dock = new MultiOutputWidget();
    dock->setObjectName(MULTI_RTMP_DOCK_ID);
    if (!obs_frontend_add_dock_by_id(MULTI_RTMP_DOCK_ID, obs_module_text("Title"), dock))
    {
        delete dock;
        return false;
    }

    obs_hotkey_register_frontend(
        "obs-multi-rtmp.start_all",
        obs_module_text("Hotkey.StartAllTargets"),
        [](void* private_data, obs_hotkey_id, obs_hotkey_t*, bool pressed) {
            auto dock = static_cast<MultiOutputWidget*>(private_data);
            if (!pressed || !dock)
                return;
            for (auto x : dock->GetAllPushWidgets())
                x->StartStreaming();
        },
        dock
    );
    obs_hotkey_register_frontend(
        "obs-multi-rtmp.stop_all",
        obs_module_text("Hotkey.StopAllTargets"),
        [](void* private_data, obs_hotkey_id, obs_hotkey_t*, bool pressed) {
            auto dock = static_cast<MultiOutputWidget*>(private_data);
            if (!pressed || !dock)
                return;
            for (auto x : dock->GetAllPushWidgets())
                x->StopStreaming();
        },
        dock
    );

    blog(LOG_INFO, TAG "version: %s by SoraYuki https://github.com/sorayuki/obs-multi-rtmp/", PLUGIN_VERSION);

    obs_frontend_add_event_callback(
        [](enum obs_frontend_event event, void *private_data) {
            auto dock = static_cast<MultiOutputWidget*>(private_data);

            for(auto x: dock->GetAllPushWidgets())
                x->OnOBSEvent(event);

            if (event == obs_frontend_event::OBS_FRONTEND_EVENT_EXIT)
            {   
                dock->SaveConfig();
            }
            else if (event == obs_frontend_event::OBS_FRONTEND_EVENT_PROFILE_CHANGED)
            {
                dock->LoadConfig();
            }
            else if (event == obs_frontend_event::OBS_FRONTEND_EVENT_FINISHED_LOADING)
            {
                // obs-websocket is available now; register the vendor requests.
                MultiRTMPWebsocketVendor::Instance()->Initialize();
            }
        }, dock
    );

    return true;
}

void obs_module_unload(void)
{
    MultiRTMPWebsocketVendor::Instance()->Shutdown();
}

const char *obs_module_description(void)
{
    return "Multiple RTMP Output Plugin";
}
