// ==WindhawkMod==
// @id              fully-custom-winver
// @name            Customozimable WinVer
// @description     Fully customize your winver.exe by even "replacing" the logo!
// @version         1.0.0
// @author          ItsTauTvyDas
// @github          https://github.com/ItsTauTvyDas
// @twitter         https://x.com/ItsTauTvyDas
// @homepage        https://itstautvydas.me
// @include         winver.exe
// @compilerOptions -lgdiplus -lgdi32 -luser32 -lole32 -luuid -ldwmapi -lcomctl32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Customizable winver.exe

Features:
* Modify any text
* Custom logo (it just creates a custom child window in the logo area)
* Change the title
* Change window parameters (make it always top, custom size, open location)
* Change background colors

All control ids were viewed with **winspy** application.

Available palceholders:
* `{default.<id>}` - Default control's text. Replace `<id>` with control ID, so if **Product name** is selected, ID would be **ProductName** (basically capitalzie each word and no spaces).
* `{default.<id>::5}` - Removes first 5 characters from control's text.
* `{default.<id>::5/9}` - Substrings control's text, removes 5 first characters and only returns next 9 characters.

Examples with `{default...}` placeholders:
* `{default.Copyright::0/1} Yoursoft {default.Copyright::12}` - take the copyright sign (get text from 0th to 1th place), show your text and add back the remaining text. This basically replaces "Microsoft" word.

To add back the **license info** link use `<A>` and `</A>` (like html) - `Click <A>here</A> to view license information`
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- logo:
  - enabled: TRUE
    $name: "Enabled"
  - center: TRUE
    $name: "Center"
  - x: 0
    $name: "X"
  - y: 10
    $name: "Y"
  - width: 0
    $name: "Width"
    $description: "Set to 0 for full image's width"
  - height: 0
    $name: "Height"
    $description: "Set to 0 for max height needed (72 pixels)"
  - path: "C:/Windows/System32/DisplaySystemToastIcon.png"
    $name: "Path to the logo"
    $description: "Can be .png, .jpg or .bmp"
  $name: "Custom logo"
- startPosition:
  - enabled: TRUE
    $name: "Enabled"
  - center: TRUE
    $name: "Center the window"
    $description: "If enabled, X and Y will act as offsets"
  - monitor: 0
    $name: "Monitor"
    $description: "If 0 then it chooses monitor where the window appeared by default"
  - x: 0
    $name: "X"
  - y: 0
    $name: "Y"
  $name: "Window open position"
- customIcon: "shell32.dll;23"
  $name: "Custom widnow icon"
  $description: "Can be either DLL with icon index (shell32.dll;23) or absolute path to .ico, .cur or .bmp"
- removeTitleIcon: FALSE
  $name: "Remove icon from title bar"
- restoreCloseButton: TRUE
  $name: "Show close button"
  $description: "Enable this if you want to still see clsoe button after titlebar icon removal"
- windowColors:
  - title: ""
    $name: "Title text color"
  - border: ""
    $name: "Border's color"
  - bar: ""
    $name: "Title bar color"
  - background: ""
    $name: "Background color"
  - separator: ""
    $name: "Separator color"
  $name: "Change window colors"
  $description: "Can be left empty for default. Specify color in RGB (e.g., 255, 16, 74)"
- titleText: ""
  $name: "Custom title text"
  $description: "Add a space if you don't want title at all"
- modifyWindows:
  - - id: ProductName
      $options:
      - LeftIcon: "Left icon"
      - HorizontalLine: "Horizontal line"
      - ProductName: "Product name"
      - Version: "Version"
      - Copyright: "Copyright"
      - Spacer1: "Spacer 1"
      - LegalNotice: "Legal notice"
      - Spacer2: "Spacer 2"
      - LicenseInfo: "License info"
      - RegisteredUser1: "Registered user 1"
      - RegisteredUser2: "Registered user 2"
      - OkButton: "Ok button"
      $name: "Label"
    - label: "Yoursoft Windows"
      $name: "Rename label"
      $description: "Check mod's description for placeholders use!"
    - hidden: FALSE
      $name: "Hidden"
    - position: ""
      $name: "Location"
      $description: "Format: x;y;width;height, d stands for default which will use original value. If nothing is specified, position is defaulted"
  $name: Modify controls
          
