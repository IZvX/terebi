#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <array>
#include <sstream>
#include <stdexcept>

namespace nmcli {

    // --- Internal Helpers ---
    namespace detail {
        // Safely escapes strings for bash execution to prevent command injection
        inline std::string escape(const std::string& arg) {
            std::string escaped = "'";
            for (char c : arg) {
                if (c == '\'') escaped += "'\\''";
                else escaped += c;
            }
            escaped += "'";
            return escaped;
        }

        // Executes a shell command and returns the standard output
        inline std::string exec(const std::string& cmd) {
            std::array<char, 128> buffer;
            std::string result;
            // Redirect stderr to stdout to catch errors
            std::unique_ptr<FILE, decltype(&pclose)> pipe(popen((cmd + " 2>&1").c_str(), "r"), pclose);
            if (!pipe) throw std::runtime_error("popen() failed!");
            while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                result += buffer.data();
            }
            return result;
        }

        // Parses colon-separated output from nmcli terse mode, respecting backslash escapes
        inline std::vector<std::string> split_unescape(const std::string& str, char delim = ':') {
            std::vector<std::string> result;
            std::string current;
            bool escaped = false;
            for (char c : str) {
                if (escaped) { current += c; escaped = false; }
                else if (c == '\\') { escaped = true; }
                else if (c == delim) { result.push_back(current); current.clear(); }
                else { current += c; }
            }
            result.push_back(current);
            return result;
        }
    }

    // --- Core Network Structure ---
    struct Network {
        std::string ssid;
        int signal;
        bool active;

        // Connect to this specific network
        bool connect(const std::string& password = "") const {
            std::string cmd = "nmcli dev wifi connect " + detail::escape(ssid);
            if (!password.empty()) {
                cmd += " password " + detail::escape(password);
            }
            std::string res = detail::exec(cmd);
            return res.find("successfully") != std::string::npos;
        }

        // Disconnect from this network
        bool disconnect() const {
            std::string cmd = "nmcli con down id " + detail::escape(ssid);
            std::string res = detail::exec(cmd);
            return res.find("successfully") != std::string::npos;
        }

        // Delete (forget) the network credentials from NetworkManager
        bool forget() const {
            std::string cmd = "nmcli con delete id " + detail::escape(ssid);
            std::string res = detail::exec(cmd);
            return res.find("successfully") != std::string::npos;
        }
    };

    // --- Global Functions ---
    
    // Connect to a network (standalone function taking a Network struct)
    inline bool connect(const Network& network, const std::string& password = "") {
        return network.connect(password);
    }

    // Returns a list of scanned/available Wi-Fi networks
    inline std::vector<Network> list_networks() {
        std::vector<Network> networks;
        // Ask nmcli for parsed fields: IN-USE, SIGNAL, SSID
        std::string output = detail::exec("nmcli -t -f IN-USE,SIGNAL,SSID dev wifi list");
        std::istringstream stream(output);
        std::string line;
        
        while (std::getline(stream, line)) {
            if (line.empty()) continue;
            auto parts = detail::split_unescape(line, ':');
            if (parts.size() >= 3) {
                Network net;
                net.active = (parts[0] == "*");
                try { net.signal = std::stoi(parts[1]); } catch (...) { net.signal = 0; }
                net.ssid = parts[2];
                
                // Ignore hidden/empty networks
                if (!net.ssid.empty()) {
                    networks.push_back(net);
                }
            }
        }
        return networks;
    }

    // --- Property Interface for "nmcli.enabled = true" ---
    struct EnabledProperty {
        // Getter (when checked in an `if`)
        operator bool() const {
            std::string res = detail::exec("nmcli -t radio wifi");
            return res.find("enabled") != std::string::npos;
        }
        
        // Setter (when doing `nmcli::enabled = true`)
        EnabledProperty& operator=(bool enable) {
            detail::exec(enable ? "nmcli radio wifi on" : "nmcli radio wifi off");
            return *this;
        }
    };

    // Global inline instance allows syntaxes like `nmcli::enabled = true`
    inline EnabledProperty enabled;

} // namespace nmcli