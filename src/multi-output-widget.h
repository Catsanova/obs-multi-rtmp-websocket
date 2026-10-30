#pragma once

#include <functional>
#include <list>
#include <memory>
#include <string>

#include <QVBoxLayout>

#include "pch.h"
#include "push-widget.h"

// Object name / dock id of the plugin dock (looked up by the websocket vendor).
inline constexpr const char* MULTI_RTMP_DOCK_ID = "obs-multi-rtmp-dock";

class MultiOutputWidget : public QWidget
{
    Q_OBJECT
public:
    MultiOutputWidget(QWidget* parent = 0);

    std::list<PushWidget*> GetAllPushWidgets();
    void SaveConfig();
    void ExportConfig();
    void ImportConfig();
    void LoadConfig();
    void OnOutputMoved(
        const QModelIndex &parent,
        int start,
        int end,
        const QModelIndex &destination,
        int row
    );

    // Websocket configuration methods. All must be called on the UI thread.
    // They edit the affected push widget in place and never rebuild the
    // whole list, so unrelated running targets are left untouched.
    bool AddNewTarget(const QString& name, const QString& protocol = "rtmp");
    bool CloneTarget(const QString& sourceId, const QString& newName, const QString& newStreamKey = "");
    bool UpdateTargetName(const QString& targetId, const QString& newName);
    bool UpdateTargetStreamKey(const QString& targetId, const QString& streamKey);
    bool UpdateTargetServiceParam(const QString& targetId, const QString& key, const QString& value);
    bool DeleteTarget(const QString& targetId);
    bool UpdateSyncStart(const QString& targetId, bool syncStart);
    bool UpdateSyncStop(const QString& targetId, bool syncStop);
    PushWidget* FindPushWidgetById(const QString& targetId);
    // Looks up by id when given, otherwise by name. Returns nullptr if neither matches.
    PushWidget* FindByIdOrName(const QString& id, const QString& name);

private:
    // Main widget of this module's dock
    QWidget* container_ = 0;
    // The layout of the root widget
    QVBoxLayout* layout_ = 0;
    // Scrollable area in case of overflows of content
    QScrollArea scroll_;
    // Widget, that contains output source widgets
    QListWidget* outputsContainer_ = 0;

    void DeletePushWidget(const std::string& targetId);
    PushWidget* AddPushWidget(const std::string& targetId);
    // Assigns a fresh id to the config, registers it and creates its widget.
    PushWidget* CreateTarget(std::shared_ptr<OutputTargetConfig> target);
    // Applies mutate to the target's config, refreshes its widget and saves.
    bool MutateTarget(const QString& targetId, std::function<void(OutputTargetConfig&)> mutate);
};