# - title: "{default}"
#   $name: Modify child windows
*/
// ==/WindhawkModSettings==

#include <windows.h>
#include <format>
#include <string>
#include <unordered_map>
#include <windhawk_utils.h>
#include <gdiplus.h>
#include <vector>
#include <dwmapi.h>

#define MOD_TITLE L"WindHawk: Customozimable WinVer"
#define LOGO_BACKGROUND_ID 100
#define LOGO_IMAGE_ID 101
#define MAX_LOGO_HEIGHT 72
#define DEFAULT_BAR_BACKGROUND RGB(241, 243, 249)
#define DEFAULT_BACKGROUND RGB(240, 240, 240)

#ifndef WH_EDITING
#define PrintRect(name, rect) Wh_Log(L"%ls RECT -> [%d, %d, %d, %d], calucated width and height -> [%d, %d]", name, rect.left, rect.top, rect.right, rect.bottom, rect.right - rect.left, rect.bottom - rect.top)
#else
#define PrintRect(name, rect) ((void)0)
#endif

struct WindowAttributes {
    std::wstring id, label;
    BOOL hidden;
    INT x, y, width, height;
};

struct DefinedWindowAttributes : WindowAttributes {
    std::wstring position;
    void pos();
};

static const std::unordered_map<std::wstring, INT> g_controlIds = {
    {L"LeftIcon",        0x3009},
    {L"Separator",       0x3327},
    {L"ProductName",     0x3500},
    {L"Version",         0x350B},
    {L"Copyright",       0x350A},
    {L"Spacer1",         0xCA03},
    {L"LegalNotice",     0x3513},
    {L"Spacer2",         0x350D},
    {L"LicenseInfo",     0x3512},
    {L"RegisteredUser1", 0x3507},
    {L"RegisteredUser2", 0x3508},
    {L"OkButton",        0x0001}
};

struct MonitorInfoEx {
    HMONITOR handle;
    MONITORINFOEX info;
};

LRESULT CALLBACK WndSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

std::unordered_map<std::wstring, WindowAttributes> g_defaultWindowAttributes;
std::unordered_map<std::wstring, DefinedWindowAttributes> g_definedWindowAttributes;
std::vector<std::wstring> errorMessages;
HWINEVENTHOOK g_hHook = nullptr;
HWND window = nullptr, logo_background = nullptr, logo_image = nullptr;
HBITMAP custom_logo = nullptr;

COLORREF windowBackgroundColor = DEFAULT_BACKGROUND, separatorColor = NULL;
BOOL hasSeparatorColor = FALSE;

INT DefaultPosition(INT i, const std::wstring& id) {
    auto dwa = g_defaultWindowAttributes[id];
    switch (i) {
        case 0: return dwa.x;
        case 1: return dwa.y;
        case 2: return dwa.width;
        case 3: return dwa.height;
        default: return NULL;
    }
}

void DefinedWindowAttributes::pos() {
    if (position.empty()) {
        x = DefaultPosition(0, id);
        y = DefaultPosition(1, id);
        width = DefaultPosition(2, id);
        height = DefaultPosition(3, id);
        return;
    }
    INT* array[] = { &x, &y, &width, &height };

    size_t start = 0, pos;
    for (INT i = 0; i < 4; i++) {
        pos = position.find(L';', start);
        if (pos == std::wstring::npos) {
            pos = position.size();
        }
        if (pos < start) {
            *array[i] = DefaultPosition(i, id);
            continue;
        }
        INT value;
        auto part = position.substr(start, pos - start);
        if (part.empty() || part == L"d") {
            value = DefaultPosition(i, id);
        } else {
            value = _wtoi(part.c_str());
        }
        *array[i] = value;
        start = pos + 1;
    }
}

