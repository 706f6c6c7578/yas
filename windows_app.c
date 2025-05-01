#undef UNICODE
#undef _UNICODE
#define UNICODE
#define _UNICODE
#define WINVER _WIN32_WINNT_WIN7
#define _WIN32_WINNT _WIN32_WINNT_WIN7

#include <libloaderapi.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <richedit.h>
#include <tchar.h>
#include <windowsx.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "core_logic.h"
#include <wchar.h>

#ifndef STATUSCLASS
#define STATUSCLASS L"msctls_statusbar32"
#endif

#ifndef SBARS_TOOLTIPS
#define SBARS_TOOLTIPS 0x0400
#endif

#pragma comment(lib, "riched20.lib")
#include "core_logic.h"

#define ID_KEYBASE_EDIT        1001
#define ID_KEYINC_EDIT         1002
#define ID_TEXT_EDIT           1003
#define ID_ENCODE_BUTTON       1004
#define ID_DECODE_BUTTON       1005
#define ID_COPY_BUTTON         1006
#define ID_PASTE_BUTTON        1007
#define ID_KEYBASE_LABEL       1008
#define ID_KEYINC_LABEL        1009
#define ID_CLEAR_CANVAS_BUTTON 1010
#define ID_CLEAR_CLIPBOARD_BUTTON 1011
#define ID_STATUSBAR           1012

HWND hKeybaseEdit;
HWND hKeyincEdit;
HWND hTextEdit; 
HWND hEncodeButton;
HWND hDecodeButton;
HWND hCopyButton;
HWND hPasteButton;
HWND hKeybaseLabel;
HWND hKeyincLabel;
HWND hClearCanvasButton;
HWND hClearClipboardButton;
HWND hStatusBar;

wchar_t* utf8_to_wide(const char* utf8_str) {
    if (!utf8_str) return NULL;
    int num_chars = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, NULL, 0);
    if (num_chars == 0) return NULL;
    wchar_t* wide_str = (wchar_t*)malloc(num_chars * sizeof(wchar_t));
    if (!wide_str) return NULL;
    MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, wide_str, num_chars);
    return wide_str;
}

char* wide_to_utf8(const wchar_t* wide_str) {
    if (!wide_str) return NULL;
    int num_bytes = WideCharToMultiByte(CP_UTF8, 0, wide_str, -1, NULL, 0, NULL, NULL);
    if (num_bytes == 0) return NULL; 
    char* utf8_str = (char*)malloc(num_bytes);
    if (!utf8_str) return NULL;
    WideCharToMultiByte(CP_UTF8, 0, wide_str, -1, utf8_str, num_bytes, NULL, NULL);
    return utf8_str;
}

void ShowStatusMessage(const wchar_t* message) {
    SetWindowTextW(hStatusBar, message ? message : L"");
}

wchar_t* GetEditText(HWND hWndEdit) {
    int len = GetWindowTextLengthW(hWndEdit);
    if (len <= 0) return wcsdup(L"");
    wchar_t* text = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
    if (!text) return NULL; 
    GetWindowTextW(hWndEdit, text, len + 1);
    return text;
}

void SetEditText(HWND hWndEdit, const wchar_t* text) {
    SetWindowTextW(hWndEdit, text ? text : L"");
}

