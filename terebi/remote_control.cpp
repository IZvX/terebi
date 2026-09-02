#include "remote_control.h"

#include "../components/shared.h"
#include "../nimble/application.h"

#include <SDL3/SDL.h>
#include <arpa/inet.h>
#include <array>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <net/if.h>
#include <netinet/in.h>
#include <random>
#include <algorithm>
#include <sstream>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace TerebiRemote
{
    namespace
    {
        enum class EventKind { Nav, PointerMove, PointerClick, PairRequired, Paired };

        struct Event
        {
            EventKind kind = EventKind::Nav;
            std::string action;
            int dx = 0;
            int dy = 0;
        };

        std::atomic<bool> g_running{false};
        std::atomic<bool> g_paired{false};
        std::thread g_serverThread;
        std::mutex g_mutex;
        std::queue<Event> g_events;
        std::vector<int> g_clientSockets;
        int g_serverSocket = -1;
        int g_port = 3000;
        std::string g_pairingCode;
        std::string g_pairToken;
        std::string g_lastClient;
        std::string g_lastMessage;
        std::string g_error;

        std::atomic<bool> g_remoteMouseActive{false};
        std::atomic<int> g_remoteMouseX{0};
        std::atomic<int> g_remoteMouseY{0};
        std::atomic<int> g_remoteMouseCursorStyle{static_cast<int>(CursorStyle::Arrow)};
        std::atomic<uint32_t> g_remoteMouseLastActivityMs{0};

        void ActivateRemoteCursor(CursorStyle style)
        {
            g_remoteMouseActive = true;
            g_remoteMouseCursorStyle = static_cast<int>(style);
            g_remoteMouseLastActivityMs = SDL_GetTicks();
            float x = 0.0f, y = 0.0f;
            SDL_GetMouseState(&x, &y);
            g_remoteMouseX = static_cast<int>(x);
            g_remoteMouseY = static_cast<int>(y);
            SDL_HideCursor();
        }

        void UpdateRemoteCursorPosition(CursorStyle style)
        {
            g_remoteMouseCursorStyle = static_cast<int>(style);
            g_remoteMouseLastActivityMs = SDL_GetTicks();
            float x = 0.0f, y = 0.0f;
            SDL_GetMouseState(&x, &y);
            g_remoteMouseX = static_cast<int>(x);
            g_remoteMouseY = static_cast<int>(y);
            g_remoteMouseActive = true;
            SDL_HideCursor();
        }

        void DeactivateRemoteCursorInternal()
        {
            g_remoteMouseActive = false;
            g_remoteMouseLastActivityMs = 0;
            g_remoteMouseCursorStyle = static_cast<int>(CursorStyle::Arrow);
        }

        std::string RandomPairingCode()
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> dist(1000, 999999);
            return std::to_string(dist(gen));
        }

        std::string RandomToken()
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> dist(0, 15);
            std::string token;
            for (int i = 0; i < 32; ++i)
                token.push_back("0123456789abcdef"[dist(gen)]);
            return token;
        }

        std::string TokenPath()
        {
            return "terebi_remote.token";
        }

        std::string LoadToken()
        {
            std::ifstream in(TokenPath());
            std::string token;
            in >> token;
            return token;
        }

        void SaveToken(const std::string &token)
        {
            std::ofstream out(TokenPath(), std::ios::trunc);
            out << token;
        }

        void SetError(const std::string &error)
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_error = error;
        }

        void SetLastMessage(const std::string &message)
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_lastMessage = message;
        }

        void QueueEvent(const Event &event)
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_events.push(event);
        }

        void PushKey(SDL_Keycode key)
        {
            SDL_Event down = {};
            down.type = SDL_EVENT_KEY_DOWN;
            down.key.key = key;
            down.key.down = true;
            SDL_PushEvent(&down);

            SDL_Event up = {};
            up.type = SDL_EVENT_KEY_UP;
            up.key.key = key;
            up.key.down = false;
            SDL_PushEvent(&up);
        }

        void PushText(const std::string &text)
        {
            static thread_local std::vector<std::string> pendingTextChunks;
            pendingTextChunks.clear();

            size_t pos = 0;
            while (pos < text.size())
            {
                size_t end = pos;
                size_t bytes = 0;
                while (end < text.size() && bytes < 31)
                {
                    unsigned char c = static_cast<unsigned char>(text[end]);
                    size_t charLen = 1;
                    if ((c & 0xe0) == 0xc0) charLen = 2;
                    else if ((c & 0xf0) == 0xe0) charLen = 3;
                    else if ((c & 0xf8) == 0xf0) charLen = 4;

                    if (end + charLen > text.size() || bytes + charLen >= 32)
                        break;

                    end += charLen;
                    bytes += charLen;
                }

                if (end == pos)
                    ++end;

                pendingTextChunks.push_back(text.substr(pos, end - pos));
                pos = end;
            }

            for (const std::string &chunk : pendingTextChunks)
            {
                SDL_Event event = {};
                event.type = SDL_EVENT_TEXT_INPUT;
                event.text.text = chunk.c_str();
                SDL_PushEvent(&event);
            }
        }

        std::string HeaderValue(const std::string &request, const std::string &name)
        {
            const std::string needle = name + ":";
            size_t pos = request.find(needle);
            if (pos == std::string::npos)
                return "";
            pos += needle.size();
            while (pos < request.size() && request[pos] == ' ')
                ++pos;
            size_t end = request.find("\r\n", pos);
            return request.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
        }

        uint32_t LeftRotate(uint32_t value, uint32_t count)
        {
            return (value << count) | (value >> (32 - count));
        }

        std::array<uint8_t, 20> Sha1(const std::string &input)
        {
            uint64_t bitLength = static_cast<uint64_t>(input.size()) * 8;
            std::vector<uint8_t> data(input.begin(), input.end());
            data.push_back(0x80);
            while ((data.size() % 64) != 56)
                data.push_back(0);
            for (int i = 7; i >= 0; --i)
                data.push_back(static_cast<uint8_t>((bitLength >> (i * 8)) & 0xff));

            uint32_t h0 = 0x67452301;
            uint32_t h1 = 0xefcdab89;
            uint32_t h2 = 0x98badcfe;
            uint32_t h3 = 0x10325476;
            uint32_t h4 = 0xc3d2e1f0;

            for (size_t chunk = 0; chunk < data.size(); chunk += 64)
            {
                uint32_t w[80] = {};
                for (int i = 0; i < 16; ++i)
                {
                    size_t j = chunk + i * 4;
                    w[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | data[j + 3];
                }
                for (int i = 16; i < 80; ++i)
                    w[i] = LeftRotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

                uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
                for (int i = 0; i < 80; ++i)
                {
                    uint32_t f = 0;
                    uint32_t k = 0;
                    if (i < 20) { f = (b & c) | ((~b) & d); k = 0x5a827999; }
                    else if (i < 40) { f = b ^ c ^ d; k = 0x6ed9eba1; }
                    else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8f1bbcdc; }
                    else { f = b ^ c ^ d; k = 0xca62c1d6; }
                    uint32_t temp = LeftRotate(a, 5) + f + e + k + w[i];
                    e = d;
                    d = c;
                    c = LeftRotate(b, 30);
                    b = a;
                    a = temp;
                }
                h0 += a; h1 += b; h2 += c; h3 += d; h4 += e;
            }

            std::array<uint8_t, 20> out = {};
            uint32_t h[5] = {h0, h1, h2, h3, h4};
            for (int i = 0; i < 5; ++i)
            {
                out[i * 4] = static_cast<uint8_t>((h[i] >> 24) & 0xff);
                out[i * 4 + 1] = static_cast<uint8_t>((h[i] >> 16) & 0xff);
                out[i * 4 + 2] = static_cast<uint8_t>((h[i] >> 8) & 0xff);
                out[i * 4 + 3] = static_cast<uint8_t>(h[i] & 0xff);
            }
            return out;
        }

        std::string Base64(const uint8_t *data, size_t len)
        {
            static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            std::string out;
            for (size_t i = 0; i < len; i += 3)
            {
                uint32_t value = data[i] << 16;
                if (i + 1 < len) value |= data[i + 1] << 8;
                if (i + 2 < len) value |= data[i + 2];
                out.push_back(table[(value >> 18) & 63]);
                out.push_back(table[(value >> 12) & 63]);
                out.push_back(i + 1 < len ? table[(value >> 6) & 63] : '=');
                out.push_back(i + 2 < len ? table[value & 63] : '=');
            }
            return out;
        }

        bool ReadHttpRequest(int fd, std::string &request)
        {
            char buffer[1024];
            while (request.find("\r\n\r\n") == std::string::npos)
            {
                ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
                if (n <= 0)
                    return false;
                request.append(buffer, buffer + n);
                if (request.size() > 8192)
                    return false;
            }
            return true;
        }

        bool SendTextFrame(int fd, const std::string &text)
        {
            std::vector<uint8_t> frame;
            frame.push_back(0x81);
            if (text.size() < 126)
            {
                frame.push_back(static_cast<uint8_t>(text.size()));
            }
            else
            {
                frame.push_back(126);
                frame.push_back(static_cast<uint8_t>((text.size() >> 8) & 0xff));
                frame.push_back(static_cast<uint8_t>(text.size() & 0xff));
            }
            frame.insert(frame.end(), text.begin(), text.end());
            return send(fd, frame.data(), frame.size(), 0) == static_cast<ssize_t>(frame.size());
        }

        bool ReadExact(int fd, uint8_t *data, size_t len)
        {
            size_t read = 0;
            while (read < len)
            {
                ssize_t n = recv(fd, data + read, len - read, 0);
                if (n <= 0)
                    return false;
                read += static_cast<size_t>(n);
            }
            return true;
        }

        bool ReadTextFrame(int fd, std::string &message)
        {
            uint8_t header[2];
            if (!ReadExact(fd, header, 2))
                return false;
            uint8_t opcode = header[0] & 0x0f;
            if (opcode == 0x8)
                return false;
            if (opcode != 0x1)
                return true;

            bool masked = (header[1] & 0x80) != 0;
            uint64_t len = header[1] & 0x7f;
            if (len == 126)
            {
                uint8_t ext[2];
                if (!ReadExact(fd, ext, 2)) return false;
                len = (ext[0] << 8) | ext[1];
            }
            else if (len == 127)
            {
                uint8_t ext[8];
                if (!ReadExact(fd, ext, 8)) return false;
                len = 0;
                for (uint8_t b : ext) len = (len << 8) | b;
            }
            if (len > 4096)
                return false;

            uint8_t mask[4] = {};
            if (masked && !ReadExact(fd, mask, 4))
                return false;

            std::vector<uint8_t> payload(static_cast<size_t>(len));
            if (len > 0 && !ReadExact(fd, payload.data(), static_cast<size_t>(len)))
                return false;

            for (size_t i = 0; masked && i < payload.size(); ++i)
                payload[i] ^= mask[i % 4];
            message.assign(payload.begin(), payload.end());
            return true;
        }

        std::string JsonString(const std::string &json, const std::string &key)
        {
            const std::string needle = "\"" + key + "\"";
            size_t pos = json.find(needle);
            if (pos == std::string::npos) return "";
            pos = json.find(':', pos);
            if (pos == std::string::npos) return "";
            pos = json.find('"', pos);
            if (pos == std::string::npos) return "";

            std::string out;
            for (size_t i = pos + 1; i < json.size(); ++i)
            {
                char c = json[i];
                if (c == '"')
                    return out;
                if (c != '\\')
                {
                    out.push_back(c);
                    continue;
                }

                if (++i >= json.size())
                    break;
                char escaped = json[i];
                if (escaped == '"' || escaped == '\\' || escaped == '/')
                    out.push_back(escaped);
                else if (escaped == 'b')
                    out.push_back('\b');
                else if (escaped == 'f')
                    out.push_back('\f');
                else if (escaped == 'n')
                    out.push_back('\n');
                else if (escaped == 'r')
                    out.push_back('\r');
                else if (escaped == 't')
                    out.push_back('\t');
            }
            return out;
        }

        int JsonInt(const std::string &json, const std::string &key)
        {
            const std::string needle = "\"" + key + "\"";
            size_t pos = json.find(needle);
            if (pos == std::string::npos) return 0;
            pos = json.find(':', pos);
            if (pos == std::string::npos) return 0;
            ++pos;
            while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
            return std::atoi(json.c_str() + pos);
        }

        bool HandleMessage(int fd, const std::string &message, bool &paired)
        {
            const std::string type = JsonString(message, "type");
            const std::string action = JsonString(message, "action");
            SetLastMessage(message);

            if (!paired)
            {
                if (type == "system" && action == "request_pair")
                {
                    QueueEvent({EventKind::PairRequired, "", 0, 0});
                    SendTextFrame(fd, "{\"type\":\"system\",\"action\":\"pair_required\"}");
                    return true;
                }

                const std::string token = JsonString(message, "token");
                if (type == "system" && action == "connect" && !g_pairToken.empty() && token == g_pairToken)
                {
                    paired = true;
                    g_paired = true;
                    QueueEvent({EventKind::Paired, "", 0, 0});
                    SendTextFrame(fd, "{\"type\":\"system\",\"action\":\"paired\",\"token\":\"" + g_pairToken + "\"}");
                    return true;
                }

                if (type == "system" && action == "connect" && JsonString(message, "code") == g_pairingCode)
                {
                    paired = true;
                    g_paired = true;
                    g_pairToken = RandomToken();
                    SaveToken(g_pairToken);
                    QueueEvent({EventKind::Paired, "", 0, 0});
                    SendTextFrame(fd, "{\"type\":\"system\",\"action\":\"paired\",\"token\":\"" + g_pairToken + "\"}");
                    return true;
                }
                SendTextFrame(fd, "{\"type\":\"system\",\"action\":\"rejected\"}");
                return false;
            }

            if (type == "nav")
            {
                QueueEvent({EventKind::Nav, action, 0, 0});
                return true;
            }
            if (type == "pointer" && action == "click")
            {
                QueueEvent({EventKind::PointerClick, action, 0, 0});
                return true;
            }
            if (type == "pointer")
            {
                QueueEvent({EventKind::PointerMove, "", JsonInt(message, "dx"), JsonInt(message, "dy")});
                return true;
            }
            if (type == "keyboard")
            {
                if (action == "backspace") QueueEvent({EventKind::Nav, "keyboard_backspace", 0, 0});
                else if (action == "enter") QueueEvent({EventKind::Nav, "keyboard_enter", 0, 0});
                else QueueEvent({EventKind::Nav, "keyboard_text:" + JsonString(message, "text"), 0, 0});
                return true;
            }
            return true;
        }

        void HandleClient(int fd, std::string clientAddress)
        {
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                g_clientSockets.push_back(fd);
                g_lastClient = clientAddress;
            }

            std::string request;
            if (!ReadHttpRequest(fd, request))
            {
                close(fd);
                return;
            }

            const std::string key = HeaderValue(request, "Sec-WebSocket-Key");
            if (key.empty())
            {
                close(fd);
                return;
            }

            const std::string acceptInput = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
            const auto digest = Sha1(acceptInput);
            const std::string accept = Base64(digest.data(), digest.size());
            const std::string response =
                "HTTP/1.1 101 Switching Protocols\r\n"
                "Upgrade: websocket\r\n"
                "Connection: Upgrade\r\n"
                "Sec-WebSocket-Accept: " + accept + "\r\n\r\n";
            send(fd, response.c_str(), response.size(), 0);

            bool paired = false;
            while (g_running)
            {
                std::string message;
                if (!ReadTextFrame(fd, message))
                    break;
                if (!message.empty() && !HandleMessage(fd, message, paired))
                    break;
            }
            if (paired)
                g_paired = false;
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                g_clientSockets.erase(std::remove(g_clientSockets.begin(), g_clientSockets.end(), fd), g_clientSockets.end());
            }
            close(fd);
        }

        void ServerLoop(int port)
        {
            g_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (g_serverSocket < 0)
            {
                SetError("Could not create socket");
                g_running = false;
                return;
            }

            int yes = 1;
            setsockopt(g_serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

            sockaddr_in addr = {};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(static_cast<uint16_t>(port));

            if (bind(g_serverSocket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
            {
                SetError("Port " + std::to_string(port) + " is unavailable");
                close(g_serverSocket);
                g_serverSocket = -1;
                g_running = false;
                return;
            }

            if (listen(g_serverSocket, 8) < 0)
            {
                SetError("Could not listen for remotes");
                close(g_serverSocket);
                g_serverSocket = -1;
                g_running = false;
                return;
            }

            while (g_running)
            {
                sockaddr_in client = {};
                socklen_t len = sizeof(client);
                int fd = accept(g_serverSocket, reinterpret_cast<sockaddr *>(&client), &len);
                if (fd < 0)
                    continue;
                std::string address = inet_ntoa(client.sin_addr);
                std::thread(HandleClient, fd, address).detach();
            }
        }
    }

    void Start(int port)
    {
        if (g_running)
            return;
        if (g_serverThread.joinable())
            g_serverThread.join();

        g_port = port;
        g_pairingCode = RandomPairingCode();
        g_pairToken = LoadToken();
        g_error.clear();
        g_lastMessage.clear();
        g_lastClient.clear();
        g_paired = false;
        g_running = true;
        g_serverThread = std::thread(ServerLoop, port);
    }

    void Stop()
    {
        g_running = false;
        if (g_serverSocket >= 0)
        {
            shutdown(g_serverSocket, SHUT_RDWR);
            close(g_serverSocket);
            g_serverSocket = -1;
        }

        {
            std::lock_guard<std::mutex> lock(g_mutex);
            for (int fd : g_clientSockets)
            {
                shutdown(fd, SHUT_RDWR);
            }
            g_clientSockets.clear();
        }

        if (g_serverThread.joinable())
            g_serverThread.join();
        g_paired = false;
        DeactivateRemoteCursorInternal();
        SDL_ShowCursor();
    }

    bool IsRemotePointerActive()
    {
        return g_remoteMouseActive.load();
    }

    std::pair<int, int> RemoteCursorPosition()
    {
        return { g_remoteMouseX.load(), g_remoteMouseY.load() };
    }

    CursorStyle RemoteCursorStyle()
    {
        return static_cast<CursorStyle>(g_remoteMouseCursorStyle.load());
    }

    uint32_t RemoteCursorLastActivityMs()
    {
        return g_remoteMouseLastActivityMs.load();
    }

    void DeactivateRemoteCursor()
    {
        DeactivateRemoteCursorInternal();
        SDL_ShowCursor();
    }

    void SetRemoteCursorStyle(CursorStyle style)
    {
        g_remoteMouseCursorStyle = static_cast<int>(style);
    }

    void ProcessEvents()
    {
        std::queue<Event> events;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            std::swap(events, g_events);
        }

        while (!events.empty())
        {
            Event event = events.front();
            events.pop();

            if (event.kind == EventKind::PointerMove)
            {
                float x = 0.0f, y = 0.0f;
                SDL_GetGlobalMouseState(&x, &y);
                SDL_WarpMouseGlobal(x + event.dx, y + event.dy);
                UpdateRemoteCursorPosition(CursorStyle::Arrow);
                continue;
            }
            if (event.kind == EventKind::PointerClick)
            {
                UpdateRemoteCursorPosition(CursorStyle::Hand);
                SDL_Event down = {};
                down.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
                down.button.button = SDL_BUTTON_LEFT;
                down.button.down = true;
                SDL_PushEvent(&down);
                SDL_Event up = {};
                up.type = SDL_EVENT_MOUSE_BUTTON_UP;
                up.button.button = SDL_BUTTON_LEFT;
                up.button.down = false;
                SDL_PushEvent(&up);
                continue;
            }
            if (event.kind == EventKind::PairRequired)
            {
                g_Context.remotePairPopup = true;
                continue;
            }
            if (event.kind == EventKind::Paired)
            {
                g_Context.remotePairPopup = false;
                continue;
            }

            if (event.action == "up") Nimble::TriggerNavigationAction(Nimble::NavAction::Up, "REMOTE_UP");
            else if (event.action == "down") Nimble::TriggerNavigationAction(Nimble::NavAction::Down, "REMOTE_DOWN");
            else if (event.action == "left") Nimble::TriggerNavigationAction(Nimble::NavAction::Left, "REMOTE_LEFT");
            else if (event.action == "right") Nimble::TriggerNavigationAction(Nimble::NavAction::Right, "REMOTE_RIGHT");
            else if (event.action == "select") PushKey(SDLK_RETURN);
            else if (event.action == "back") Nimble::TriggerNavigationAction(Nimble::NavAction::Back, "REMOTE_BACK");
            else if (event.action == "forward") Nimble::TriggerNavigationAction(Nimble::NavAction::Forward, "REMOTE_FORWARD");
            else if (event.action == "keyboard_backspace") BackspaceFocusedTextField();
            else if (event.action == "keyboard_enter") PushKey(SDLK_RETURN);
            else if (event.action.rfind("keyboard_text:", 0) == 0) InsertTextIntoFocusedTextField(event.action.substr(14));
            else if (event.action == "home")
            {
                g_Context.settingsOpen = false;
                g_NextFocusedWidgetId = "navbar_home";
            }
            else if (event.action == "menu")
            {
                g_Context.settingsOpen = !g_Context.settingsOpen;
                g_NextFocusedWidgetId = g_Context.settingsOpen ? "drawer_settings_closesettings" : "navbar_settings";
            }
        }
    }

    Status GetStatus()
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        return {
            g_running.load(),
            g_paired.load(),
            g_port,
            g_pairingCode,
            g_lastClient,
            g_lastMessage,
            g_error
        };
    }

    std::string LocalAddress()
    {
        int fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0)
            return "localhost";

        ifconf ifc = {};
        char buffer[1024];
        ifc.ifc_len = sizeof(buffer);
        ifc.ifc_buf = buffer;
        if (ioctl(fd, SIOCGIFCONF, &ifc) < 0)
        {
            close(fd);
            return "localhost";
        }

        ifreq *it = ifc.ifc_req;
        const ifreq *end = it + (ifc.ifc_len / sizeof(ifreq));
        for (; it != end; ++it)
        {
            sockaddr_in *addr = reinterpret_cast<sockaddr_in *>(&it->ifr_addr);
            std::string ip = inet_ntoa(addr->sin_addr);
            if (ip.rfind("127.", 0) != 0)
            {
                close(fd);
                return ip;
            }
        }

        close(fd);
        return "localhost";
    }
}