HICON LoadCustomIcon(const std::wstring& source) {
    size_t pos = source.find(L';');
    // <dll>;<index>
    if (pos != std::wstring::npos) {
        auto dllPath = source.substr(0, pos);
        INT index = _wtoi(source.substr(pos + 1).c_str());

        HICON hIcon = nullptr;
        ExtractIconExW(dllPath.c_str(), index, &hIcon, nullptr, 1);
        return hIcon;
    }

    // full path to .ico
    return (HICON)LoadImageW(nullptr, source.c_str(), IMAGE_ICON, 32, 32, LR_LOADFROMFILE | LR_DEFAULTCOLOR);
}

WindowAttributes GetWindowAttributes(HWND hWnd, const std::wstring& id) {
    WindowAttributes attr = {};
    attr.id = id;

    RECT rect;
    if (GetWindowRect(hWnd, &rect)) {
        MapWindowPoints(HWND_DESKTOP, GetParent(hWnd), reinterpret_cast<POINT*>(&rect), 2);
        attr.x = rect.left;
        attr.y = rect.top;
        attr.width = rect.right - rect.left;
        attr.height = rect.bottom - rect.top;
    }

    wchar_t buffer[256];
    INT len = GetWindowTextW(hWnd, buffer, 256);
    if (len > 0) {
        attr.label.assign(buffer, len);
    }

    return attr;
}

void ReplaceAll(std::wstring& str, const std::wstring& from, const std::wstring& to) {
    if (from.empty()) {
        return;
    }
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::wstring::npos) {
        str.replace(pos, from.length(), to);
        pos += to.length();
    }
}

BOOL LoadColor(const std::wstring& settingsName, COLORREF& color) {
    auto colorStr = Wh_GetStringSetting(settingsName.c_str());
    if (*colorStr) {
        INT r = 0, g = 0, b = 0;
        if (swscanf(colorStr, L"%d%*[,]%d%*[,]%d", &r, &g, &b) == 3) {
            color = RGB(r, g, b);
            Wh_FreeStringSetting(colorStr);
            return TRUE;
        }
    } 
    Wh_FreeStringSetting(colorStr);
    return FALSE;
}

void LoadColor(const std::wstring& settingsName, COLORREF& color, COLORREF defaultColor) {
    if (!LoadColor(settingsName, color)) {
        color = defaultColor;
    }
}

void LoadSettings() {
    g_definedWindowAttributes.clear();

    for (INT i = 0;; i++) {
        auto option = WindhawkUtils::StringSetting(Wh_GetStringSetting(L"modifyWindows[%d].id", i));
        if (!*option) {
            break;
        }

        auto label = WindhawkUtils::StringSetting(Wh_GetStringSetting(L"modifyWindows[%d].label", i));
        auto position = WindhawkUtils::StringSetting(Wh_GetStringSetting(L"modifyWindows[%d].position", i));

        DefinedWindowAttributes attrs = {};
        attrs.id = option;
        attrs.label = *label ? label.get() : L"";
        attrs.position = *position ? position.get() : L"";
        attrs.hidden = Wh_GetIntSetting(L"modifyWindows[%d].hidden", i);
        g_definedWindowAttributes[option.get()] = attrs;
    }

    LoadColor(L"windowColors.background", windowBackgroundColor, DEFAULT_BACKGROUND);
    hasSeparatorColor = LoadColor(L"windowColors.separator", separatorColor);
}

RECT LoadPosition(const std::wstring& settingsPrefix) {
    RECT rect{};
    rect.left = std::max(0, Wh_GetIntSetting((settingsPrefix + L".x").c_str()));
    rect.top = std::max(0, Wh_GetIntSetting((settingsPrefix + L".y").c_str()));
    rect.right = Wh_GetIntSetting((settingsPrefix + L".width").c_str());
    rect.bottom = Wh_GetIntSetting((settingsPrefix + L".height").c_str());
    return rect;
}

