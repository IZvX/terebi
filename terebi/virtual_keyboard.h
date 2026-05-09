#pragma once
#include <string>
#include <vector>
#include <functional>
#include "../components/shared.h"
#include "../nimble/application.h"

struct KeyboardStyle {
    SDL_Color keyBg;
    SDL_Color keyHover;
    SDL_Color textColor;
    float radius = 8.0f;
    int keyWidth = 50;
    int keyHeight = 50;
    int spacing = 10;
};

namespace Widgets {

    inline Widget KeyboardKey(const std::string &label, const KeyboardStyle &kSt, AppFonts &fonts, std::function<void()> onPress, float expandX = 0.0f)
    {
        WidgetStyle style;
        style.color = kSt.keyBg;
        style.radius = (int)kSt.radius;

        Widget key = RoundedBox(Vector2{ kSt.keyWidth, kSt.keyHeight }, style, Center(Text(label, fonts.arial18, kSt.textColor)), {0, 0}, "osk_key_" + label);

        if (expandX > 0.0f)
            key.expandX = expandX;

        key = key.OnHover("osk_hov_" + label, 0.1f, [kSt](Widget &w, float t) {
            w.animateColor(kSt.keyHover, t);
            SetCursor(CursorType::Hand);
        });

        if (onPress)
            key = key.OnClick("osk_click_" + label, 0, std::move(onPress), [](Widget &, float) {});

        return key;
    }

    inline Widget onScreenKeyboard(int width, AppFonts &fonts, std::string &targetText, const std::string &textFieldId, bool visible, std::function<void()> onEnter = nullptr)
    {
        if (!visible)
            return SizedBox({0, 0});

        bool pywal = g_Context.pywalEnabled;
        WalTheme wal = g_Context.currentTheme;

        KeyboardStyle kSt;
        kSt.keyBg = GetThemeColor(DefaultTheme::SurfaceLight, wal.background, pywal);
        kSt.keyBg.a = 230;
        kSt.keyHover = GetThemeColor(DefaultTheme::Hover, wal.color4, pywal);
        kSt.textColor = GetThemeColor(DefaultTheme::TextPrimary, wal.foreground, pywal);

        static bool shiftMode = false;
        static bool symbolMode = false;

        auto applyText = [&](const std::string &text) {
            TextFieldState &tfState = g_TextFieldState[textFieldId];
            if (tfState.cursorPosition > (int)targetText.length())
                tfState.cursorPosition = (int)targetText.length();
            if (tfState.selectionAnchor > (int)targetText.length())
                tfState.selectionAnchor = (int)targetText.length();

            if (tfState.HasSelection()) {
                int start = tfState.GetSelectionStart();
                int end = tfState.GetSelectionEnd();
                targetText.erase(start, end - start);
                tfState.cursorPosition = start;
                tfState.selectionAnchor = start;
            }

            targetText.insert(tfState.cursorPosition, text);
            tfState.cursorPosition += (int)text.length();
            tfState.selectionAnchor = tfState.cursorPosition;
        };

        auto applyBackspace = [&] {
            TextFieldState &tfState = g_TextFieldState[textFieldId];
            if (tfState.HasSelection()) {
                int start = tfState.GetSelectionStart();
                int end = tfState.GetSelectionEnd();
                targetText.erase(start, end - start);
                tfState.cursorPosition = start;
                tfState.selectionAnchor = start;
                return;
            }
            if (tfState.cursorPosition <= 0)
                return;
            targetText.erase(tfState.cursorPosition - 1, 1);
            tfState.cursorPosition--;
            tfState.selectionAnchor = tfState.cursorPosition;
        };

        auto buildRow = [&](const std::vector<std::string> &keys) {
            std::vector<Widget> keyWidgets;
            for (const auto &text : keys) {
                keyWidgets.push_back(KeyboardKey(text, kSt, fonts, [&, text]() {
                    if (text == "Shift") {
                        shiftMode = !shiftMode;
                        return;
                    }
                    if (text == "?123" || text == "ABC") {
                        symbolMode = !symbolMode;
                        shiftMode = false;
                        return;
                    }
                    if (text == "Back") {
                        applyBackspace();
                        return;
                    }
                    if (text == "Enter") {
                        if (onEnter)
                            onEnter();
                        return;
                    }
                    if (text == "Space") {
                        applyText(" ");
                        if (shiftMode && !symbolMode)
                            shiftMode = false;
                        return;
                    }
                    applyText(text);
                    if (shiftMode && !symbolMode)
                        shiftMode = false;
                }));
            }
            return Row(RowArgs{
                .mainAlign = MainAxisAlignment::Center,
                .crossAlign = CrossAxisAlignment::Center,
                .spacing = kSt.spacing,
                .children = keyWidgets
            });
        };

        const std::vector<std::string> row1 = symbolMode ? std::vector<std::string>{"1","2","3","4","5","6","7","8","9","0"}
                                                           : std::vector<std::string>{"Q","W","E","R","T","Y","U","I","O","P"};
        const std::vector<std::string> row2 = symbolMode ? std::vector<std::string>{"@","#","$","%","&","*","(",")","-","+"}
                                                           : std::vector<std::string>{"A","S","D","F","G","H","J","K","L"};
        const std::vector<std::string> row3 = symbolMode ? std::vector<std::string>{"[","]","{","}","#","%","^","*","+","="}
                                                           : std::vector<std::string>{"Z","X","C","V","B","N","M"};

        std::vector<Widget> bottomKeys;
        bottomKeys.push_back(KeyboardKey(symbolMode ? "ABC" : "?123", kSt, fonts, [&]() {
            symbolMode = !symbolMode;
            shiftMode = false;
        }));
        bottomKeys.push_back(KeyboardKey("Back", kSt, fonts, applyBackspace, 1.5f));

        WidgetStyle bgStyle;
        bgStyle.color = {18, 18, 20, 240};
        bgStyle.radius = 20;

        return RoundedBox(Vector2{width, 300}, bgStyle,
            Padding({16, 16}, Column(MainAxisAlignment::Start, CrossAxisAlignment::Stretch, kSt.spacing, {
                buildRow(row1),
                buildRow(row2),
                buildRow(row3),
                Row(MainAxisAlignment::Center, CrossAxisAlignment::Center, kSt.spacing, bottomKeys),
                Row(MainAxisAlignment::Center, CrossAxisAlignment::Center, kSt.spacing, {
                    KeyboardKey("Space", kSt, fonts, [&]() { applyText(" "); }, 6.0f),
                    KeyboardKey("Enter", kSt, fonts, [&]() { if (onEnter) onEnter(); }, 2.0f)
                })
            }))
        );
    }

} // namespace Widgets
