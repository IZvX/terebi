#pragma once

#include "app_card.h"
#include "shared.h"

#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_set>
#include <cctype>

namespace fs = std::filesystem;

struct InstalledAppEntry {
    std::string id;
    std::string name;
    std::string exec;
    IconData icon;
    TTF_Font* iconFont;
    SDL_Color color;
};

// Helper: lowercase string for matching
inline std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

// Clean XDG Exec field codes (%u, %U, %f, %F, etc.)
inline std::string SanitizeExec(std::string exec) {
    std::string res;
    bool inQuotes = false;
    for (size_t i = 0; i < exec.size(); ++i) {
        if (exec[i] == '"') inQuotes = !inQuotes;
        if (!inQuotes && exec[i] == '%' && i + 1 < exec.size()) {
            i++; // skip field code
            continue;
        }
        res += exec[i];
    }
    // Trim trailing spaces
    while (!res.empty() && std::isspace(res.back())) res.pop_back();
    return res;
}

// Map app metadata to matching FontAwesome icons and theme colors
inline void DeduceAppStyle(const std::string& name, const std::string& exec, const std::string& cat, AppFonts& fonts, IconData& outIcon, TTF_Font*& outFont, SDL_Color& outColor) {
    std::string key = ToLower(name + " " + exec + " " + cat);

    // Browsers
    if (key.find("firefox") != std::string::npos || key.find("chrome") != std::string::npos || key.find("chromium") != std::string::npos || key.find("brave") != std::string::npos || key.find("browser") != std::string::npos || key.find("zen") != std::string::npos) {
        outIcon = FontAwesome::Google();
        outFont = fonts.fontAwesomeB24;
        outColor = { 255, 120, 50, 255 }; // Orange
    }
    // Terminal & Consoles
    else if (key.find("terminal") != std::string::npos || key.find("alacritty") != std::string::npos || key.find("foot") != std::string::npos || key.find("kitty") != std::string::npos || key.find("wezterm") != std::string::npos || key.find("xterm") != std::string::npos || key.find("konsole") != std::string::npos) {
        outIcon = FontAwesome::Linux();
        outFont = fonts.fontAwesomeB24;
        outColor = { 200, 205, 220, 255 }; // Slate
    }
    // Gaming & Steam
    else if (key.find("steam") != std::string::npos || key.find("lutris") != std::string::npos || key.find("heroic") != std::string::npos || key.find("retroarch") != std::string::npos || key.find("game") != std::string::npos || key.find("emulator") != std::string::npos) {
        outIcon = FontAwesome::Steam();
        outFont = fonts.fontAwesomeB24;
        outColor = { 45, 140, 255, 255 }; // Steam Blue
    }
    // Music & Audio
    else if (key.find("spotify") != std::string::npos || key.find("music") != std::string::npos || key.find("rhythmbox") != std::string::npos || key.find("audacious") != std::string::npos || key.find("audio") != std::string::npos) {
        outIcon = FontAwesome::Spotify();
        outFont = fonts.fontAwesomeB24;
        outColor = { 30, 215, 96, 255 }; // Green
    }
    // Video & Streaming
    else if (key.find("youtube") != std::string::npos || key.find("freetube") != std::string::npos || key.find("mpv") != std::string::npos || key.find("vlc") != std::string::npos || key.find("video") != std::string::npos || key.find("kodi") != std::string::npos) {
        outIcon = FontAwesome::YouTube();
        outFont = fonts.fontAwesomeB24;
        outColor = { 255, 50, 50, 255 }; // Red
    }
    // Code & Dev Editors
    else if (key.find("code") != std::string::npos || key.find("vscodium") != std::string::npos || key.find("cursor") != std::string::npos || key.find("nvim") != std::string::npos || key.find("emacs") != std::string::npos || key.find("sublime") != std::string::npos || key.find("git") != std::string::npos) {
        outIcon = FontAwesome::Github();
        outFont = fonts.fontAwesomeB24;
        outColor = { 40, 190, 255, 255 }; // Cyan
    }
    // Chat & Social
    else if (key.find("discord") != std::string::npos || key.find("vesktop") != std::string::npos || key.find("webcord") != std::string::npos) {
        outIcon = FontAwesome::Discord();
        outFont = fonts.fontAwesomeB24;
        outColor = { 114, 137, 218, 255 }; // Discord Blurple
    }
    else if (key.find("telegram") != std::string::npos) {
        outIcon = FontAwesome::Telegram();
        outFont = fonts.fontAwesomeB24;
        outColor = { 0, 136, 204, 255 }; // Telegram Blue
    }
    // Files & Storage
    else if (key.find("thunar") != std::string::npos || key.find("dolphin") != std::string::npos || key.find("nautilus") != std::string::npos || key.find("nemo") != std::string::npos || key.find("file") != std::string::npos) {
        outIcon = FontAwesome::Folder();
        outFont = fonts.fontAwesome24;
        outColor = { 255, 195, 45, 255 }; // Amber
    }
    // Media & Photos
    else if (key.find("gimp") != std::string::npos || key.find("inkscape") != std::string::npos || key.find("blender") != std::string::npos || key.find("krita") != std::string::npos || key.find("image") != std::string::npos || key.find("photo") != std::string::npos) {
        outIcon = FontAwesome::Images();
        outFont = fonts.fontAwesome24;
        outColor = { 240, 90, 140, 255 }; // Pink
    }
    // Default / System App
    else {
        outIcon = FontAwesome::Grid();
        outFont = fonts.fontAwesome24;
        outColor = { 80, 160, 240, 255 }; // Light Blue
    }
}