void LayoutControls(HWND hWnd, int clientWidth, int clientHeight) {
    int margin = 10;
    int labelWidth = 60;
    int editHeight = 25;
    int buttonWidth = 120;
    int buttonHeight = 30;
    int keyRowHeight = editHeight;
    int statusHeight = 20;
    int currentY = margin;
    int labelHeight = 20;
    int keyEditWidth_fixed = 40;

    SetWindowPos(hKeybaseLabel, NULL, margin, currentY + (keyRowHeight - labelHeight)/2, labelWidth, labelHeight, SWP_NOZORDER);
    SetWindowPos(hKeybaseEdit, NULL, margin + labelWidth + 5, currentY, keyEditWidth_fixed, editHeight, SWP_NOZORDER);
    SetWindowPos(hKeyincLabel, NULL, margin + labelWidth + 5 + keyEditWidth_fixed + margin, currentY + (keyRowHeight - labelHeight)/2, labelWidth, labelHeight, SWP_NOZORDER);
    SetWindowPos(hKeyincEdit, NULL, margin + labelWidth + 5 + keyEditWidth_fixed + margin + labelWidth + 5, currentY, keyEditWidth_fixed, editHeight, SWP_NOZORDER);
    currentY += keyRowHeight + margin;

    int textEditHeight = clientHeight - currentY - margin - buttonHeight - margin - statusHeight;
    if (textEditHeight < 50) textEditHeight = 50;
    SetWindowPos(hTextEdit, NULL, margin, currentY, clientWidth - 2 * margin, textEditHeight, SWP_NOZORDER);
    currentY += textEditHeight + margin;

    int buttonSpacing = 5;
    int numButtons = 6;
    int totalButtonWidth = numButtons * buttonWidth + (numButtons - 1) * buttonSpacing;
    int buttonStartX = (clientWidth - totalButtonWidth) / 2;
    if (buttonStartX < margin) buttonStartX = margin;

    SetWindowPos(hEncodeButton, NULL, buttonStartX, currentY, buttonWidth, buttonHeight, SWP_NOZORDER);
    SetWindowPos(hDecodeButton, NULL, buttonStartX + (buttonWidth + buttonSpacing), currentY, buttonWidth, buttonHeight, SWP_NOZORDER);
    SetWindowPos(hCopyButton, NULL, buttonStartX + 2*(buttonWidth + buttonSpacing), currentY, buttonWidth, buttonHeight, SWP_NOZORDER);
    SetWindowPos(hPasteButton, NULL, buttonStartX + 3*(buttonWidth + buttonSpacing), currentY, buttonWidth, buttonHeight, SWP_NOZORDER);
    SetWindowPos(hClearCanvasButton, NULL, buttonStartX + 4*(buttonWidth + buttonSpacing), currentY, buttonWidth, buttonHeight, SWP_NOZORDER);
    SetWindowPos(hClearClipboardButton, NULL, buttonStartX + 5*(buttonWidth + buttonSpacing), currentY, buttonWidth, buttonHeight, SWP_NOZORDER);

    SetWindowPos(hStatusBar, NULL, 0, clientHeight - statusHeight, clientWidth, statusHeight, SWP_NOZORDER);
    SendMessage(hStatusBar, WM_SIZE, 0, 0);
}

