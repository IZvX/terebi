#pragma once
#include "../shared.h"
#include "../nav_item_rounded.h"
#include "../settings_drawer.h"
#include "../drawer_toggle.h"
#include "../drawer_item.h"

inline Widget SettingsTab_Dev(AppFonts& fonts) {
    using namespace Widgets;
    bool pywalEnabled = g_Context.pywalEnabled;
    WalTheme wal = g_Context.currentTheme;

    SDL_Color surface = GetThemeColor(DefaultTheme::SurfaceLight, wal.foreground, pywalEnabled);
    surface.a = 8;

    WidgetStyle cardStyle;
    cardStyle.color  = surface;
    cardStyle.radius = 12;

    // Determine opacity for the sub-settings
    bool debugDisabled = !g_GlobalDebug.enabled;
    float subAlpha = !debugDisabled ? 1.0f : 0.5f;
    // --- Main Layout ---
    return Column(MainAxisAlignment::Start, CrossAxisAlignment::Start, 20, std::vector<Widget>{
        // Header
        Expanded(0, 1,
            Row(MainAxisAlignment::Start, CrossAxisAlignment::Center, 20, {
                NavItemRounded(
                    FontAwesome::ArrowLeft().codepoint,
                    "Back",
                    "drawer_dev_back",
                    {"drawer_dev_statereset_toggle","drawer_dev_debugtoggle", "", "", "drawer_dev_debugtoggle", "drawer_dev_statereset_toggle"},
                    fonts
                ).OnClick(
                    "drawer_system_back",
                    0.25f,
                    []() {
                        SetDrawerTab("settings_drawer", "main");
                        g_NextFocusedWidgetId = "drawer_settings_main_system";
                    },
                    [](Widget&, float) {}
                ),

                Text("Developer Settings", fonts.arial28, {255, 255, 255, 255}, {})
            })
        ),
        Expanded(1,1,Column(MainAxisAlignment::Start, CrossAxisAlignment::Stretch, 10, {
                // 1. Master Toggle (Always 1.0 opacity)
                DrawerToggle(g_GlobalDebug.enabled,{},"Enable Debug","drawer_dev_debugtoggle",{"drawer_dev_back","","","","","drawer_dev_back"},fonts),
                
                // 2. Sub-Toggles (Wrapped in Opacity)
                Opacity(subAlpha, DrawerToggle(g_GlobalDebug.showBounds, {}, "Show Bounds", "drawer_dev_showbounds_toggle", 
                    {"drawer_dev_debugtoggle", "drawer_dev_showpadding_toggle", "", "", "", ""}, fonts)
                    .WithDisabled(debugDisabled)),
                
                Opacity(subAlpha, DrawerToggle(g_GlobalDebug.showPadding, {}, "Show Padding", "drawer_dev_showpadding_toggle", 
                    {"drawer_dev_showbounds_toggle", "drawer_dev_showspacing_toggle", "", "", "", ""}, fonts)
                    .WithDisabled(debugDisabled)),
                
                Opacity(subAlpha, DrawerToggle(g_GlobalDebug.showSpacing, {}, "Show Spacing", "drawer_dev_showspacing_toggle", 
                    {"drawer_dev_showpadding_toggle", "drawer_dev_showexpanded_toggle", "", "", "", ""}, fonts)
                    .WithDisabled(debugDisabled)),
                
                Opacity(subAlpha, DrawerToggle(g_GlobalDebug.showExpanded, {}, "Show Expanded", "drawer_dev_showexpanded_toggle", 
                    {"drawer_dev_showspacing_toggle", "drawer_dev_showrow_toggle", "", "", "", ""}, fonts)
                    .WithDisabled(debugDisabled)),
                
                Opacity(subAlpha, DrawerToggle(g_GlobalDebug.showRow, {}, "Show Row", "drawer_dev_showrow_toggle", 
                    {"drawer_dev_showexpanded_toggle", "drawer_dev_showcolumn_toggle", "", "", "", ""}, fonts)
                    .WithDisabled(debugDisabled)),
                
                Opacity(subAlpha, DrawerToggle(g_GlobalDebug.showColumn, {}, "Show Column", "drawer_dev_showcolumn_toggle", 
                    {"drawer_dev_showrow_toggle", "drawer_dev_shownavarrows_toggle", "", "", "", ""}, fonts)
                    .WithDisabled(debugDisabled)),
                
                Opacity(subAlpha, DrawerToggle(g_GlobalDebug.showNavArrows, {}, "Show NavArrows", "drawer_dev_shownavarrows_toggle", 
                    {"drawer_dev_showcolumn_toggle", "drawer_dev_focusonlynav_toggle", "", "", "", ""}, fonts)
                    .WithDisabled(debugDisabled)),

                Opacity(subAlpha, DrawerToggle(g_GlobalDebug.focusOnlyNavArrows, {}, "Focus-Only NavArrows", "drawer_dev_focusonlynav_toggle", 
                    {"drawer_dev_shownavarrows_toggle", "drawer_dev_childreninherit_toggle", "", "", "", ""}, fonts)
                    .WithDisabled(debugDisabled)),
                
                Opacity(subAlpha, DrawerToggle(g_GlobalDebug.childrenInherit, {}, "Children Inherit Debug", "drawer_dev_childreninherit_toggle", 
                    {"drawer_dev_focusonlynav_toggle", "drawer_dev_widgetids_toggle", "", "", "", ""}, fonts)
                    .WithDisabled(debugDisabled)),

                // --- 1. Layout & Metrics ---
                DrawerToggle(g_Settings.showWidgetIds,{},"Overlay Widget IDs","drawer_dev_widgetids_toggle",{"","","","","",""},fonts),
                DrawerToggle(g_Settings.showClipRects,{},"Show Clip Rects","drawer_dev_cliprects_toggle",{"","","","","",""},fonts),
                DrawerToggle(g_Settings.showFlexWeights,{},"Show Flex Weights","drawer_dev_flexweights_toggle",{"","","","","",""},fonts),

                // --- 2. Interaction & State ---
                DrawerToggle(g_Settings.showFocusLoop,{},"Show Focus Loop","drawer_dev_focusloop_toggle",{"","","","","",""},fonts),
                DrawerToggle(g_Settings.highlightInputCapture,{},"Highlight Input Capture","drawer_dev_inputcapture_toggle",{"","","","","",""},fonts),

                // --- 3. Performance & Rendering ---
                DrawerToggle(g_Settings.slowAnimations,{},"Slow Animations 10%","drawer_dev_slowanim_toggle",{"","","","","",""},fonts),
                DrawerToggle(g_Settings.paintFlashMode,{},"Paint-Flash Mode","drawer_dev_paintflash_toggle",{"","","","","",""},fonts),
                DrawerToggle(g_Settings.showFPSOverlay,{},"Show FPS Overlay","drawer_dev_fpsoverlay_toggle",{"","","","","",""},fonts),

                // --- 4. Logic & Data ---
                DrawerToggle(g_Settings.triggerStateReset,{},"Trigger State Reset","drawer_dev_statereset_toggle",{"","drawer_dev_back","","","drawer_dev_back",""},fonts)

            },ScrollBehavior::Always,ScrollAxis::Vertical,"",0,0,true)
        )
    });
}