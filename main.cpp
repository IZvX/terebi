#include "nimble/application.h"
#include "nimble/debug.h"
#include "terebi/app.h"

int main(int argc, char** argv)
{
    Nimble::Init();
    Nimble::ApplicationConfig config;
    config.title = "Terebi UI";
    config.width = 1280;
    config.height = 720;
    config.sdlWindowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    config.enableVSync = true;
    config.enableGPU = false;

    Nimble::OnInit([]() {
        Nimble::LoadFont("assets/fonts/Arial.ttf", 16);
    });

    NimbleDebug::ImGui([]() {
        ImGui::Begin("Debug");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Focused Widget: %s", Nimble::FocusedWidget().c_str());
        ImGui::Text("Input focused: %s", Nimble::IsTextFieldFocused() ? "true" : "false");
        ImGui::Text("OSK visible: %s", g_Context.oskVisible ? "true" : "false");
        ImGui::Checkbox("Show Bounds", &Nimble::Debug::showBounds);
        ImGui::Checkbox("Show Layout", &Nimble::Debug::showLayout);
        ImGui::End();
    });

    Nimble::OnTextFieldFocus([](const std::string& id, bool focused) {
        g_Context.oskVisible = focused;
        g_Context.oskTargetFieldId = focused ? id : "";
        if (focused) {
            SDL_StartTextInput();
            printf("TextField '%s' gained focus\n", id.c_str());
        } else {
            SDL_StopTextInput();
            printf("TextField '%s' lost focus\n", id.c_str());
        }
    });

    auto app = std::make_unique<TerebiApp>();
    Nimble::Application application(config, std::move(app));
    if (!application.Initialize())
        return -1;
    application.Run();
    return 0;
}