std::wstring _substr(std::wstring str, size_t pos, INT npos) {
    if (npos < 0) {
        return str.substr(pos);
    }
    return str.substr(pos, npos);
}

void ApplyPlaceholder(std::wstring& str) {
    for (auto& id : g_controlIds) {
        auto label = g_defaultWindowAttributes[id.first].label;
        std::wstring placeholder0 = L"{default." + id.first + L"}";
        ReplaceAll(str, placeholder0, label);
        for (size_t pos = 0; (pos = str.find(L"{default." + id.first, pos)) != std::wstring::npos;) {
            std::wstring placeholder1 = L"{default." + id.first + L"::";
            if (str.contains(placeholder1)) {
                size_t placeholderPos = str.find(placeholder1);
                size_t placeholderPosEnd = placeholder1.length();
                INT current = 0;
                INT positions[2] = {
                    0, -1
                };
                std::wstring position = L"";
                for (size_t pos = placeholderPos + placeholder1.length(); pos < str.length(); pos++) {
                    placeholderPosEnd++;
                    if (iswdigit(str[pos])) {
                        position += str[pos];
                    } else if (str[pos] == L'/' || str[pos] == L'}') {
                        // Very dirty way of doing this lol
                        positions[current] = stoi(position);
                        if (str[pos] == L'}') {
                            break;
                        }
                        position = L"";
                        current = 1;
                    }
                }
                ReplaceAll(str, str.substr(placeholderPos, placeholderPosEnd), label.substr(positions[0], positions[1]));
            }
        }
    }
}

HBITMAP LoadImageFile(const std::wstring& path, INT width, INT height, RECT* rect, COLORREF background) {
    static ULONG_PTR gdiToken = 0;
    if (gdiToken == 0) {
        Gdiplus::GdiplusStartupInput input;
        if (Gdiplus::GdiplusStartup(&gdiToken, &input, nullptr) != Gdiplus::Ok) {
            return nullptr;
        }
    }

    std::unique_ptr<Gdiplus::Image> img(Gdiplus::Image::FromFile(path.c_str(), FALSE));
    if (!img || img->GetLastStatus() != Gdiplus::Ok) {
        return nullptr;
    }

    HBITMAP hbmp = nullptr;
    Gdiplus::Color bg(GetRValue(background), GetGValue(background), GetBValue(background));

    if (height == 0) {
        height = MAX_LOGO_HEIGHT;
    }

    if (width == 0 && height > 0) {
        width = static_cast<int>(width * MAX_LOGO_HEIGHT / height);
    }

    if (rect) {
        rect->right = width > 0 ? width : img->GetWidth();
        rect->bottom = height > 0 ? height : img->GetHeight();
    }

    if (width > 0 && height > 0) {
        Gdiplus::Bitmap bitmap(width, height, PixelFormat32bppPARGB);
        Gdiplus::Graphics g(&bitmap);
        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        g.DrawImage(img.get(), Gdiplus::Rect(0, 0, width, height), 0, 0, img->GetWidth(), img->GetHeight(), Gdiplus::UnitPixel);
        if (bitmap.GetHBITMAP(bg, &hbmp) == Gdiplus::Ok) {
            return hbmp;
        }
    } else {
        if (img->GetType() == Gdiplus::ImageTypeBitmap) {
            auto bitmap = static_cast<Gdiplus::Bitmap*>(img.get());
            if (bitmap->GetHBITMAP(bg, &hbmp) == Gdiplus::Ok) {
                return hbmp;
            }
        }
        Gdiplus::Bitmap bitmap(img->GetWidth(), img->GetHeight(), PixelFormat32bppPARGB);
        Gdiplus::Graphics g(&bitmap);
        g.DrawImage(img.get(), 0, 0);
        if (bitmap.GetHBITMAP(bg, &hbmp) == Gdiplus::Ok) {
            return hbmp;
        }
    }
    return nullptr;
}

