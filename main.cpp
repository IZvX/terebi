#include "nimble/application.h"
#include "nimble/debug.h"
#include "terebi/app.h"

#include <iostream>
#include <cstring>
#include <string>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

int main(int argc, char** argv)
{
    // -------------------------------------------------------------
    // 1. UI CLIENT MODE (Runs inside the Compositor)
    // -------------------------------------------------------------
    // if (argc > 1 && strcmp(argv[1], "--ui") == 0) {
        Nimble::Init();
        Nimble::ApplicationConfig config;
        config.title = "Terebi";
        config.width = 1280;
        config.height = 720;
        config.sdlWindowFlags = SDL_WINDOW_TRANSPARENT | SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP;
        config.enableVSync = true;
        config.enableGPU = true;

        Nimble::OnInit([]() {
            Nimble::LoadFont("assets/fonts/Arial.ttf", 16);
        });

        Nimble::OnShutdown([]() {
            SDL_ShowCursor();
        });

        NimbleDebug::ImGui([]() {
            ImGui::Begin("Debug");
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
            ImGui::End();
        });

        auto app = std::make_unique<TerebiApp>();
        Nimble::Application application(config, std::move(app));
        if (application.Initialize()) {
            application.Run();
        }
        return 0;
    // }

    // -------------------------------------------------------------
    // 2. COMPOSITOR SERVER MODE
    // -------------------------------------------------------------
    // signal(SIGINT, cleanup_children);
    // signal(SIGTERM, cleanup_children);

    // std::string startup_cmd = std::string(argv[0]) + " --ui";
    // int ret = tinywl_main(argc, argv, startup_cmd.c_str());

    // cleanup_children(0);
    // return ret;
}