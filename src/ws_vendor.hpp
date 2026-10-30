#pragma once

class MultiRTMPWebsocketVendor {
public:
    static MultiRTMPWebsocketVendor* Instance();

    bool Initialize();
    void Shutdown();

private:
    MultiRTMPWebsocketVendor() = default;
    ~MultiRTMPWebsocketVendor() = default;

    static MultiRTMPWebsocketVendor* s_instance;
    int m_initAttempts = 0;
};
