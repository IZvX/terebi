#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <queue>
#include <string>
#include <utility>

namespace TerebiRemote
{
    struct Status
    {
        bool running = false;
        bool paired = false;
        int port = 3000;
        std::string pairingCode;
        std::string lastClient;
        std::string lastMessage;
        std::string error;
    };

    enum class CursorStyle { Arrow, IBeam, Hand };

    void Start(int port = 3000);
    void Stop();
    void ProcessEvents();
    Status GetStatus();
    std::string LocalAddress();

    bool IsRemotePointerActive();
    std::pair<int, int> RemoteCursorPosition();
    CursorStyle RemoteCursorStyle();
    uint32_t RemoteCursorLastActivityMs();
    void DeactivateRemoteCursor();
    void SetRemoteCursorStyle(CursorStyle style);
}