BOOL CALLBACK EnumMonitorsProc(HMONITOR handle, HDC, LPRECT, LPARAM monitorsParam) {
    auto monitors = reinterpret_cast<std::vector<MonitorInfoEx>*>(monitorsParam);
    MonitorInfoEx info = {};
    info.handle = handle;
    info.info.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfoW(handle, &info.info);
    monitors->push_back(info);
    return TRUE;
}

BOOL FileExists(PCWSTR path) {
    if (!path || !*path) {
        return false;
    }
    DWORD attrs = GetFileAttributesW(path);
    return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

void DeleteLogoImage() {
    HBITMAP oldImage = (HBITMAP)SendMessageW(logo_image, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)NULL);
    if (oldImage) {
        DeleteObject(oldImage);
    }
}

void CenterWindow(HWND hWnd, RECT& rect, INT& x, INT& y) {
    std::vector<MonitorInfoEx> monitors;
    EnumDisplayMonitors(nullptr, nullptr, EnumMonitorsProc, reinterpret_cast<LPARAM>(&monitors));

    HMONITOR monitor = nullptr;
    const RECT* monitorRect = nullptr;

    INT index = Wh_GetIntSetting(L"startPosition.monitor") - 1;
    if (index < 0 || index >= (INT)monitors.size()) {
        monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(MONITORINFO) };
        if (!GetMonitorInfoW(monitor, &mi)) {
            return;
        }
        monitorRect = &mi.rcWork;
    } else {
        monitor = monitors[index].handle;
        monitorRect = &monitors[index].info.rcWork;
    }

    x = monitorRect->left + ((monitorRect->right - monitorRect->left) - (rect.right - rect.left)) / 2;
    y = monitorRect->top  + ((monitorRect->bottom - monitorRect->top) - (rect.bottom - rect.top)) / 2;
}

void RedrawNonClientWindow(HWND hWnd) {
    // SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    RedrawWindow(hWnd, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
}

