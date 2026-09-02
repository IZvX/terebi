#pragma once
#include <string>
#include "../utils/cursors.h"

namespace Nimble {
    void FireTextFieldFocusEvent(const std::string& id, bool focused);
}

namespace Widgets
{
    inline Widget TextField(const std::string &id, std::string &text, const std::string &placeholder, Vector2 size, TextFieldStyle tfStyle)
    {
        Widget w = {id, size};
        w.style.color = tfStyle.backgroundColor;
        w.style.radius = tfStyle.radius;
        w.style.borderWidth = tfStyle.borderWidth;
        w.style.borderColor = tfStyle.borderColor;

        w.BindText(&text);

        w.paint = [&text, placeholder, tfStyle, id](SDL_Renderer *renderer, SDL_Rect rect, const WidgetStyle &s, const InputState &input, const std::vector<Widget> &, WidgetDebug)
        {
            bool isFocused = g_UIState[id].isFocused;
            Nimble::FireTextFieldFocusEvent(id, isFocused);
            g_inputFocused = isFocused;
            TextFieldState &tfState = g_TextFieldState[id];

            // Safeguard against underlying text manipulation
            if (tfState.cursorPosition > text.length())
                tfState.cursorPosition = text.length();
            if (tfState.selectionAnchor > text.length())
                tfState.selectionAnchor = text.length();

            bool isHoveredRect = (input.mouseX >= rect.x && input.mouseX <= rect.x + rect.w &&
                                  input.mouseY >= rect.y && input.mouseY <= rect.y + rect.h);
            if (isHoveredRect)
                SetCursor(CursorType::IBeam);

            if (input.rightMouseClicked && isHoveredRect)
            {
                g_NextFocusedWidgetId = id;
                tfState.contextMenuOpen = true;
                tfState.contextMenuPos = {input.mouseX, input.mouseY};
            }

            if (input.mouseClicked && isHoveredRect)
            {
                int relX = input.mouseX - (rect.x + tfStyle.padding.x);
                tfState.cursorPosition = GetTextIndexFromMouse(tfStyle.font, text, relX);
                if (!(input.keyMod & SDL_KMOD_SHIFT))
                    tfState.selectionAnchor = tfState.cursorPosition;
                tfState.isDragging = true;
            }
            else if (!input.leftMouseDown)
            {
                tfState.isDragging = false;
            }

            if (tfState.isDragging && input.leftMouseDown)
            {
                int relX = input.mouseX - (rect.x + tfStyle.padding.x);
                tfState.cursorPosition = GetTextIndexFromMouse(tfStyle.font, text, relX);
            }

            // Store the rendered geometry to allow mouse clicks/selection processing
            tfState.lastRect = rect;

            float blinkTime = SDL_GetTicks() / 500.0f;
            // Hide cursor while selecting text
            bool showCursor = isFocused && !tfState.HasSelection() && (fmod(blinkTime, 2.0f) < 1.0f);

            // 1. Draw Border and Background
            SDL_Color borderColor = isFocused ? tfStyle.focusedBorderColor : s.borderColor;
            if (s.borderWidth > 0 && borderColor.a > 0)
            {
                SDL_Rect borderRect = {rect.x - s.borderWidth, rect.y - s.borderWidth, rect.w + s.borderWidth * 2, rect.h + s.borderWidth * 2};
                DrawRoundedBoxOutlineAA(renderer, borderRect, s.radius, s.borderWidth, borderColor);
            }
            FillRoundedBoxAA(renderer, rect, s.radius, s.color);

            if (!tfStyle.font)
                return;

            SDL_RenderSetClipRect(renderer, &rect);

            int textX = rect.x + tfStyle.padding.x;
            bool isPlaceholder = text.empty() && !isFocused;
            std::string displayText = isPlaceholder ? placeholder : text;
            SDL_Color renderColor = isPlaceholder ? tfStyle.placeholderColor : tfStyle.textColor;

            // 2. Render HIGHLIGHT SELECTION Background
            if (isFocused && tfState.HasSelection() && !isPlaceholder)
            {
                int start = tfState.GetSelectionStart();
                int end = tfState.GetSelectionEnd();
                int w1 = 0, w2 = 0, h = 0;

                if (start > 0)
                    TTF_SizeUTF8(tfStyle.font, text.substr(0, start).c_str(), &w1, &h);
                TTF_SizeUTF8(tfStyle.font, text.substr(0, end).c_str(), &w2, &h);

                int fontH = TTF_FontHeight(tfStyle.font);
                int yOff = (rect.h - fontH) / 2;

                SDL_Rect selRect = {textX + w1, rect.y + yOff, w2 - w1, fontH};

                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 120, 215, 128);
                SDL_RenderFillRect(renderer, &selRect);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            }

            // 3. Render the actual Text on top of selection
            if (!displayText.empty())
            {
                SDL_Surface *surf = TTF_RenderUTF8_Blended(tfStyle.font, displayText.c_str(), renderColor);
                if (surf)
                {
                    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                    int yOff = (rect.h - surf->h) / 2;
                    SDL_Rect dst = {textX, rect.y + yOff, surf->w, surf->h};

                    SDL_RenderCopy(renderer, tex, nullptr, &dst);
                    SDL_DestroyTexture(tex);
                    SDL_FreeSurface(surf);
                }
            }

            // 4. Draw The Typing Cursor
            if (showCursor)
            {
                int cursorXOffset = 0;
                if (!text.empty())
                {
                    std::string textBeforeCursor = text.substr(0, tfState.cursorPosition);
                    int w, h;
                    TTF_SizeUTF8(tfStyle.font, textBeforeCursor.c_str(), &w, &h);
                    cursorXOffset = w;
                }

                int cursorH = TTF_FontHeight(tfStyle.font);
                int yOff = (rect.h - cursorH) / 2;
                SDL_Rect cursorRect = {textX + cursorXOffset, rect.y + yOff, 2, cursorH};
                SDL_SetRenderDrawColor(renderer, tfStyle.cursorColor.r, tfStyle.cursorColor.g, tfStyle.cursorColor.b, tfStyle.cursorColor.a);
                SDL_RenderFillRect(renderer, &cursorRect);
            }

            SDL_RenderSetClipRect(renderer, nullptr);

            if (tfState.contextMenuOpen)
            {
                const char *items[] = {"Copy", "Cut", "Paste", "Select All"};
                const int itemCount = 4;
                const int itemHeight = 28;
                const int padding = 8;
                const int menuW = 140;
                const int menuH = itemCount * itemHeight + padding * 2;

                int menuX = tfState.contextMenuPos.x;
                int menuY = tfState.contextMenuPos.y;
                int maxX = rect.x + rect.w - menuW;
                if (menuX > maxX)
                    menuX = std::max(rect.x, maxX);
                if (menuY + menuH > rect.y + rect.h + 20)
                    menuY = std::max(rect.y, rect.y + rect.h - menuH);

                SDL_Rect menuRect = {menuX, menuY, menuW, menuH};
                FillRoundedBoxAA(renderer, menuRect, 10, {28, 28, 30, 230});

                bool clickedOutside = input.mouseClicked &&
                    !(input.mouseX >= menuRect.x && input.mouseX <= menuRect.x + menuRect.w &&
                      input.mouseY >= menuRect.y && input.mouseY <= menuRect.y + menuRect.h);
                if (clickedOutside)
                    tfState.contextMenuOpen = false;

                for (int i = 0; i < itemCount; ++i)
                {
                    SDL_Rect itemRect = {menuX + padding, menuY + padding + i * itemHeight, menuW - padding * 2, itemHeight};
                    bool hovered = input.mouseX >= itemRect.x && input.mouseX <= itemRect.x + itemRect.w &&
                                   input.mouseY >= itemRect.y && input.mouseY <= itemRect.y + itemRect.h;
                    SDL_Color bg = hovered ? SDL_Color{255, 255, 255, 20} : SDL_Color{0, 0, 0, 0};
                    FillRoundedBoxAA(renderer, itemRect, 6, bg);

                    if (input.mouseClicked && hovered)
                    {
                        if (i == 0 && tfState.HasSelection())
                        {
                            std::string selection = text.substr(tfState.GetSelectionStart(), tfState.GetSelectionEnd() - tfState.GetSelectionStart());
                            SDL_SetClipboardText(selection.c_str());
                        }
                        else if (i == 1 && tfState.HasSelection())
                        {
                            std::string selection = text.substr(tfState.GetSelectionStart(), tfState.GetSelectionEnd() - tfState.GetSelectionStart());
                            SDL_SetClipboardText(selection.c_str());
                            tfState.DeleteSelection(text);
                        }
                        else if (i == 2)
                        {
                            const char *clipboard = SDL_GetClipboardText();
                            if (clipboard && clipboard[0] != '\0')
                            {
                                if (tfState.HasSelection())
                                    tfState.DeleteSelection(text);
                                text.insert(tfState.cursorPosition, clipboard);
                                tfState.cursorPosition += (int)SDL_strlen(clipboard);
                                tfState.selectionAnchor = tfState.cursorPosition;
                            }
                        }
                        else if (i == 3)
                        {
                            tfState.selectionAnchor = 0;
                            tfState.cursorPosition = (int)text.length();
                        }
                        tfState.contextMenuOpen = false;
                    }

                    if (tfStyle.font)
                    {
                        SDL_Surface *surf = TTF_RenderUTF8_Blended(tfStyle.font, items[i], {255, 255, 255, 255});
                        if (surf)
                        {
                            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                            int yOff2 = (itemHeight - surf->h) / 2;
                            SDL_Rect dst = {itemRect.x + 6, itemRect.y + yOff2, surf->w, surf->h};
                            SDL_RenderCopy(renderer, tex, nullptr, &dst);
                            SDL_DestroyTexture(tex);
                            SDL_FreeSurface(surf);
                        }
                    }
                }
            }
        };

        return w;
    }
}