#pragma once

#include "shared.h"
#include "../nimble/application.h"
#include "../fontawesome/fontawesome.h"
#include <cctype>

// ─────────────────────────────────────────────────────────────────────────────
//  Logic helpers
// ─────────────────────────────────────────────────────────────────────────────

inline SDL_Color OskColor(SDL_Color fallback, SDL_Color walColor, Uint8 alpha)
{
    SDL_Color color = GetThemeColor(fallback, walColor, g_Context.pywalEnabled);
    color.a = alpha;
    return color;
}

inline void OskPushText(const std::string &text)
{
    InsertTextIntoTextField(g_Context.oskTargetFieldId, text);
}

inline void OskPushKey(SDL_Keycode key)
{
    if (key == SDLK_BACKSPACE)
        BackspaceTextField(g_Context.oskTargetFieldId);
    else if (key == SDLK_RETURN)
    {
        Nimble::ClearSecondaryFocus();
        SDL_Event down = {};
        down.type = SDL_EVENT_KEY_DOWN;
        down.key.key = SDLK_RETURN;
        down.key.down = true;
        SDL_PushEvent(&down);
    }
}

inline bool OskMoveCursor(bool right)
{
    if (g_Context.oskTargetFieldId.empty())
        return false;

    auto bindingIt = g_TextFieldBindings.find(g_Context.oskTargetFieldId);
    if (bindingIt == g_TextFieldBindings.end() || !bindingIt->second)
        return false;

    std::string &text = *bindingIt->second;
    TextFieldState &tfState = g_TextFieldState[g_Context.oskTargetFieldId];
    tfState.ClampToText(text);

    if (right)
    {
        if (tfState.HasSelection())
            tfState.cursorPosition = tfState.GetSelectionEnd();
        else
            MoveCursorRight(text, tfState.cursorPosition);
    }
    else
    {
        if (tfState.HasSelection())
            tfState.cursorPosition = tfState.GetSelectionStart();
        else
            MoveCursorLeft(text, tfState.cursorPosition);
    }

    tfState.selectionAnchor = tfState.cursorPosition;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Mode state  (alpha / symbols)
// ─────────────────────────────────────────────────────────────────────────────

enum class OskMode { Alpha, Symbols };
enum class OskShiftState { Off, One, Locked };
inline OskMode g_OskMode = OskMode::Alpha;
inline OskShiftState g_OskShiftState = OskShiftState::Off;

inline bool OskShiftEnabled()
{
    return g_OskShiftState != OskShiftState::Off;
}

inline std::string OskApplyShift(const std::string &text)
{
    if (g_OskShiftState == OskShiftState::Off || text.size() != 1)
        return text;
    unsigned char c = static_cast<unsigned char>(text[0]);
    if (std::islower(c))
        return std::string(1, static_cast<char>(std::toupper(c)));
    return text;
}

inline void OskConsumeShift()
{
    if (g_OskShiftState == OskShiftState::One)
        g_OskShiftState = OskShiftState::Off;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Design tokens
// ─────────────────────────────────────────────────────────────────────────────

namespace Osk
{
    constexpr SDL_Color PanelBg      = {14,  16,  22,  230};
    constexpr SDL_Color PanelBorder  = {255, 255, 255, 18};

    constexpr SDL_Color KeyBg        = {255, 255, 255, 12};
    constexpr SDL_Color KeyBorder    = {255, 255, 255, 0};

    constexpr SDL_Color TextNormal   = {200, 205, 220, 255};
    constexpr SDL_Color TextMuted    = {120, 128, 150, 255};

    constexpr SDL_Color FocusBorder  = {255, 255, 255, 255};
    constexpr SDL_Color HoverBg      = {255, 255, 255, 24};

    constexpr SDL_Color ActionBg     = {255, 255, 255, 18};
    constexpr SDL_Color ActionBorder = {255, 255, 255, 180};
    constexpr SDL_Color ActionText   = {255, 255, 255, 255};

    constexpr SDL_Color DelBg        = {255, 255, 255, 8};
    constexpr SDL_Color DelIcon      = {210, 100, 120, 255};

    constexpr SDL_Color ModBg        = {255, 255, 255, 6};
    constexpr SDL_Color ModText      = {160, 168, 190, 255};
    constexpr SDL_Color ModActiveBg  = {255, 255, 255, 30};
    constexpr SDL_Color ModActiveText= {255, 255, 255, 255};

    constexpr int RadiusPanel = 16;
    constexpr int RadiusKey   = 10;
    constexpr int KeySize     = 54;   // square — same W and H
    constexpr int Gap         = 8;    // uniform gap H and V
    constexpr int PanelPad    = 18;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Core key builder
// ─────────────────────────────────────────────────────────────────────────────

inline Widget OskBuildKey(
    Widget                child,
    const std::string    &id,
    const WidgetNav      &nav,
    std::function<void()> action,
    float                 flex        = 1.0f,
    SDL_Color             bg          = Osk::KeyBg,
    SDL_Color             hoverBg     = Osk::HoverBg,
    SDL_Color             focusBorder = Osk::FocusBorder)
{
    using namespace Widgets;

    WidgetStyle style;
    style.color       = bg;
    style.radius      = Osk::RadiusKey;
    style.borderColor = Osk::KeyBorder;
    style.borderWidth = 1;

    // Fixed square size — flex only stretches horizontally via Expanded
    Widget key = RoundedBox({Osk::KeySize, Osk::KeySize}, style, Center(child));
    key.id = id;

    return Expanded(0, flex,
        key
        .OnClick(id, 0.06f, action,[](Widget &, float){}, Easing::EaseOutQuad, false, true)
        .OnKeyPress(SDLK_RETURN, action)
        .OnFocus(id, 0.20f, [focusBorder](Widget &w, float t) {
            w.animateBorder(focusBorder, 2, t);
            w.animateColor(Osk::HoverBg, t);
        })
        .OnHover(id, 0.12f,[hoverBg](Widget &w, float t) {
            SetCursor(CursorType::Hand);
            w.animateColor(hoverBg, t);
            w.animateBorder({255, 255, 255, 60}, 1, t);
        })
        .WithNav(nav.up, nav.down, nav.left, nav.right, nav.next, nav.prev)
    );
}

inline Widget OskTextKey(
    const std::string    &label,
    const std::string    &id,
    const WidgetNav      &nav,
    TTF_Font             *font,
    std::function<void()> action,
    float                 flex = 1.0f,
    SDL_Color             col  = Osk::TextNormal,
    SDL_Color             bg   = Osk::KeyBg)
{
    return OskBuildKey(Widgets::Text(label, font, col, {}),
                       id, nav, action, flex, bg);
}

inline Widget OskIconKey(
    IconData              icon,
    TTF_Font             *faFont,
    int                   iconSz,
    const std::string    &id,
    const WidgetNav      &nav,
    std::function<void()> action,
    float                 flex        = 1.0f,
    SDL_Color             col         = Osk::TextNormal,
    SDL_Color             bg          = Osk::KeyBg,
    SDL_Color             focusBorder = Osk::FocusBorder)
{
    return OskBuildKey(Widgets::Icon(icon, faFont, iconSz, col),
                       id, nav, action, flex, bg, Osk::HoverBg, focusBorder);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helper for keyboard layout assembly
// ─────────────────────────────────────────────────────────────────────────────

inline Widget OskAssembleKeyboard(int ww, int wh, int kbW, const std::vector<Widget> &rows, const std::vector<Widget> &rowNum)
{
    using namespace Widgets;
    
    const int kbH = Osk::PanelPad * 2 + Osk::KeySize * 5 + Osk::Gap * 5;  // 5 rows + gaps

    WidgetStyle panel;
    panel.color       = Osk::PanelBg;
    panel.radius      = Osk::RadiusPanel;
    panel.borderColor = Osk::PanelBorder;
    panel.borderWidth = 1;
    panel.shadowColor = {0, 0, 0, 140};
    panel.shadowBlur  = 32;

    std::vector<Widget> allRows;
    allRows.push_back(Row(MainAxisAlignment::Center, CrossAxisAlignment::Stretch, Osk::Gap, rowNum));
    for (const auto &row : rows) {
        allRows.push_back(row);
    }

    Widget keyboard = RoundedBox({kbW, kbH}, panel,
        Expanded(1, 1,
            Padding({Osk::PanelPad, Osk::PanelPad},
                Column(MainAxisAlignment::Start, CrossAxisAlignment::Stretch, Osk::Gap, allRows,
                    ScrollBehavior::None, ScrollAxis::Vertical, "osk_col", 1.0f, 1.0f)
            )
        )
    );
    keyboard.id = "osk_panel";

    // Background to capture clicks and prevent focus loss
    WidgetStyle shade;
    shade.color = {0, 0, 0, 0}; // transparent
    Widget background = RoundedBox({ww, wh}, shade, 
        Position(PositionType::Fixed,
            {(ww - kbW) / 2, std::max(16, wh - kbH - 16)},
            keyboard)
        .OnClick("osk_background", 0.0f,[](){
            // Refocus the OSK
            if (!Nimble::SecondaryFocusedWidget().empty())
                Nimble::SetSecondaryFocus(Nimble::SecondaryFocusedWidget());
        },[](Widget &, float){}, Easing::EaseOutQuad, false, false));

    return background;
}

// ─────────────────────────────────────────────────────────────────────────────
//  RenderOnScreenKeyboard
// ─────────────────────────────────────────────────────────────────────────────

inline Widget RenderOnScreenKeyboard(int ww, int wh, AppFonts &fonts)
{
    using namespace Widgets;
    using namespace Osk;

    const int kbW = std::min(800, std::max(360, ww - 48));

    const bool isAlpha = (g_OskMode == OskMode::Alpha);

    // ── Helper: the ?123 / ABC toggle key ────────────────────────────────────
    // nav for it is always the same regardless of mode
    auto ModeKey = [&](const std::string &id, const std::string &upId, const std::string &rightId) -> Widget {
        std::string label    = isAlpha ? "?123" : "ABC";
        SDL_Color   activeBg = isAlpha ? ModBg : ModActiveBg;
        SDL_Color   textCol  = isAlpha ? ModText : ModActiveText;
        return OskTextKey(label, id,
            {upId, "", "", rightId, rightId, ""},
            fonts.arial16,[]() { g_OskMode = (g_OskMode == OskMode::Alpha) ? OskMode::Symbols : OskMode::Alpha; },
            1.2f, textCol, activeBg);
    };

    // ─────────────────────────────────────────────────────────────────────────
    //  NUMBER ROW  — always visible, plain digits only
    // ─────────────────────────────────────────────────────────────────────────

    // ids: osk_n0 … osk_n9
    // wrapping: left of n0 → n9,  right of n9 → n0
    struct NumDef { const char *d, *id, *down; };
    static const NumDef nd[10] = {
        {"1","osk_n0","osk_r0c0"}, {"2","osk_n1","osk_r0c1"},
        {"3","osk_n2","osk_r0c2"}, {"4","osk_n3","osk_r0c3"},
        {"5","osk_n4","osk_r0c4"}, {"6","osk_n5","osk_r0c5"},
        {"7","osk_n6","osk_r0c6"}, {"8","osk_n7","osk_r0c7"},
        {"9","osk_n8","osk_r0c8"}, {"0","osk_n9","osk_r0c9"},
    };

    std::vector<Widget> rowNum;
    rowNum.reserve(10);
    for (int i = 0; i < 10; ++i)
    {
        std::string digit = nd[i].d;
        WidgetNav nav {
            "",                                        // up
            nd[i].down,                                // down
            i > 0 ? nd[i-1].id : nd[9].id,            // left  — wrap
            i < 9 ? nd[i+1].id : nd[0].id,            // right — wrap
            i < 9 ? nd[i+1].id : nd[0].id,
            i > 0 ? nd[i-1].id : nd[9].id,
        };
        rowNum.push_back(OskTextKey(nd[i].d, nd[i].id, nav,
            fonts.arial22,
            [digit]() { OskPushText(digit); }));
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  ALPHA MODE  (4 rows)
    // ─────────────────────────────────────────────────────────────────────────
    //
    //  Row naming: osk_r{row}c{col}   row 0=qwerty, 1=asdf, 2=zxcv, 3=bottom
    //  10 keys per row (including shift/bsp/mode/enter)
    //
    //  Wrapping: leftmost col → rightmost col and vice versa.

    if (isAlpha)
    {
        // ── Row 0: q w e r t y u i o p ───────────────────────────────────────
        // 10 equal keys, wrap L/R
        static const char *r0[10] = {"q","w","e","r","t","y","u","i","o","p"};
        std::vector<Widget> row0;
        for (int i = 0; i < 10; ++i)
        {
            std::string ch = r0[i];
            std::string label = OskApplyShift(ch);
            std::string id = "osk_r0c" + std::to_string(i);
            WidgetNav nav {
                nd[i].id,                                          // up → num row
                "osk_r1c" + std::to_string(i),                    // down
                i > 0 ? "osk_r0c" + std::to_string(i-1) : "osk_r0c9",  // left wrap
                i < 9 ? "osk_r0c" + std::to_string(i+1) : "osk_r0c0",  // right wrap
                i < 9 ? "osk_r0c" + std::to_string(i+1) : "osk_r0c0",
                i > 0 ? "osk_r0c" + std::to_string(i-1) : "osk_r0c9",
            };
            row0.push_back(OskTextKey(label, id, nav, fonts.arial24,
                [ch]() {
                    std::string text = OskApplyShift(ch);
                    OskPushText(text);
                    if (g_OskShiftState == OskShiftState::One && text != ch)
                        g_OskShiftState = OskShiftState::Off;
                }));
        }

        // ── Row 1: a s d f g h j k l @ ───────────────────────────────────────
        static const char *r1chars[10] = {"a","s","d","f","g","h","j","k","l","@"};
        std::vector<Widget> row1;
        for (int i = 0; i < 10; ++i)
        {
            std::string ch = r1chars[i];
            std::string label = OskApplyShift(ch);
            std::string id = "osk_r1c" + std::to_string(i);
            WidgetNav nav {
                "osk_r0c" + std::to_string(i),
                "osk_r2c" + std::to_string(i),
                i > 0 ? "osk_r1c" + std::to_string(i-1) : "osk_r1c9",
                i < 9 ? "osk_r1c" + std::to_string(i+1) : "osk_r1c0",
                i < 9 ? "osk_r1c" + std::to_string(i+1) : "osk_r1c0",
                i > 0 ? "osk_r1c" + std::to_string(i-1) : "osk_r1c9",
            };
            row1.push_back(OskTextKey(label, id, nav, fonts.arial24,
                [ch]() {
                    std::string text = OskApplyShift(ch);
                    OskPushText(text);
                    if (g_OskShiftState == OskShiftState::One && text != ch)
                        g_OskShiftState = OskShiftState::Off;
                }));
        }

        // ── Row 2: Shift  z x c v b n m .  Backspace ─────────────────────────
        // col 0 = shift (flex 1.2), cols 1-8 = letters, col 9 = bsp (flex 1.2)
        std::vector<Widget> row2;

        // Shift
        {
            bool shiftActive = (g_OskShiftState != OskShiftState::Off);
            IconData shiftIcon = (g_OskShiftState == OskShiftState::Locked)
                ? FontAwesome::Shift()
                : FontAwesome::ShiftOutline();
            SDL_Color shiftBg = shiftActive ? ModActiveBg : ModBg;
            SDL_Color shiftText = shiftActive ? ModActiveText : ModText;
            row2.push_back(OskIconKey(shiftIcon, fonts.fontAwesome24, 16,
                "osk_r2c0",
                {"osk_r1c0", "osk_r3c0", "osk_r2c9", "osk_r2c1", "osk_r2c1", "osk_r2c9"},[](){
                    if (g_OskShiftState == OskShiftState::One)
                        g_OskShiftState = OskShiftState::Locked;
                    else if (g_OskShiftState == OskShiftState::Locked)
                        g_OskShiftState = OskShiftState::Off;
                    else
                        g_OskShiftState = OskShiftState::One;
                }, 1.2f, shiftText, shiftBg));
        }

        static const char *r2chars[8] = {"z","x","c","v","b","n","m","."};
        for (int i = 0; i < 8; ++i)
        {
            std::string ch = r2chars[i];
            std::string label = OskApplyShift(ch);
            std::string id = "osk_r2c" + std::to_string(i + 1);
            std::string upL  = "osk_r1c" + std::to_string(i + 1);
            std::string downL= (i == 3 || i == 4) ? "osk_r3c3" : "osk_r3c" + std::to_string(i + 1); // v and b go to space
            std::string left = i > 0 ? "osk_r2c" + std::to_string(i) : "osk_r2c0";
            std::string right= "osk_r2c" + std::to_string(i + 2); // c9 = bsp
            WidgetNav nav { upL, downL, left, right, right, left };
            row2.push_back(OskTextKey(label, id, nav, fonts.arial24,
                [ch]() {
                    std::string text = OskApplyShift(ch);
                    OskPushText(text);
                    if (g_OskShiftState == OskShiftState::One && text != ch)
                        g_OskShiftState = OskShiftState::Off;
                }));
        }

        // Backspace
        row2.push_back(OskIconKey(FontAwesome::Backspace(), fonts.fontAwesome24, 18,
            "osk_r2c9",
            {"osk_r1c9", "osk_r3c9", "osk_r2c8", "osk_r2c0", "osk_r2c0", "osk_r2c8"},[](){OskPushKey(SDLK_BACKSPACE);},
            1.2f, DelIcon, DelBg, {220, 80, 100, 255}));

        // ── Row 3: Mode  ◀  ▶  [space]  –  _  .com  Enter ───────────────────
        // 10 slots: 0=mode 1=curL 2=curR 3-5=space(flex3) 6=dash 7=under 8=dot 9=enter
        // We'll use flex to fill the row like the others.
        std::vector<Widget> row3;

        // col 0: mode toggle
        row3.push_back(ModeKey("osk_r3c0", "osk_r2c0", "osk_r3c1"));

        // col 1: cursor left
        row3.push_back(OskIconKey(FontAwesome::ChevronLeft(), fonts.fontAwesome24, 16,
            "osk_r3c1",
            {"osk_r2c1","","osk_r3c0","osk_r3c2","osk_r3c2","osk_r3c0"},[](){OskMoveCursor(false);}, 1.0f, ModText, ModBg));

        // col 2: cursor right
        row3.push_back(OskIconKey(FontAwesome::ChevronRight(), fonts.fontAwesome24, 16,
            "osk_r3c2",
            {"osk_r2c2","","osk_r3c1","osk_r3c3","osk_r3c3","osk_r3c1"},[](){OskMoveCursor(true);}, 1.0f, ModText, ModBg));

        // cols 3-5: space (flex 3.0 = same total as 3 normal keys)
        row3.push_back(OskIconKey(FontAwesome::SpaceBar(), fonts.fontAwesome24, 18,
            "osk_r3c3",
            {"osk_r2c4","","osk_r3c2","osk_r3c6","osk_r3c6","osk_r3c2"},[](){OskPushText(" ");},
            3.0f + 2.0f * (Gap / (float)KeySize), ModText, ModBg));

        // col 6: dash
        row3.push_back(OskTextKey("-","osk_r3c6",
            {"osk_r2c5","","osk_r3c3","osk_r3c7","osk_r3c7","osk_r3c3"},
            fonts.arial24,[](){OskPushText("-");}));

        // col 7: underscore
        row3.push_back(OskTextKey("_","osk_r3c7",
            {"osk_r2c6","","osk_r3c6","osk_r3c8","osk_r3c8","osk_r3c6"},
            fonts.arial24,[](){OskPushText("_");}));

        // col 8: keyboard down
        row3.push_back(OskIconKey(FontAwesome::KeyboardDown(), fonts.fontAwesome24, 16,
            "osk_r3c8",
            {"osk_r2c7","","osk_r3c7","osk_r3c9","osk_r3c9","osk_r3c7"},[](){
                g_Context.oskVisible = false;
                g_OskMode = OskMode::Alpha;
                g_OskShiftState = OskShiftState::Off;
                Nimble::ClearSecondaryFocus();
            }, 1.0f, ModText, ModBg));

        // col 9: enter
        row3.push_back(OskIconKey(FontAwesome::ArrowLeft(), fonts.fontAwesome24, 18,
            "osk_r3c9",
            {"osk_r2c9","","osk_r3c8","osk_r3c0","osk_r3c0","osk_r3c8"},[](){OskPushKey(SDLK_RETURN);},
            1.2f, ActionText, ActionBg, ActionBorder));

        // ── Assemble alpha layout ─────────────────────────────────────────────
        return OskAssembleKeyboard(ww, wh, kbW, {
            Row(MainAxisAlignment::Center, CrossAxisAlignment::Stretch, Osk::Gap, row0),
            Row(MainAxisAlignment::Center, CrossAxisAlignment::Stretch, Osk::Gap, row1),
            Row(MainAxisAlignment::Center, CrossAxisAlignment::Stretch, Osk::Gap, row2),
            Row(MainAxisAlignment::Center, CrossAxisAlignment::Stretch, Osk::Gap, row3)
        }, rowNum);
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  SYMBOL MODE
    // ─────────────────────────────────────────────────────────────────────────
    //
    //  Row 0: num row (same)
    //  Row 1: ! @ # $ % ^ & * ( )          — osk_r0c0…9 reused from numrow-down
    //  Row 2: - _ = + [ ] { } | \
    //  Row 3: ; : ' " , . < > ? /
    //  Row 4: mode  ◀  ▶  space  ~  `  ^  enter
    //
    //  All use the same osk_r{row}c{col} naming.

    auto SymRow = [&](const std::vector<std::string> &chars, int row,
                      const std::string &upPrefix, const std::string &downPrefix)
        -> std::vector<Widget>
    {
        std::vector<Widget> out;
        int n = (int)chars.size();
        for (int i = 0; i < n; ++i)
        {
            std::string ch = chars[i];
            std::string id = "osk_r" + std::to_string(row) + "c" + std::to_string(i);
            std::string up   = upPrefix   + std::to_string(i);
            std::string down = downPrefix + std::to_string(i);
            std::string left  = i > 0 ? "osk_r" + std::to_string(row) + "c" + std::to_string(i-1)
                                       : "osk_r" + std::to_string(row) + "c" + std::to_string(n-1);
            std::string right = i < n-1 ? "osk_r" + std::to_string(row) + "c" + std::to_string(i+1)
                                        : "osk_r" + std::to_string(row) + "c0";
            WidgetNav nav { up, down, left, right, right, left };
            out.push_back(OskTextKey(ch, id, nav, fonts.arial22,
                [ch]() { OskPushText(ch); }));
        }
        return out;
    };

    std::vector<Widget> srow1 = SymRow({"!","@","#","$","%","^","&","*","(",")"},
        1, "osk_n", "osk_r2c");
    std::vector<Widget> srow2 = SymRow({"-","_","=","+","[","]","{","}","|","\\"},
        2, "osk_r1c", "osk_r3c");
    std::vector<Widget> srow3 = SymRow({";",":","'","\"",",",".",">","<","?","/"},
        3, "osk_r2c", "osk_r4c");

    // Fix navigation for , and . to go to space bar
    srow3[4] = OskTextKey(",", "osk_r3c4",
        {"osk_r2c4", "osk_r4c3", "osk_r3c3", "osk_r3c5", "osk_r3c5", "osk_r3c3"},
        fonts.arial22,[](){OskPushText(",");});
    srow3[5] = OskTextKey(".", "osk_r3c5",
        {"osk_r2c5", "osk_r4c3", "osk_r3c4", "osk_r3c6", "osk_r3c6", "osk_r3c4"},
        fonts.arial22,[](){OskPushText(".");});

    // Symbol bottom row: mode ◀ ▶ space ~ ` ^ bsp enter   (10 slots)
    std::vector<Widget> srow4;

    srow4.push_back(ModeKey("osk_r4c0", "osk_r3c0", "osk_r4c1"));

    srow4.push_back(OskIconKey(FontAwesome::ChevronLeft(), fonts.fontAwesome24, 16,
        "osk_r4c1", {"osk_r3c1","","osk_r4c0","osk_r4c2","osk_r4c2","osk_r4c0"},[](){OskMoveCursor(false);}, 1.0f, ModText, ModBg));

    srow4.push_back(OskIconKey(FontAwesome::ChevronRight(), fonts.fontAwesome24, 16,
        "osk_r4c2", {"osk_r3c2","","osk_r4c1","osk_r4c3","osk_r4c3","osk_r4c1"},[](){OskMoveCursor(true);}, 1.0f, ModText, ModBg));

    srow4.push_back(OskTextKey("space","osk_r4c3",
        {"osk_r3c3","","osk_r4c2","osk_r4c6","osk_r4c6","osk_r4c2"},
        fonts.arial16,[](){OskPushText(" ");},
        3.0f + 2.0f * (Gap / (float)KeySize), ModText, ModBg));

    srow4.push_back(OskTextKey("~","osk_r4c6",
        {"osk_r3c6","","osk_r4c3","osk_r4c7","osk_r4c7","osk_r4c3"},
        fonts.arial24,[](){OskPushText("~");}));

    srow4.push_back(OskTextKey("`","osk_r4c7",
        {"osk_r3c7","","osk_r4c6","osk_r4c8","osk_r4c8","osk_r4c6"},
        fonts.arial24,[](){OskPushText("`");}));

    srow4.push_back(OskIconKey(FontAwesome::KeyboardDown(), fonts.fontAwesome24, 16,
        "osk_r4c8",
        {"osk_r3c8","","osk_r4c7","osk_r4c9","osk_r4c9","osk_r4c7"},[](){
            g_Context.oskVisible = false;
            g_OskMode = OskMode::Alpha;
            g_OskShiftState = OskShiftState::Off;
            Nimble::ClearSecondaryFocus();
        }, 1.0f, ModText, ModBg));

    srow4.push_back(OskIconKey(FontAwesome::Times(), fonts.fontAwesome24, 18,
        "osk_r4c9",
        {"osk_r3c9","","osk_r4c8","osk_r4c10","osk_r4c10","osk_r4c8"},[](){OskPushKey(SDLK_BACKSPACE);},
        1.0f, DelIcon, DelBg, {220, 80, 100, 255}));

    srow4.push_back(OskIconKey(FontAwesome::ArrowLeft(), fonts.fontAwesome24, 18,
        "osk_r4c10",
        {"osk_r3c9","","osk_r4c9","osk_r4c0","osk_r4c0","osk_r4c9"},[](){OskPushKey(SDLK_RETURN);},
        1.2f, ActionText, ActionBg, ActionBorder));

    return OskAssembleKeyboard(ww, wh, kbW, {
        Row(MainAxisAlignment::Center, CrossAxisAlignment::Stretch, Osk::Gap, srow1),
        Row(MainAxisAlignment::Center, CrossAxisAlignment::Stretch, Osk::Gap, srow2),
        Row(MainAxisAlignment::Center, CrossAxisAlignment::Stretch, Osk::Gap, srow3),
        Row(MainAxisAlignment::Center, CrossAxisAlignment::Stretch, Osk::Gap, srow4)
    }, rowNum);
}

// ─────────────────────────────────────────────────────────────────────────────
//  app.h fix — change the initial secondary focus to the new first key id
//  In your Build() method, change:
//      Nimble::SetSecondaryFocus("osk_1_0");
//  to:
//      Nimble::SetSecondaryFocus("osk_r0c0");
// ─────────────────────────────────────────────────────────────────────────────

inline Widget RenderRemotePairPopup(int ww, int wh, AppFonts &fonts, const std::string &code)
{
    using namespace Widgets;
    WalTheme wal = g_Context.currentTheme;

    WidgetStyle shade;
    shade.color = {0, 0, 0, 140};

    WidgetStyle card;
    card.color       = OskColor(DefaultTheme::BackgroundSecondary, wal.background, 240);
    card.radius      = 18;
    card.borderColor = OskColor(DefaultTheme::Border, wal.color4, 180);
    card.borderWidth = 2;
    card.shadowColor = {0, 0, 0, 100};
    card.shadowBlur  = 20;

    SDL_Color titleCol  = OskColor(DefaultTheme::TextPrimary,   wal.foreground, 255);
    SDL_Color mutedCol  = OskColor(DefaultTheme::TextSecondary, wal.color7,     255);
    SDL_Color accentCol = OskColor(DefaultTheme::AccentPrimary, wal.color4,     255);

    Widget dismiss = OskTextKey("Dismiss", "remote_pair_dismiss",
        {"", "", "", "", "", ""},
        fonts.arial18,
        []() { g_Context.remotePairPopup = false; },
        1.0f, Osk::ActionText, Osk::ActionBg);

    Widget popup = Center(RoundedBox({440, 240}, card,
        Padding({28, 28},
            Column(MainAxisAlignment::Center, CrossAxisAlignment::Center, 14, {
                Text("Remote Pairing",                fonts.arial28, titleCol,  {}),
                Text("Enter this code on your phone", fonts.arial16, mutedCol,  {}),
                Text(code, fonts.spaceGrotesk48,      accentCol, {}),
                dismiss
            })
        )
    ));

    Widget layer = RoundedBox({ww, wh}, shade, popup);
    layer.id = "remote_pair_popup";
    return layer;
}