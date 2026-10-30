#pragma once

#include <string>
#include <string_view>

// Service parameter holding the stream key (bearer token for WHIP).
inline constexpr const char* SERVICE_PARAM_STREAM_KEY = "key";
inline constexpr const char* SERVICE_PARAM_BEARER_TOKEN = "bearer_token";

struct ProtocolInfo {
    const char* protocol;
    const char* label;
    const char* outputId;
    const char* serviceId;
};

class ProtocolInfos {
public:
    virtual const ProtocolInfo* GetInfo(const char* protocol) = 0;
    virtual const ProtocolInfo* GetList() = 0;
};

ProtocolInfos* GetProtocolInfos();