int extract_keys_from_header(const wchar_t* text, int* keybase, int* keyinc) {
    char* utf8_text = wide_to_utf8(text);
    if (!utf8_text) return 0;
    
    int result = 0;
    if (utf8_text && sscanf(utf8_text, "KEYBASE:%d KEYINC:%d", keybase, keyinc) == 2) {
        if (*keybase >= 0 && *keybase < BASE64_LEN && *keyinc >= 0 && *keyinc < BASE64_LEN) {
            result = 1;
        }
    }
    
    free(utf8_text);
    return result;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            LoadLibraryW(L"Msftedit.dll");
            
            hKeybaseLabel = CreateWindowW(L"STATIC", L"keybase:", WS_VISIBLE | WS_CHILD | SS_RIGHT,
                                         0, 0, 0, 0, hWnd, (HMENU)ID_KEYBASE_LABEL, GetModuleHandle(NULL), NULL);
            hKeybaseEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                                        0, 0, 0, 0, hWnd, (HMENU)ID_KEYBASE_EDIT, GetModuleHandle(NULL), NULL);
            hKeyincLabel = CreateWindowW(L"STATIC", L"keyinc:", WS_VISIBLE | WS_CHILD | SS_RIGHT,
                                         0, 0, 0, 0, hWnd, (HMENU)ID_KEYINC_LABEL, GetModuleHandle(NULL), NULL);
            hKeyincEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                                       0, 0, 0, 0, hWnd, (HMENU)ID_KEYINC_EDIT, GetModuleHandle(NULL), NULL);
            SendMessage(hKeybaseEdit, EM_SETLIMITTEXT, (WPARAM)2, 0);
            SendMessage(hKeyincEdit, EM_SETLIMITTEXT, (WPARAM)2, 0);

            hTextEdit = CreateWindowExW(WS_EX_CLIENTEDGE, 
                                       L"RICHEDIT50W",  
                                       L"", 
                                       WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | WS_HSCROLL |
                                       ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN,
                                       0, 0, 0, 0, hWnd, (HMENU)ID_TEXT_EDIT, GetModuleHandle(NULL), NULL);

            LOGFONTW lf;
            memset(&lf, 0, sizeof(LOGFONTW));
            lf.lfHeight = -MulDiv(12, GetDeviceCaps(GetDC(hWnd), LOGPIXELSY), 72);
            wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Segoe UI Emoji");
            HFONT hFont = CreateFontIndirectW(&lf);
            SendMessage(hTextEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

            CHARFORMAT2 cf;
            memset(&cf, 0, sizeof(CHARFORMAT2));
            cf.cbSize = sizeof(CHARFORMAT2);
            cf.dwMask = CFM_COLOR | CFM_FACE | CFM_SIZE;
            cf.crTextColor = RGB(0, 0, 0);
            wcscpy_s(cf.szFaceName, LF_FACESIZE, L"Segoe UI Emoji");
            cf.yHeight = 200;
            SendMessage(hTextEdit, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);

            hEncodeButton = CreateWindowW(L"BUTTON", L"Encode", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                         0, 0, 0, 0, hWnd, (HMENU)ID_ENCODE_BUTTON, GetModuleHandle(NULL), NULL);
            hDecodeButton = CreateWindowW(L"BUTTON", L"Decode", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                         0, 0, 0, 0, hWnd, (HMENU)ID_DECODE_BUTTON, GetModuleHandle(NULL), NULL);
            hCopyButton = CreateWindowW(L"BUTTON", L"Copy", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                       0, 0, 0, 0, hWnd, (HMENU)ID_COPY_BUTTON, GetModuleHandle(NULL), NULL);
            hPasteButton = CreateWindowW(L"BUTTON", L"Paste", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                        0, 0, 0, 0, hWnd, (HMENU)ID_PASTE_BUTTON, GetModuleHandle(NULL), NULL);
            hClearCanvasButton = CreateWindowW(L"BUTTON", L"Clear Canvas", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                        0, 0, 0, 0, hWnd, (HMENU)ID_CLEAR_CANVAS_BUTTON, GetModuleHandle(NULL), NULL);
            hClearClipboardButton = CreateWindowW(L"BUTTON", L"Clear Clipboard", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                        0, 0, 0, 0, hWnd, (HMENU)ID_CLEAR_CLIPBOARD_BUTTON, GetModuleHandle(NULL), NULL);
            hStatusBar = CreateWindowExW(0, STATUSCLASS, NULL, 
                                WS_CHILD | WS_VISIBLE | SBARS_TOOLTIPS,
                                0, 0, 0, 0, hWnd, (HMENU)ID_STATUSBAR, GetModuleHandle(NULL), NULL);
            ShowStatusMessage(L"Ready");
            break;

        case WM_SIZE:
            LayoutControls(hWnd, LOWORD(lParam), HIWORD(lParam));
            break; 

        case WM_COMMAND:
            {
                int id = LOWORD(wParam);
                if (HIWORD(wParam) == BN_CLICKED) {
                    switch (id) {
                        case ID_DECODE_BUTTON:
                        {
                            wchar_t *input_wtext = GetEditText(hTextEdit);
                            int auto_keys = 0;
                            int keybase = 0, keyinc = 0;
                            
                            if (input_wtext && *input_wtext != L'\0') {
                                if (extract_keys_from_header(input_wtext, &keybase, &keyinc)) {
                                    wchar_t keybase_str[10], keyinc_str[10];
                                    swprintf_s(keybase_str, 10, L"%d", keybase);
                                    swprintf_s(keyinc_str, 10, L"%d", keyinc);
                                    SetEditText(hKeybaseEdit, keybase_str);
                                    SetEditText(hKeyincEdit, keyinc_str);
                                    auto_keys = 1;
                                }
                            }

                            wchar_t *keybase_wstr = GetEditText(hKeybaseEdit);
                            wchar_t *keyinc_wstr = GetEditText(hKeyincEdit);
                            long keybase_manual, keyinc_manual;
                            wchar_t *endptr_base, *endptr_inc;
                            int keys_valid = 1;
                            ShowStatusMessage(L"");
                            
                            keybase_manual = wcstol(keybase_wstr, &endptr_base, 10);
                            keyinc_manual = wcstol(keyinc_wstr, &endptr_inc, 10);
                            
                            if (*keybase_wstr == L'\0' || *endptr_base != L'\0') {
                                ShowStatusMessage(L"Error: keybase value must be an integer.");
                                keys_valid = 0;
                            }
                            if (*keyinc_wstr == L'\0' || *endptr_inc != L'\0') {
                                ShowStatusMessage(L"Error: keyinc value must be an integer.");
                                keys_valid = 0;
                            }
                            
                            if (keybase_wstr) free(keybase_wstr);
                            if (keyinc_wstr) free(keyinc_wstr);
                            
                            if (!keys_valid) {
                                if (input_wtext) free(input_wtext);
                                return 0;
                            }
                            
                            if (keybase_manual < 0 || keybase_manual >= BASE64_LEN || 
                                keyinc_manual < 0 || keyinc_manual >= BASE64_LEN) {
                                wchar_t msg[100];
                                swprintf_s(msg, 100, L"Error: Keys must be 0-%d.", BASE64_LEN - 1);
                                ShowStatusMessage(msg);
                                if (input_wtext) free(input_wtext); 
                                return 0;
                            }

                            if (auto_keys) {
                                keybase_manual = keybase;
                                keyinc_manual = keyinc;
                            }

                            char *input_utf8_text = wide_to_utf8(input_wtext);
                            if (input_wtext) free(input_wtext); 
                            if (!input_utf8_text) {
                                ShowStatusMessage(L"Memory allocation or UTF-8 conversion failed.");
                                return 0; 
                            }
                            
                            char *error_msg_utf8 = NULL; 
                            char *output_utf8_text = decrypt_and_decode_string(input_utf8_text, keybase_manual, keyinc_manual, &error_msg_utf8);
                            if (input_utf8_text) free(input_utf8_text);
                            
                            if (output_utf8_text) {
                                wchar_t* output_wtext = utf8_to_wide(output_utf8_text);
                                free(output_utf8_text);
                                if (output_wtext) {
                                     SetEditText(hTextEdit, output_wtext);
                                     free(output_wtext); 
                                     ShowStatusMessage(L"Decoding successful.");
                                } else {
                                     ShowStatusMessage(L"UTF-16 conversion failed after decoding.");
                                }
                            } else {
                                wchar_t* error_msg_wstr = utf8_to_wide(error_msg_utf8 ? error_msg_utf8 : "An unknown error occurred during decoding.");
                                ShowStatusMessage(error_msg_wstr ? error_msg_wstr : L"An unknown error occurred during decoding.");
                                if (error_msg_utf8) free(error_msg_utf8);
                                if (error_msg_wstr) free(error_msg_wstr);
                            }
                            break;
                        }

                        case ID_ENCODE_BUTTON:
                        {
                            wchar_t *keybase_wstr = GetEditText(hKeybaseEdit);
                            wchar_t *keyinc_wstr = GetEditText(hKeyincEdit);
                            wchar_t *input_wtext_original = GetEditText(hTextEdit);
                            long keybase, keyinc;
                            wchar_t *endptr_base, *endptr_inc;
                            int keys_valid = 1;
                            ShowStatusMessage(L"");
                            
                            keybase = wcstol(keybase_wstr, &endptr_base, 10);
                            keyinc = wcstol(keyinc_wstr, &endptr_inc, 10);
                            
                            if (*keybase_wstr == L'\0' || *endptr_base != L'\0') {
                                ShowStatusMessage(L"Error: keybase value must be an integer.");
                                keys_valid = 0;
                            }
                            if (*keyinc_wstr == L'\0' || *endptr_inc != L'\0') {
                                ShowStatusMessage(L"Error: keyinc value must be an integer.");
                                keys_valid = 0;
                            }
                            
                            if (keybase_wstr) free(keybase_wstr);
                            if (keyinc_wstr) free(keyinc_wstr);
                            
                            if (!keys_valid) {
                                if (input_wtext_original) free(input_wtext_original); 
                                return 0;
                            }
                            
                            if (keybase < 0 || keybase >= BASE64_LEN || keyinc < 0 || keyinc >= BASE64_LEN) {
                                wchar_t msg[100];
                                swprintf_s(msg, 100, L"Error: Keys must be 0-%d.", BASE64_LEN - 1);
                                ShowStatusMessage(msg);
                                if (input_wtext_original) free(input_wtext_original); 
                                return 0;
                            }
                            
                            char *input_utf8_original = wide_to_utf8(input_wtext_original);
                            if (input_wtext_original) free(input_wtext_original); 
                            if (!input_utf8_original) {
                                ShowStatusMessage(L"Memory allocation or UTF-8 conversion failed.");
                                return 0; 
                            }
                            
                            char *input_utf8_normalized = normalize_crlf(input_utf8_original);
                            if (input_utf8_original) free(input_utf8_original);
                            if (!input_utf8_normalized) {
                                 ShowStatusMessage(L"Memory allocation failed during CRLF normalization.");
                                 return 0;
                            }
                            
                            char *error_msg_utf8 = NULL;
                            char *output_utf8_text = encode_and_encrypt_string(input_utf8_normalized, keybase, keyinc, &error_msg_utf8);
                            if (input_utf8_normalized) free(input_utf8_normalized);
                            
                            if (output_utf8_text) {
                                wchar_t* output_wtext = utf8_to_wide(output_utf8_text);
                                free(output_utf8_text);
                                if (output_wtext) {
                                     SetEditText(hTextEdit, output_wtext);
                                     free(output_wtext);
                                     ShowStatusMessage(L"Encoding successful.");
                                } else {
                                     ShowStatusMessage(L"UTF-16 conversion failed after encoding.");
                                }
                            } else {
                                wchar_t* error_msg_wstr = utf8_to_wide(error_msg_utf8 ? error_msg_utf8 : "An unknown error occurred during encoding.");
                                ShowStatusMessage(error_msg_wstr ? error_msg_wstr : L"An unknown error occurred during encoding.");
                                if (error_msg_utf8) free(error_msg_utf8);
                                if (error_msg_wstr) free(error_msg_wstr);
                            }
                            break;
                        }

                        case ID_COPY_BUTTON:
                        {
                            wchar_t *wtext_to_copy = GetEditText(hTextEdit);
                            if (wtext_to_copy && *wtext_to_copy != L'\0') {
                                if (OpenClipboard(hWnd)) {
                                    EmptyClipboard();
                                    HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, (wcslen(wtext_to_copy) + 1) * sizeof(wchar_t));
                                    if (hClipboardData) {
                                        wchar_t* pwchData = (wchar_t*)GlobalLock(hClipboardData);
                                        if (pwchData) {
                                            wcscpy_s(pwchData, wcslen(wtext_to_copy) + 1, wtext_to_copy);
                                            GlobalUnlock(hClipboardData);
                                            SetClipboardData(CF_UNICODETEXT, hClipboardData);
                                            ShowStatusMessage(L"Text copied to clipboard.");
                                        } else {
                                             GlobalFree(hClipboardData);
                                             ShowStatusMessage(L"Failed to lock clipboard memory for copy.");
                                        }
                                    } else {
                                        ShowStatusMessage(L"Failed to allocate global memory for clipboard.");
                                    }
                                    CloseClipboard(); 
                                } else {
                                    ShowStatusMessage(L"Failed to open clipboard.");
                                }
                            } else {
                                ShowStatusMessage(L"Text area is empty. Nothing to copy.");
                            }
                            if (wtext_to_copy) free(wtext_to_copy);
                            break;
                        }

                        case ID_PASTE_BUTTON:
                        {
                            if (OpenClipboard(hWnd)) { 
                                HGLOBAL hClipboardData = GetClipboardData(CF_UNICODETEXT); 
                                if (hClipboardData) {
                                    wchar_t* pwchData = (wchar_t*)GlobalLock(hClipboardData);
                                    if (pwchData) {
                                        if (*pwchData != L'\0') {
                                            SetEditText(hTextEdit, pwchData);
                                            ShowStatusMessage(L"Text pasted from clipboard.");
                                        } else {
                                             ShowStatusMessage(L"Clipboard contains empty text.");
                                        }
                                        GlobalUnlock(hClipboardData); 
                                    } else {
                                         ShowStatusMessage(L"Failed to lock clipboard memory for paste.");
                                    }
                                } else {
                                    ShowStatusMessage(L"Clipboard does not contain plain text.");
                                }
                                CloseClipboard();
                            } else {
                                ShowStatusMessage(L"Failed to open clipboard for paste.");
                            }
                            break;
                        }

                        case ID_CLEAR_CANVAS_BUTTON:
                        {
                            SetEditText(hTextEdit, L""); 
                            ShowStatusMessage(L"Text area cleared.");
                            break;
                        }

                        case ID_CLEAR_CLIPBOARD_BUTTON:
                        {
                             if (OpenClipboard(hWnd)) {
                                 EmptyClipboard();
                                 CloseClipboard();
                                 ShowStatusMessage(L"Clipboard cleared.");
                             } else {
                                 ShowStatusMessage(L"Failed to open clipboard to clear.");
                             }
                             break;
                        }
                    }
                }
            }
            break;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEXW wc; 
    HWND hWnd;
    MSG Msg;
    
    INITCOMMONCONTROLSEX iccex;
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&iccex);
    
    wc.cbSize        = sizeof(WNDCLASSEXW); 
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = L"YasWinAppClass";
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
    
    if (!RegisterClassExW(&wc)) {
        MessageBoxW(NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK); 
        return 1;
    }
    
    hWnd = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"YasWinAppClass",
        L"Yet Another SCOS",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 480,
        NULL,
        NULL,
        hInstance,
        NULL);
        
    if (hWnd == NULL) {
        MessageBoxW(NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK); 
        return 1;
    }
    
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    
    while (GetMessage(&Msg, NULL, 0, 0) > 0) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    
    return (int)Msg.wParam;
}