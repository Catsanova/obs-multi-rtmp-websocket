#pragma once

#include "pch.h"
#include "output-config.h"

class PushWidget : public QWidget {
    Q_OBJECT
public:
    explicit PushWidget(QWidget* parent = nullptr);
    virtual ~PushWidget() {}
    virtual bool ShowEditDlg() = 0;
    virtual void StartStreaming() = 0;
    virtual void StopStreaming() = 0;
    virtual bool IsRunning() = 0;
    virtual void OnOBSEvent(obs_frontend_event ev) = 0;
    virtual QPushButton* GetDeleteButton() = 0;
    // When false, StartStreaming() refuses to start this target (see the
    // "enabled" field on OutputTargetConfig for why). Setting this to false
    // while the target is live also force-stops it.
    virtual bool GetEnabled() = 0;
    virtual void SetEnabled(bool enabled) = 0;

    // Accessors used by the obs-websocket vendor API (ws_vendor.cpp).
    // All must be called on the UI thread.
    virtual QString GetStatusText() const = 0;
    virtual OutputTargetConfigPtr GetConfig() const = 0;
    // Re-reads the (possibly externally modified) config into the widget.
    virtual void UpdateUI() = 0;
};

PushWidget* createPushWidget(const std::string& targetId, QWidget* parent = 0);