void UpdatePosition(HWND hWnd) {
    if (!Wh_GetIntSetting(L"startPosition.enabled")) {
        return;
    }

    RECT rectWindow, rectOffset = LoadPosition(L"startPosition");
    GetWindowRect(hWnd, &rectWindow);

    INT x = 0, y = 0;
    if (Wh_GetIntSetting(L"startPosition.center")) {
        CenterWindow(hWnd, rectWindow, x, y);
    }
    x += rectOffset.left;
    y += rectOffset.top;

    Wh_Log(L"Positioning window at %d %d", x, y);

    SetWindowPos(hWnd, nullptr, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

void UpdateWindowColors(HWND hWnd) {
    COLORREF color;
    LoadColor(L"windowColors.title", color, GetSysColor(COLOR_CAPTIONTEXT));
    DwmSetWindowAttribute(hWnd, DWMWA_TEXT_COLOR, &color, sizeof(color));
    LoadColor(L"windowColors.bar", color, DEFAULT_BAR_BACKGROUND);
    DwmSetWindowAttribute(hWnd, DWMWA_CAPTION_COLOR, &color, sizeof(color));
    LoadColor(L"windowColors.border", color, GetSysColor(COLOR_WINDOW));
    DwmSetWindowAttribute(hWnd, DWMWA_BORDER_COLOR, &color, sizeof(color));
}

void UpdateTitle(HWND hWnd) {
    static WCHAR originalTitle[256] = L"";
    if (originalTitle[0] == L'\0') {
        GetWindowTextW(hWnd, originalTitle, ARRAYSIZE(originalTitle));
    }
    auto titleText = Wh_GetStringSetting(L"titleText");
    if (titleText && *titleText) {
        SetWindowTextW(hWnd, titleText);
    } else {
        SetWindowTextW(hWnd, originalTitle);
    }
    Wh_FreeStringSetting(titleText);
}

void UpdateLogo(HWND hWnd) {
    if (logo_image) {
        DeleteLogoImage();
        ShowWindow(logo_image, SW_HIDE);
    }

    RECT clientRect;
    GetClientRect(hWnd, &clientRect);

    if (!logo_background) {
        logo_background = CreateWindowExW(
            WS_EX_NOPARENTNOTIFY,
            L"Static",
            L"",
            WS_CHILD | WS_VISIBLE,
            0, 0, clientRect.right, MAX_LOGO_HEIGHT,
            hWnd,
            (HMENU)1,
            GetModuleHandle(nullptr),
            nullptr
        );
        SetWindowSubclass(logo_background, WndSubclassProc, 1, LOGO_BACKGROUND_ID);
        Wh_Log(L"Logo's background handle -> %p", logo_background);
    }

    if (!logo_image) {
        logo_image = CreateWindowExW(
            0,
            L"Static",
            L"",
            WS_CHILD | WS_VISIBLE | SS_BITMAP,
            0, 0, 0, 0,
            hWnd,
            (HMENU)1,
            GetModuleHandle(nullptr),
            nullptr
        );
        SetWindowSubclass(logo_image, WndSubclassProc, 1, LOGO_IMAGE_ID);
        Wh_Log(L"Logo's image handle -> %p", logo_image);
    }

    if (!Wh_GetIntSetting(L"logo.enabled")) {
        ShowWindow(logo_background, SW_HIDE);
        return;
    }

    auto path = Wh_GetStringSetting(L"logo.path");
    if (!path || !*path || !FileExists(path)) {
        Wh_Log(L"Invalid logo file!");
        errorMessages.push_back(std::format(L"Path to custom logo is invalid or the file doesn't exist!\n\n{}", path));
        Wh_FreeStringSetting(path);
        ShowWindow(logo_background, SW_HIDE);
        return;
    }

    RECT rect = LoadPosition(L"logo");

    // if (rect.bottom == 0) {
    //     rect.bottom = 72;
    // }

    COLORREF background = DEFAULT_BACKGROUND;
    LoadColor(L"windowColors.background", background);

    Wh_Log(L"Loading %ls", path);
    custom_logo = LoadImageFile(path, rect.right, rect.bottom, &rect, background);
    if (custom_logo) {
        Wh_Log(L"Logo image set");
        if (Wh_GetIntSetting(L"logo.center")) {
            PrintRect(L"Logo position before centering", rect);
            rect.left = (clientRect.right - clientRect.left) / 2 - rect.right / 2;
        }
        MoveWindow(logo_image, rect.left, rect.top, rect.right, rect.bottom, TRUE);
        PrintRect(L"Logo position", rect);
    } else {
        Wh_Log(L"Failed to load logo: %ls", GetLastError());
    }
    Wh_FreeStringSetting(path);
    ShowWindow(logo_background, SW_SHOW);
    ShowWindow(logo_image, SW_SHOW);
}

void UpdateControls(HWND hWnd) {
    if (g_defaultWindowAttributes.empty()) {
        for (auto& id : g_controlIds) {
            HWND item = GetDlgItem(hWnd, id.second);
            if (!item) {
                Wh_Log(L"Failed to find %ls (%d) window", id.first.c_str(), id.second);
                continue;
            }
            g_defaultWindowAttributes[id.first] = GetWindowAttributes(item, id.first);
        }
    }

    for (auto& entry : g_definedWindowAttributes) {
        entry.second.pos();
        auto id = g_controlIds.find(entry.first);
        if (id != g_controlIds.end()) {
            HWND item = GetDlgItem(hWnd, id->second);
            if (!item) {
                continue;
            }

            Wh_Log(L"Found child window %ls (%d)", id->first.c_str(), id->second);

            // Modify caption
            auto label = entry.second.label;
            ApplyPlaceholder(label);
            SetWindowTextW(item, label.c_str());

            // Modify visibility
            ShowWindow(item, entry.second.hidden ? SW_HIDE : SW_SHOW);
            
            // Modify position
            MoveWindow(item, entry.second.x, entry.second.y, entry.second.width, entry.second.height, TRUE);

            #ifndef WH_EDITING
            RECT rect;
            GetClientRect(item, &rect);
            PrintRect(L"Window position", rect);
            #endif
        }
    }
}

void SetCustomIcon(HWND hWnd) {
    auto customIcon = Wh_GetStringSetting(L"customIcon");
    if (*customIcon) {
        HICON hIcon = LoadCustomIcon(customIcon);
        if (hIcon) {
            SendMessageW(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            if (Wh_GetIntSetting(L"removeTitleIcon")) {
                SendMessageW(hWnd, WM_SETICON, ICON_SMALL, 0);
                LONG style = GetWindowLongW(hWnd, GWL_STYLE);
                style &= ~WS_SYSMENU;
                SetWindowLongW(hWnd, GWL_STYLE, style);
            } else {
                SendMessageW(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            }
        } else {
            errorMessages.push_back(std::format(L"Failed to load icon ({}), please check the settings!", customIcon));
        }
    }
    Wh_FreeStringSetting(customIcon);
}

void ToggleCloseButton(HWND hWnd) {
    if (Wh_GetIntSetting(L"restoreCloseButton")) {
        LONG style = GetWindowLongW(hWnd, GWL_STYLE);
        style |= WS_SYSMENU;
        SetWindowLongW(hWnd, GWL_STYLE, style);
    }
}

void Update(HWND hWnd) {
    Wh_Log(L"UPDATING CUSTOM ICON");
    SetCustomIcon(hWnd);
    Wh_Log(L"UPDATING POSITION");
    UpdatePosition(hWnd);
    Wh_Log(L"UPDATING CONTROLS");
    UpdateControls(hWnd);
    Wh_Log(L"UPDATING LOGO");
    UpdateLogo(hWnd);
    Wh_Log(L"UPDATING TITLE");
    UpdateTitle(hWnd);
    Wh_Log(L"UPDATING WINDOW COLORS");
    UpdateWindowColors(hWnd);
    Wh_Log(L"DONE");
}

void ShowErrorsIfAny(HWND hWnd) {
    for (auto& message : errorMessages) {
        MessageBoxW(hWnd, message.c_str(), MOD_TITLE, MB_OK | MB_ICONERROR);
    }
    errorMessages.clear();
}

// Window processing for both main window and its children (dwRefData = 0 is main window, anything else - child)
LRESULT CALLBACK WndSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    static COLORREF c_windowBackgroundColor, c_separatorColor;
    static HBRUSH c_windowBackgroundBrush, c_separatorBrush;
    static const INT separatorId = g_controlIds.find(L"Separator")->second;

    if (c_windowBackgroundColor != windowBackgroundColor) {
        c_windowBackgroundBrush = CreateSolidBrush(windowBackgroundColor);
        c_windowBackgroundColor = windowBackgroundColor;
    }

    if (hasSeparatorColor && c_separatorColor != separatorColor) {
        c_separatorBrush = CreateSolidBrush(separatorColor);
        c_separatorColor = separatorColor;
    }

    if ((uMsg == WM_CTLCOLORSTATIC || uMsg == WM_CTLCOLORBTN) && dwRefData == 0) { // Set background color for controls
        SetBkMode((HDC)wParam, TRANSPARENT);
        return reinterpret_cast<LRESULT>(c_windowBackgroundBrush);
    } else if (uMsg == WM_PAINT) {
        if (dwRefData == 0) { // Set background color for window itself
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            FillRect(hdc, &ps.rcPaint, c_windowBackgroundBrush);
            EndPaint(hWnd, &ps);
            return 0;
        } else if (dwRefData == LOGO_IMAGE_ID && custom_logo) { // Draw custom logo
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, custom_logo);
            BITMAP bmp;
            GetObjectW(custom_logo, sizeof(bmp), &bmp);
            BitBlt(hdc, 0, 0, bmp.bmWidth, bmp.bmHeight, hdcMem, 0, 0, SRCCOPY);
            SelectObject(hdcMem, hbmOld);
            DeleteDC(hdcMem);
            EndPaint(hWnd, &ps);
            return 0;
        } else if (static_cast<INT>(dwRefData) == separatorId && hasSeparatorColor) { // Recolor separator
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT mRect;
            GetClientRect(hWnd, &mRect);
            // There seems to be some white pixels left out on both sides
            mRect.left -= 1;
            mRect.right += 1;
            FrameRect(hdc, &mRect, c_separatorBrush);
            EndPaint(hWnd, &ps);
            return 0;
        }
    } else if (uMsg == WM_DESTROY && dwRefData == 0) {
        DeleteObject(c_windowBackgroundBrush);
        DeleteObject(c_separatorBrush);
        c_windowBackgroundBrush = nullptr;
        c_separatorBrush = nullptr;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void CALLBACK WinEventProc(HWINEVENTHOOK, DWORD event, HWND hWnd, LONG idObject, LONG, DWORD, DWORD) {
    if (idObject != OBJID_WINDOW || !hWnd || !IsWindow(hWnd)) {
        return;
    }

    if (event == EVENT_OBJECT_CREATE && !window) {
        if (!GetWindow(hWnd, GW_OWNER)) {
            window = hWnd;
            Wh_Log(L"Window created");
            SetWindowSubclass(hWnd, WndSubclassProc, 1, 0);
            for (auto& entry : g_controlIds) {
                HWND item = GetDlgItem(hWnd, entry.second);
                if (!item) {
                    Wh_Log(L"Failed to find window child (%p)", entry.second);
                    continue;
                }
                SetWindowSubclass(item, WndSubclassProc, 1, entry.second);
            }
            Update(hWnd);
        }
    } else if (window == hWnd) {
        Wh_Log(L"Window showed");
        ToggleCloseButton(hWnd);
        UnhookWinEvent(g_hHook); // No more needed
        g_hHook = nullptr;
        ShowErrorsIfAny(hWnd);
    }
}

BOOL CALLBACK EnumMessageBoxProc(HWND hwnd, LPARAM) {
    if (GetWindow(hwnd, GW_OWNER) == window) {
        wchar_t cls[64];
        GetClassNameW(hwnd, cls, _countof(cls));
        if (wcscmp(cls, L"#32770") == 0) {
            PostMessageW(hwnd, WM_CLOSE, 0, 0);
            return FALSE;
        }
    }
    return TRUE;
}

BOOL Wh_ModInit() {
    Wh_Log(L"Init");
    LoadSettings();

    g_hHook = SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_SHOW, nullptr, WinEventProc, GetCurrentProcessId(), 0, WINEVENT_OUTOFCONTEXT);
    return TRUE;
}

void Wh_ModUninit() {
    Wh_Log(L"Uninit");
    if (g_hHook) {
        UnhookWinEvent(g_hHook);
    }
    if (IsWindow(window)) {
        EnumWindows(EnumMessageBoxProc, 0);
    }
    if (IsWindow(logo_image)) {
        DeleteLogoImage();
        DestroyWindow(logo_image);
        CloseWindow(logo_image);
    }
    if (IsWindow(logo_background)) {
        DestroyWindow(logo_background);
        CloseWindow(logo_background);
    }
}

void Wh_ModSettingsChanged() {
    Wh_Log(L"Settings changed");
    LoadSettings();
    if (IsWindow(window)) {
        Update(window);
        RedrawNonClientWindow(window);
        ToggleCloseButton(window);
        ShowErrorsIfAny(window);
    }
}