// Scan installed .desktop applications across system paths
inline std::vector<InstalledAppEntry> ScanInstalledApplications(AppFonts& fonts) {
    std::vector<InstalledAppEntry> apps;
    std::unordered_set<std::string> seenNames;

    std::vector<std::string> searchPaths = {
        "/usr/share/applications",
        "/usr/local/share/applications",
        "/var/lib/flatpak/exports/share/applications"
    };

    const char* home = getenv("HOME");
    if (home) {
        searchPaths.push_back(std::string(home) + "/.local/share/applications");
        searchPaths.push_back(std::string(home) + "/.local/share/flatpak/exports/share/applications");
    }

    for (const auto& dirPath : searchPaths) {
        if (!fs::exists(dirPath)) continue;

        try {
            for (const auto& entry : fs::directory_iterator(dirPath)) {
                if (entry.path().extension() != ".desktop") continue;

                std::ifstream file(entry.path());
                if (!file.is_open()) continue;

                std::string line;
                std::string appName;
                std::string appExec;
                std::string appCat;
                bool isApp = false;
                bool noDisplay = false;
                bool inDesktopEntry = false;

                while (std::getline(file, line)) {
                    // Trim line
                    while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();

                    if (line == "[Desktop Entry]") {
                        inDesktopEntry = true;
                        continue;
                    }
                    if (!inDesktopEntry) continue;
                    if (line.rfind("[", 0) == 0 && line != "[Desktop Entry]") break; // Next section

                    if (line == "Type=Application") isApp = true;
                    if (line == "NoDisplay=true" || line == "Hidden=true") noDisplay = true;

                    if (appName.empty() && line.rfind("Name=", 0) == 0) {
                        appName = line.substr(5);
                    }
                    if (appExec.empty() && line.rfind("Exec=", 0) == 0) {
                        appExec = SanitizeExec(line.substr(5));
                    }
                    if (appCat.empty() && line.rfind("Categories=", 0) == 0) {
                        appCat = line.substr(11);
                    }
                }

                if (isApp && !noDisplay && !appName.empty() && !appExec.empty()) {
                    if (seenNames.find(appName) == seenNames.end()) {
                        seenNames.insert(appName);

                        std::string safeId = "app_" + std::to_string(apps.size());
                        IconData icon;
                        TTF_Font* iconFont = nullptr;
                        SDL_Color color;
                        DeduceAppStyle(appName, appExec, appCat, fonts, icon, iconFont, color);

                        apps.push_back({ safeId, appName, appExec, icon, iconFont, color });
                    }
                }
            }
        } catch (...) {}
    }

    // Sort apps alphabetically
    std::sort(apps.begin(), apps.end(), [](const InstalledAppEntry& a, const InstalledAppEntry& b) {
        return ToLower(a.name) < ToLower(b.name);
    });

    return apps;
}

inline Widget RenderAppsSection(int ww, int wh, float dt, AppFonts& fonts, GlobalContext& gctx)
{
    using namespace Widgets;

    // Cache the scanned apps so we don't hit the disk on every frame
    static std::vector<InstalledAppEntry> s_installedApps;
    if (s_installedApps.empty()) {
        s_installedApps = ScanInstalledApplications(fonts);
    }

    // Header Title with real app counter
    std::string countLabel = std::to_string(s_installedApps.size()) + " applications installed";
    Widget header = Row(MainAxisAlignment::Start, CrossAxisAlignment::Center, 12, {
        RoundedBox({ 4, 18 }, WidgetStyle{ .color = { 55, 135, 255, 255 }, .radius = 2 }, {}),
        Text("Applications", fonts.spaceGrotesk24 ? fonts.spaceGrotesk24 : fonts.arial20, { 255, 255, 255, 255 }, {}),
        Text(countLabel, fonts.arial12, { 160, 165, 180, 180 }, {})
    });

    // Navbar destinations for Up Arrow mapping
    const std::vector<std::string> navTargets = {
        "navbar_home", "navbar_movies", "navbar_shows", "navbar_apps", "navbar_library", "navbar_search"
    };

    std::vector<Widget> appCards;
    const size_t totalApps = s_installedApps.size();

    for (size_t i = 0; i < totalApps; ++i) {
        const auto& item = s_installedApps[i];

        // Map Up Arrow to the nearest navbar item
        std::string upTarget = navTargets[std::min(i, navTargets.size() - 1)];

        // Build continuous wrap-around navigation ring
        std::string leftTarget  = (i > 0) ? s_installedApps[i - 1].id : s_installedApps[totalApps - 1].id;
        std::string rightTarget = (i + 1 < totalApps) ? s_installedApps[i + 1].id : s_installedApps[0].id;

        WidgetNav nav{
            .up = upTarget,
            .down = "",
            .left = leftTarget,
            .right = rightTarget,
            .next = rightTarget,
            .prev = leftTarget
        };

        appCards.push_back(AppCard(
            item.id,
            item.name,
            item.icon,
            item.iconFont,
            item.exec,
            nav,
            fonts,
            item.color
        ));
    }

    Widget appsRow = Row(MainAxisAlignment::Start, CrossAxisAlignment::Center, 18, 
                         appCards, 
                         ScrollBehavior::Auto, ScrollAxis::Horizontal, "apps_row", 1.0f, 0.0f);

    return Column(MainAxisAlignment::Start, CrossAxisAlignment::Start, 16, {
        header,
        appsRow
    });
}