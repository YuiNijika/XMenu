// --------------------------------------------------------------------------
// 仅供 main.cpp #include 使用
// --------------------------------------------------------------------------
#ifndef XMENU_INSTALLER_MAIN_CPP
#error "Ui.cpp must be #included from installer/main.cpp (not compiled as its own TU)"
#endif

    enum class WizardStep {
        Version = 0,
        Path = 1,
        Components = 2,
        Install = 3
    };

    struct FetchResult {
        bool ok = false;
        bool cancelled = false;
        std::wstring error;
        ReleaseInfo release;
    };

    struct InstallResult {
        bool ok = false;
        bool cancelled = false;
        std::wstring error;
        std::wstring summary;
        std::wstring version;
    };

    struct QuestionRequest {
        std::wstring title;
        std::wstring text;
        unsigned int flags = MB_OK;
        int result = IDOK;
        HANDLE completed = nullptr;
    };

    struct InstallerUiState {
        HINSTANCE instance = nullptr;
        HWND window = nullptr;
        DWORD uiThreadId = 0;
        int dpi = 96;

        // 左侧导航
        HWND stepButtons[4] = {};
        HWND brandTitle = nullptr;
        HWND brandSubtitle = nullptr;
        HWND headerTitle = nullptr;
        HWND headerSubtitle = nullptr;
        HWND notesCaption = nullptr;

        // 版本页
        HWND versionSourceText = nullptr;
        HWND releaseVersionText = nullptr;
        HWND packageVersionText = nullptr;
        HWND releaseNotesEdit = nullptr;
        HWND refreshButton = nullptr;
        HWND openGtamodxButton = nullptr;
        HWND openGithubButton = nullptr;

        // 目录页
        HWND pathEdit = nullptr;
        HWND browseButton = nullptr;
        HWND gameTypeText = nullptr;
        HWND localVersionText = nullptr;

        // 组件页
        HWND moduleIII = nullptr;
        HWND moduleVC = nullptr;
        HWND moduleSA = nullptr;
        HWND rootDependencies = nullptr;
        HWND downloadSource = nullptr;

        // 安装页
        HWND summaryEdit = nullptr;
        HWND installButton = nullptr;
        HWND statusText = nullptr;
        HWND progressBar = nullptr;
        HWND logBox = nullptr;

        // 底部
        HWND backButton = nullptr;
        HWND nextButton = nullptr;
        HWND cancelButton = nullptr;

        HFONT titleFont = nullptr;
        HFONT sectionFont = nullptr;
        HFONT normalFont = nullptr;
        HFONT smallFont = nullptr;
        HBRUSH windowBrush = nullptr;
        HBRUSH sidebarBrush = nullptr;
        HBRUSH dividerBrush = nullptr;
        HBRUSH accentBrush = nullptr;
        HBRUSH accentHoverBrush = nullptr;
        HBRUSH accentPressedBrush = nullptr;
        HBRUSH secondaryBrush = nullptr;
        HBRUSH secondaryHoverBrush = nullptr;
        HBRUSH secondaryPressedBrush = nullptr;
        HBRUSH disabledBrush = nullptr;
        HBRUSH navHoverBrush = nullptr;
        HBRUSH cardBorderBrush = nullptr;
        HBRUSH editBrush = nullptr;
        HBRUSH logBrush = nullptr;

        HDC bufferDc = nullptr;
        HBITMAP bufferBitmap = nullptr;
        int bufferWidth = 0;
        int bufferHeight = 0;

        std::unordered_map<HWND, bool> buttonHover;

        std::string gameRoot;
        GameType gameType = GameType::Unknown;
        ReleaseInfo currentRelease;
        bool isReleaseFetched = false;
        WizardStep step = WizardStep::Version;
        std::atomic<bool> busy{false};
        std::atomic<bool> cancelRequested{false};
        std::atomic<int> lastPostedPercent{-1};
        std::thread worker;

        bool marquee = false;
        float marqueePosition = 0.0f;
        float marqueeDirection = 1.0f;
    };

    constexpr int ControlBrowse = 1001;
    constexpr int ControlInstall = 1002;
    constexpr int ControlModuleIII = 1003;
    constexpr int ControlModuleVC = 1004;
    constexpr int ControlModuleSA = 1005;
    constexpr int ControlRootDependencies = 1006;
    constexpr int ControlDownloadSource = 1007;
    constexpr int ControlFetchRelease = 1008;
    constexpr int ControlBack = 1009;
    constexpr int ControlNext = 1010;
    constexpr int ControlOpenGtamodx = 1011;
    constexpr int ControlOpenGithub = 1012;
    constexpr int ControlCancel = 1013;
    constexpr int ControlStepVersion = 1100;
    constexpr int ControlStepPath = 1101;
    constexpr int ControlStepComponents = 1102;
    constexpr int ControlStepInstall = 1103;

    constexpr UINT MsgStatus = WM_APP + 1;
    constexpr UINT MsgProgress = WM_APP + 2;
    constexpr UINT MsgLog = WM_APP + 3;
    constexpr UINT MsgFetchDone = WM_APP + 4;
    constexpr UINT MsgInstallDone = WM_APP + 5;
    constexpr UINT MsgQuestion = WM_APP + 6;

    constexpr UINT_PTR TimerMarquee = 1;

    // 布局常量，所有控件坐标都从这里推导，避免各写一套导致错位
    constexpr int WindowWidth = 960;
    constexpr int WindowHeight = 680;
    constexpr int MinWindowWidth = 860;
    constexpr int MinWindowHeight = 600;
    constexpr int SidebarWidth = 208;
    constexpr int SidebarPadding = 18;
    constexpr int ContentPadding = 28;
    constexpr int ContentLeft = SidebarWidth + ContentPadding;
    constexpr int HeaderLine = 104;
    constexpr int FooterHeight = 68;
    constexpr int BodyTop = HeaderLine + 26;
    constexpr int RowGap = 10;
    constexpr int RowHeight = 24;
    constexpr int FieldHeight = 32;
    constexpr int ButtonHeight = 34;
    constexpr int ButtonWidth = 132;
    constexpr int FooterButtonWidth = 116;

    const wchar_t* const StepLabels[] = {
        L"1  检查版本",
        L"2  选择目录",
        L"3  选择组件",
        L"4  确认安装"
    };

    InstallerUiState gUi;

    // 语义色板，明暗两套，控件与自绘文字都从这里取色
    enum class UiColorRole {
        Window,
        Sidebar,
        SidebarBorder,
        Card,
        CardBorder,
        Divider,
        TextPrimary,
        TextSecondary,
        TextMuted,
        TextOnAccent,
        Accent,
        AccentHover,
        AccentPressed,
        Secondary,
        SecondaryHover,
        SecondaryPressed,
        SecondaryBorder,
        Disabled,
        DisabledText,
        NavHover,
        NavText,
        EditBackground,
        EditBorder,
        LogBackground,
        LogText,
        StepDone,
        Danger,
    };

    struct UiTheme {
        bool dark = false;
        COLORREF colorWindow = RGB(246, 247, 250);
        COLORREF colorSidebar = RGB(22, 26, 34);
        COLORREF colorSidebarBorder = RGB(38, 44, 56);
        COLORREF colorCard = RGB(255, 255, 255);
        COLORREF colorCardBorder = RGB(222, 226, 234);
        COLORREF colorDivider = RGB(228, 232, 239);
        COLORREF colorTextPrimary = RGB(32, 36, 44);
        COLORREF colorTextSecondary = RGB(96, 104, 118);
        COLORREF colorTextMuted = RGB(140, 148, 162);
        COLORREF colorTextOnAccent = RGB(255, 255, 255);
        COLORREF colorAccent = RGB(41, 98, 255);
        COLORREF colorAccentHover = RGB(66, 120, 255);
        COLORREF colorAccentPressed = RGB(28, 74, 205);
        COLORREF colorSecondary = RGB(238, 241, 246);
        COLORREF colorSecondaryHover = RGB(229, 234, 242);
        COLORREF colorSecondaryPressed = RGB(214, 220, 231);
        COLORREF colorSecondaryBorder = RGB(212, 218, 228);
        COLORREF colorDisabled = RGB(236, 238, 243);
        COLORREF colorDisabledText = RGB(160, 166, 178);
        COLORREF colorNavHover = RGB(36, 43, 56);
        COLORREF colorNavText = RGB(176, 184, 198);
        COLORREF colorEditBackground = RGB(255, 255, 255);
        COLORREF colorEditBorder = RGB(214, 219, 228);
        COLORREF colorLogBackground = RGB(250, 251, 253);
        COLORREF colorLogText = RGB(58, 64, 76);
        COLORREF colorStepDone = RGB(122, 196, 140);
        COLORREF colorDanger = RGB(214, 76, 76);

        COLORREF Color(UiColorRole role) const {
            switch (role) {
            case UiColorRole::Window: return colorWindow;
            case UiColorRole::Sidebar: return colorSidebar;
            case UiColorRole::SidebarBorder: return colorSidebarBorder;
            case UiColorRole::Card: return colorCard;
            case UiColorRole::CardBorder: return colorCardBorder;
            case UiColorRole::Divider: return colorDivider;
            case UiColorRole::TextPrimary: return colorTextPrimary;
            case UiColorRole::TextSecondary: return colorTextSecondary;
            case UiColorRole::TextMuted: return colorTextMuted;
            case UiColorRole::TextOnAccent: return colorTextOnAccent;
            case UiColorRole::Accent: return colorAccent;
            case UiColorRole::AccentHover: return colorAccentHover;
            case UiColorRole::AccentPressed: return colorAccentPressed;
            case UiColorRole::Secondary: return colorSecondary;
            case UiColorRole::SecondaryHover: return colorSecondaryHover;
            case UiColorRole::SecondaryPressed: return colorSecondaryPressed;
            case UiColorRole::SecondaryBorder: return colorSecondaryBorder;
            case UiColorRole::Disabled: return colorDisabled;
            case UiColorRole::DisabledText: return colorDisabledText;
            case UiColorRole::NavHover: return colorNavHover;
            case UiColorRole::NavText: return colorNavText;
            case UiColorRole::EditBackground: return colorEditBackground;
            case UiColorRole::EditBorder: return colorEditBorder;
            case UiColorRole::LogBackground: return colorLogBackground;
            case UiColorRole::LogText: return colorLogText;
            case UiColorRole::StepDone: return colorStepDone;
            case UiColorRole::Danger: return colorDanger;
            default: return colorTextPrimary;
            }
        }
    };

    UiTheme gTheme;

    // 跟随系统深浅色设置，注册表项在 Win10 1809 之后稳定可用
    void LoadSystemTheme() {
        gTheme = UiTheme();
        DWORD appsUseLightTheme = 1;
        DWORD size = sizeof(appsUseLightTheme);
        HKEY key = nullptr;
        if (RegOpenKeyExW(HKEY_CURRENT_USER,
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS) {
            RegQueryValueExW(key, L"AppsUseLightTheme", nullptr, nullptr,
                reinterpret_cast<LPBYTE>(&appsUseLightTheme), &size);
            RegCloseKey(key);
        }
        gTheme.dark = appsUseLightTheme == 0;
        if (!gTheme.dark) {
            return;
        }

        gTheme.colorWindow = RGB(24, 26, 31);
        gTheme.colorSidebar = RGB(17, 19, 23);
        gTheme.colorSidebarBorder = RGB(38, 41, 48);
        gTheme.colorCard = RGB(32, 35, 41);
        gTheme.colorCardBorder = RGB(54, 58, 68);
        gTheme.colorDivider = RGB(48, 52, 61);
        gTheme.colorTextPrimary = RGB(236, 239, 244);
        gTheme.colorTextSecondary = RGB(168, 175, 188);
        gTheme.colorTextMuted = RGB(128, 135, 148);
        gTheme.colorAccent = RGB(64, 116, 255);
        gTheme.colorAccentHover = RGB(88, 134, 255);
        gTheme.colorAccentPressed = RGB(44, 92, 214);
        gTheme.colorSecondary = RGB(44, 48, 57);
        gTheme.colorSecondaryHover = RGB(54, 59, 70);
        gTheme.colorSecondaryPressed = RGB(36, 40, 48);
        gTheme.colorSecondaryBorder = RGB(62, 67, 79);
        gTheme.colorDisabled = RGB(38, 41, 48);
        gTheme.colorDisabledText = RGB(104, 110, 122);
        gTheme.colorNavHover = RGB(38, 42, 51);
        gTheme.colorNavText = RGB(158, 166, 180);
        gTheme.colorEditBackground = RGB(28, 31, 37);
        gTheme.colorEditBorder = RGB(58, 63, 74);
        gTheme.colorLogBackground = RGB(22, 24, 29);
        gTheme.colorLogText = RGB(196, 202, 214);
    }

    void ApplyWindowChrome(HWND window) {
        // 标题栏跟随主题，否则深色下会留一条白条
        BOOL darkTitle = gTheme.dark ? TRUE : FALSE;
        constexpr DWORD immersiveDarkMode = 20;
        constexpr DWORD windowCornerPreference = 33;
        constexpr DWORD windowBorderColor = 34;
        constexpr DWORD captionColor = 35;
        constexpr DWORD textColor = 36;
        const int corner = 2;
        DwmSetWindowAttribute(window, immersiveDarkMode, &darkTitle, sizeof(darkTitle));
        DwmSetWindowAttribute(window, windowCornerPreference, &corner, sizeof(corner));
        if (gTheme.dark) {
            DwmSetWindowAttribute(window, windowBorderColor, &gTheme.colorCardBorder, sizeof(COLORREF));
            DwmSetWindowAttribute(window, captionColor, &gTheme.colorSidebar, sizeof(COLORREF));
            DwmSetWindowAttribute(window, textColor, &gTheme.colorTextPrimary, sizeof(COLORREF));
        } else {
            const COLORREF noBorder = 0xFFFFFFFF;
            DwmSetWindowAttribute(window, windowBorderColor, &noBorder, sizeof(noBorder));
        }
    }

    int Scale(int value) {
        return MulDiv(value, gUi.dpi, 96);
    }

    int QuerySystemDpi() {
        using GetDpiForSystemFn = UINT(WINAPI*)();
        HMODULE user32 = GetModuleHandleW(L"user32.dll");
        if (user32) {
            auto getDpi = reinterpret_cast<GetDpiForSystemFn>(GetProcAddress(user32, "GetDpiForSystem"));
            if (getDpi) {
                const UINT dpi = getDpi();
                if (dpi > 0) {
                    return static_cast<int>(dpi);
                }
            }
        }
        HDC dc = GetDC(nullptr);
        const int dpi = dc ? GetDeviceCaps(dc, LOGPIXELSY) : 96;
        if (dc) {
            ReleaseDC(nullptr, dc);
        }
        return dpi > 0 ? dpi : 96;
    }

    void EnableDpiAwareness() {
        HMODULE user32 = GetModuleHandleW(L"user32.dll");
        if (!user32) {
            return;
        }
        using SetContextFn = BOOL(WINAPI*)(void*);
        auto setContext = reinterpret_cast<SetContextFn>(GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (setContext && setContext(reinterpret_cast<void*>(-4))) {
            return;
        }
        using SetAwarenessFn = BOOL(WINAPI*)();
        auto setAwareness = reinterpret_cast<SetAwarenessFn>(GetProcAddress(user32, "SetProcessDPIAware"));
        if (setAwareness) {
            setAwareness();
        }
    }

    void PostStatus(const std::wstring& status) {
        if (gUi.window) {
            PostMessageW(gUi.window, MsgStatus, 0, reinterpret_cast<LPARAM>(new std::wstring(status)));
        }
    }

    void PostLog(const std::wstring& message) {
        if (gUi.window) {
            PostMessageW(gUi.window, MsgLog, 0, reinterpret_cast<LPARAM>(new std::wstring(message)));
        }
    }

    // 0 表示不确定进度，其余值为 1 到 101 的百分比
    void PostProgress(int percent) {
        if (gUi.window) {
            PostMessageW(gUi.window, MsgProgress, static_cast<WPARAM>(percent + 1), 0);
        }
    }

    void SetStatus(const std::wstring& status) {
        if (gUi.statusText) {
            SetWindowTextW(gUi.statusText, status.c_str());
        }
    }

    void AppendUiLog(const std::wstring& message) {
        if (!gUi.logBox) {
            return;
        }
        const int length = GetWindowTextLengthW(gUi.logBox);
        SendMessageW(gUi.logBox, EM_SETSEL, length, length);
        SendMessageW(gUi.logBox, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(message.c_str()));
        SendMessageW(gUi.logBox, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(L"\r\n"));
    }

    void SetControlText(HWND hwnd, const std::wstring& text) {
        if (hwnd) {
            SetWindowTextW(hwnd, text.c_str());
        }
    }

    void ShowControl(HWND hwnd, bool show) {
        if (hwnd) {
            ShowWindow(hwnd, show ? SW_SHOW : SW_HIDE);
        }
    }

    void ApplyFont(HWND hwnd, HFONT font) {
        if (hwnd && font) {
            SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        }
    }

    void InitUiFonts();
    void RefreshThemeBrushes();
    void LayoutControls(int clientWidth, int clientHeight);

    // 用系统界面字体而非固定字体名，中文与其它语言都能正确回退
    HFONT CreateUiFont(int pointSize, int weight) {
        NONCLIENTMETRICSW metrics{};
        metrics.cbSize = sizeof(metrics);
        LOGFONTW logFont{};
        if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0)) {
            logFont = metrics.lfMessageFont;
        } else {
            logFont.lfHeight = -pointSize;
            logFont.lfWeight = weight;
            logFont.lfQuality = CLEARTYPE_QUALITY;
            wcscpy_s(logFont.lfFaceName, L"Segoe UI");
        }

        HDC dc = GetDC(nullptr);
        const int dpi = dc ? GetDeviceCaps(dc, LOGPIXELSY) : 96;
        if (dc) {
            ReleaseDC(nullptr, dc);
        }
        logFont.lfHeight = -MulDiv(pointSize, dpi, 72);
        logFont.lfWeight = weight;
        logFont.lfQuality = CLEARTYPE_QUALITY;
        return CreateFontIndirectW(&logFont);
    }

    void InitUiFonts() {
        gUi.titleFont = CreateUiFont(16, FW_SEMIBOLD);
        gUi.sectionFont = CreateUiFont(11, FW_SEMIBOLD);
        gUi.normalFont = CreateUiFont(9, FW_NORMAL);
        gUi.smallFont = CreateUiFont(8, FW_NORMAL);
        RefreshThemeBrushes();
    }

    // 主题变化时重建画刷，字体不需要动
    void RefreshThemeBrushes() {
        HBRUSH brushes[] = {
            gUi.windowBrush, gUi.sidebarBrush, gUi.dividerBrush, gUi.accentBrush,
            gUi.accentHoverBrush, gUi.accentPressedBrush, gUi.secondaryBrush,
            gUi.secondaryHoverBrush, gUi.secondaryPressedBrush, gUi.disabledBrush,
            gUi.navHoverBrush, gUi.cardBorderBrush, gUi.editBrush, gUi.logBrush
        };
        for (HBRUSH brush : brushes) {
            if (brush) {
                DeleteObject(brush);
            }
        }
        gUi.windowBrush = CreateSolidBrush(gTheme.Color(UiColorRole::Window));
        gUi.sidebarBrush = CreateSolidBrush(gTheme.Color(UiColorRole::Sidebar));
        gUi.dividerBrush = CreateSolidBrush(gTheme.Color(UiColorRole::Divider));
        gUi.accentBrush = CreateSolidBrush(gTheme.Color(UiColorRole::Accent));
        gUi.accentHoverBrush = CreateSolidBrush(gTheme.Color(UiColorRole::AccentHover));
        gUi.accentPressedBrush = CreateSolidBrush(gTheme.Color(UiColorRole::AccentPressed));
        gUi.secondaryBrush = CreateSolidBrush(gTheme.Color(UiColorRole::Secondary));
        gUi.secondaryHoverBrush = CreateSolidBrush(gTheme.Color(UiColorRole::SecondaryHover));
        gUi.secondaryPressedBrush = CreateSolidBrush(gTheme.Color(UiColorRole::SecondaryPressed));
        gUi.disabledBrush = CreateSolidBrush(gTheme.Color(UiColorRole::Disabled));
        gUi.navHoverBrush = CreateSolidBrush(gTheme.Color(UiColorRole::NavHover));
        gUi.cardBorderBrush = CreateSolidBrush(gTheme.Color(UiColorRole::CardBorder));
        gUi.editBrush = CreateSolidBrush(gTheme.Color(UiColorRole::EditBackground));
        gUi.logBrush = CreateSolidBrush(gTheme.Color(UiColorRole::LogBackground));
    }

    void DestroyUiFonts() {
        HFONT fonts[] = { gUi.titleFont, gUi.sectionFont, gUi.normalFont, gUi.smallFont };
        for (HFONT font : fonts) {
            if (font) {
                DeleteObject(font);
            }
        }
        HBRUSH brushes[] = {
            gUi.windowBrush, gUi.sidebarBrush, gUi.dividerBrush, gUi.accentBrush,
            gUi.accentHoverBrush, gUi.accentPressedBrush, gUi.secondaryBrush,
            gUi.secondaryHoverBrush, gUi.secondaryPressedBrush, gUi.disabledBrush,
            gUi.navHoverBrush, gUi.cardBorderBrush, gUi.editBrush, gUi.logBrush
        };
        for (HBRUSH brush : brushes) {
            if (brush) {
                DeleteObject(brush);
            }
        }
        gUi.titleFont = nullptr;
        gUi.sectionFont = nullptr;
        gUi.normalFont = nullptr;
        gUi.smallFont = nullptr;
        gUi.windowBrush = nullptr;
        gUi.sidebarBrush = nullptr;
        gUi.dividerBrush = nullptr;
        gUi.accentBrush = nullptr;
        gUi.accentHoverBrush = nullptr;
        gUi.accentPressedBrush = nullptr;
        gUi.secondaryBrush = nullptr;
        gUi.secondaryHoverBrush = nullptr;
        gUi.secondaryPressedBrush = nullptr;
        gUi.disabledBrush = nullptr;
        gUi.navHoverBrush = nullptr;
        gUi.cardBorderBrush = nullptr;
        gUi.editBrush = nullptr;
        gUi.logBrush = nullptr;
    }

    void FillRoundedRect(HDC dc, const RECT& rect, int radius, HBRUSH brush) {
        if (!brush) {
            return;
        }
        HGDIOBJ oldBrush = SelectObject(dc, brush);
        HGDIOBJ oldPen = SelectObject(dc, GetStockObject(NULL_PEN));
        RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
        SelectObject(dc, oldPen);
        SelectObject(dc, oldBrush);
    }

    void StrokeRoundedRect(HDC dc, const RECT& rect, int radius, COLORREF color, int thickness) {
        HPEN pen = CreatePen(PS_SOLID, thickness, color);
        HGDIOBJ oldPen = SelectObject(dc, pen);
        HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
        RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
        SelectObject(dc, oldBrush);
        SelectObject(dc, oldPen);
        DeleteObject(pen);
    }

    bool IsButtonHovered(HWND hwnd) {
        const auto found = gUi.buttonHover.find(hwnd);
        return found != gUi.buttonHover.end() && found->second;
    }

    LRESULT CALLBACK ButtonSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
        switch (message) {
        case WM_MOUSEMOVE:
            if (!IsButtonHovered(hwnd)) {
                gUi.buttonHover[hwnd] = true;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            {
                TRACKMOUSEEVENT track{};
                track.cbSize = sizeof(track);
                track.dwFlags = TME_LEAVE;
                track.hwndTrack = hwnd;
                TrackMouseEvent(&track);
            }
            break;
        case WM_MOUSELEAVE:
            gUi.buttonHover[hwnd] = false;
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_ENABLE:
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        default:
            break;
        }
        return DefSubclassProc(hwnd, message, wParam, lParam);
    }

    void DrawCommandButton(const DRAWITEMSTRUCT& item) {
        const bool primary = item.CtlID == ControlInstall || item.CtlID == ControlNext;
        const bool disabled = (item.itemState & ODS_DISABLED) != 0;
        const bool pressed = (item.itemState & ODS_SELECTED) != 0;
        const bool hovered = IsButtonHovered(item.hwndItem);
        const bool focused = (item.itemState & ODS_FOCUS) != 0;

        HBRUSH background = gUi.secondaryBrush;
        COLORREF border = gTheme.Color(UiColorRole::SecondaryBorder);
        if (primary) {
            background = pressed ? gUi.accentPressedBrush : (hovered ? gUi.accentHoverBrush : gUi.accentBrush);
            border = gTheme.Color(UiColorRole::Accent);
            if (disabled) {
                background = gUi.disabledBrush;
                border = gTheme.Color(UiColorRole::CardBorder);
            }
        } else if (disabled) {
            background = gUi.disabledBrush;
            border = gTheme.Color(UiColorRole::CardBorder);
        } else if (pressed) {
            background = gUi.secondaryPressedBrush;
        } else if (hovered) {
            background = gUi.secondaryHoverBrush;
        }

        RECT rect = item.rcItem;
        const int radius = Scale(8);
        FillRoundedRect(item.hDC, rect, radius, background);
        if (!primary || disabled) {
            StrokeRoundedRect(item.hDC, rect, radius, border, Scale(1));
        }
        if (focused && !disabled) {
            RECT focusRect = rect;
            InflateRect(&focusRect, -Scale(3), -Scale(3));
            StrokeRoundedRect(item.hDC, focusRect, Scale(6), gTheme.Color(UiColorRole::Accent), Scale(1));
        }

        COLORREF textColor = gTheme.Color(UiColorRole::TextPrimary);
        if (disabled) {
            textColor = gTheme.Color(UiColorRole::DisabledText);
        } else if (primary) {
            textColor = gTheme.Color(UiColorRole::TextOnAccent);
        }

        wchar_t text[128]{};
        GetWindowTextW(item.hwndItem, text, 128);
        SetBkMode(item.hDC, TRANSPARENT);
        SetTextColor(item.hDC, textColor);
        HGDIOBJ oldFont = SelectObject(item.hDC, gUi.normalFont);
        DrawTextW(item.hDC, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(item.hDC, oldFont);
    }

    // 勾选框自绘，系统样式在深色下会留白底
    void DrawCheckboxControl(const DRAWITEMSTRUCT& item) {
        const bool checked = SendMessageW(item.hwndItem, BM_GETCHECK, 0, 0) == BST_CHECKED;
        const bool disabled = (item.itemState & ODS_DISABLED) != 0;
        const bool hovered = IsButtonHovered(item.hwndItem);

        RECT rect = item.rcItem;
        FillRect(item.hDC, &rect, gUi.windowBrush);

        const int boxSize = Scale(16);
        RECT box{};
        box.left = rect.left + Scale(2);
        box.top = rect.top + ((rect.bottom - rect.top) - boxSize) / 2;
        box.right = box.left + boxSize;
        box.bottom = box.top + boxSize;

        const int radius = Scale(4);
        HBRUSH boxBrush = checked ? gUi.accentBrush : gUi.editBrush;
        if (disabled) {
            boxBrush = gUi.disabledBrush;
        }
        FillRoundedRect(item.hDC, box, radius, boxBrush);
        const COLORREF boxBorder = checked
            ? gTheme.Color(UiColorRole::Accent)
            : (hovered ? gTheme.Color(UiColorRole::Accent) : gTheme.Color(UiColorRole::EditBorder));
        StrokeRoundedRect(item.hDC, box, radius, boxBorder, Scale(1));

        if (checked) {
            HPEN pen = CreatePen(PS_SOLID, Scale(2), gTheme.Color(UiColorRole::TextOnAccent));
            HGDIOBJ oldPen = SelectObject(item.hDC, pen);
            const int left = box.left + Scale(4);
            const int middle = box.left + Scale(7);
            const int right = box.right - Scale(4);
            const int top = box.top + Scale(8);
            const int bottom = box.bottom - Scale(5);
            MoveToEx(item.hDC, left, top, nullptr);
            LineTo(item.hDC, middle, bottom);
            LineTo(item.hDC, right, box.top + Scale(5));
            SelectObject(item.hDC, oldPen);
            DeleteObject(pen);
        }

        wchar_t text[256]{};
        GetWindowTextW(item.hwndItem, text, 256);
        RECT textRect = rect;
        textRect.left = box.right + Scale(8);
        SetBkMode(item.hDC, TRANSPARENT);
        SetTextColor(item.hDC, disabled ? gTheme.Color(UiColorRole::DisabledText) : gTheme.Color(UiColorRole::TextPrimary));
        HGDIOBJ oldFont = SelectObject(item.hDC, gUi.normalFont);
        DrawTextW(item.hDC, text, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(item.hDC, oldFont);
    }

    // 进度条走经典绘制，否则视觉样式会忽略颜色设置
    void RefreshProgressBarTheme() {
        if (!gUi.progressBar) {
            return;
        }
        SetWindowTheme(gUi.progressBar, L"", L"");
        SendMessageW(gUi.progressBar, PBM_SETBARCOLOR, 0, static_cast<LPARAM>(gTheme.Color(UiColorRole::Accent)));
        SendMessageW(gUi.progressBar, PBM_SETBKCOLOR, 0, static_cast<LPARAM>(gTheme.Color(UiColorRole::Disabled)));
    }

    WizardStep StepFromControlId(int controlId) {
        switch (controlId) {
        case ControlStepPath:
            return WizardStep::Path;
        case ControlStepComponents:
            return WizardStep::Components;
        case ControlStepInstall:
            return WizardStep::Install;
        default:
            return WizardStep::Version;
        }
    }

    void DrawStepButton(const DRAWITEMSTRUCT& item) {
        const WizardStep step = StepFromControlId(item.CtlID);
        const bool selected = step == gUi.step;
        const bool done = static_cast<int>(step) < static_cast<int>(gUi.step);
        const bool hovered = IsButtonHovered(item.hwndItem);
        const bool disabled = (item.itemState & ODS_DISABLED) != 0;

        HBRUSH background = nullptr;
        if (selected) {
            background = gUi.accentBrush;
        } else if (hovered && !disabled) {
            background = gUi.navHoverBrush;
        }
        RECT rect = item.rcItem;
        const int radius = Scale(8);
        if (background) {
            FillRoundedRect(item.hDC, rect, radius, background);
        } else if (done) {
            RECT marker = rect;
            marker.right = marker.left + Scale(3);
            marker.top += Scale(8);
            marker.bottom -= Scale(8);
            FillRoundedRect(item.hDC, marker, Scale(1), gUi.accentBrush);
        }

        wchar_t text[64]{};
        GetWindowTextW(item.hwndItem, text, 64);
        SetBkMode(item.hDC, TRANSPARENT);
        COLORREF textColor = gTheme.Color(UiColorRole::NavText);
        if (disabled) {
            textColor = gTheme.Color(UiColorRole::DisabledText);
        } else if (selected) {
            textColor = gTheme.Color(UiColorRole::TextOnAccent);
        } else if (done) {
            textColor = gTheme.Color(UiColorRole::StepDone);
        }
        SetTextColor(item.hDC, textColor);
        HGDIOBJ oldFont = SelectObject(item.hDC, gUi.normalFont);
        RECT textRect = rect;
        textRect.left += Scale(14);
        DrawTextW(item.hDC, text, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(item.hDC, oldFont);
    }

    void SetProgressValue(int value) {
        if (gUi.marquee) {
            gUi.marquee = false;
            KillTimer(gUi.window, TimerMarquee);
        }
        if (gUi.progressBar) {
            SendMessageW(gUi.progressBar, PBM_SETPOS, static_cast<WPARAM>(value < 0 ? 0 : value), 0);
        }
    }

    void StartMarquee() {
        if (gUi.marquee) {
            return;
        }
        gUi.marquee = true;
        gUi.marqueePosition = 0.0f;
        gUi.marqueeDirection = 1.0f;
        SetTimer(gUi.window, TimerMarquee, 16, nullptr);
    }

    void StopMarquee() {
        if (gUi.window && gUi.marquee) {
            KillTimer(gUi.window, TimerMarquee);
        }
        gUi.marquee = false;
    }

    void SetBusyUi(bool busy) {        gUi.busy = busy;
        const BOOL enable = busy ? FALSE : TRUE;
        EnableWindow(gUi.refreshButton, enable);
        EnableWindow(gUi.browseButton, enable);
        EnableWindow(gUi.backButton, enable && gUi.step != WizardStep::Version);
        EnableWindow(gUi.nextButton, enable);
        EnableWindow(gUi.installButton, enable);
        EnableWindow(gUi.downloadSource, enable);
        EnableWindow(gUi.openGtamodxButton, enable);
        EnableWindow(gUi.openGithubButton, enable);
        EnableWindow(gUi.moduleIII, enable);
        EnableWindow(gUi.moduleVC, enable);
        EnableWindow(gUi.moduleSA, enable);
        EnableWindow(gUi.rootDependencies, enable);
        for (HWND stepButton : gUi.stepButtons) {
            EnableWindow(stepButton, enable);
        }

        if (busy) {
            SetControlText(gUi.cancelButton, L"取消");
            EnableWindow(gUi.cancelButton, TRUE);
            ShowControl(gUi.cancelButton, TRUE);
            if (gUi.step == WizardStep::Install) {
                SetControlText(gUi.installButton, L"安装中...");
            }
        } else {
            gUi.cancelRequested = false;
            StopMarquee();
            ShowControl(gUi.cancelButton, FALSE);
            SetControlText(gUi.installButton, L"开始安装");
        }
    }

    void SetCheckbox(HWND hwnd, bool checked) {
        if (hwnd) {
            SendMessageW(hwnd, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
        }
    }

    bool IsCheckboxChecked(HWND hwnd) {
        return hwnd && SendMessageW(hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
    }

    InstallOptions ReadOptionsFromUi() {
        InstallOptions options;
        options.installXMenuIII = IsCheckboxChecked(gUi.moduleIII);
        options.installXMenuVC = IsCheckboxChecked(gUi.moduleVC);
        options.installXMenuSA = IsCheckboxChecked(gUi.moduleSA);
        options.installRootDependencies = IsCheckboxChecked(gUi.rootDependencies);
        return options;
    }

    bool HasSelectedGameModule(const InstallOptions& options) {
        return options.installXMenuIII || options.installXMenuVC || options.installXMenuSA;
    }

    int SelectedDownloadSourceIndex() {
        if (!gUi.downloadSource) {
            return DefaultDownloadSourceIndex;
        }
        const LRESULT selected = SendMessageW(gUi.downloadSource, CB_GETCURSEL, 0, 0);
        if (selected < 0 || selected >= static_cast<LRESULT>(sizeof(DownloadSources) / sizeof(DownloadSources[0]))) {
            return DefaultDownloadSourceIndex;
        }
        return static_cast<int>(selected);
    }

    const DownloadSource& SelectedDownloadSource() {
        return DownloadSources[SelectedDownloadSourceIndex()];
    }

    std::wstring FormatReleaseNotes(const std::string& body) {
        std::wstring releaseNotes = WideFromUtf8(body);
        if (releaseNotes.empty()) {
            return L"该版本没有提供更新日志。";
        }
        std::wstring formatted;
        for (const wchar_t c : releaseNotes) {
            if (c == L'\n') {
                formatted += L"\r\n";
            } else if (c != L'\r') {
                formatted += c;
            }
        }
        return formatted;
    }

    void UpdateVersionLabels() {
        if (!gUi.isReleaseFetched) {
            SetControlText(gUi.versionSourceText, L"检测源：尚未检查");
            SetControlText(gUi.releaseVersionText, L"最新版本：—");
            SetControlText(gUi.packageVersionText, L"安装包版本：—");
            SetControlText(gUi.releaseNotesEdit, L"正在获取版本信息。\r\n优先 GTAMODX，失败时回退 GitHub。");
            return;
        }

        const std::string source = gUi.currentRelease.versionSource.empty() ? "Unknown" : gUi.currentRelease.versionSource;
        SetControlText(gUi.versionSourceText, L"检测源：" + WideFromUtf8(source));
        SetControlText(gUi.releaseVersionText, L"最新版本：" + WideFromUtf8(gUi.currentRelease.tagName));
        const std::string package = gUi.currentRelease.packageVersion.empty()
            ? gUi.currentRelease.tagName
            : gUi.currentRelease.packageVersion;
        SetControlText(gUi.packageVersionText, L"安装包版本：" + WideFromUtf8(package) + L"（GitHub Release）");
        SetControlText(gUi.releaseNotesEdit, FormatReleaseNotes(gUi.currentRelease.body));
    }

    void UpdateSummaryText() {
        std::wostringstream summary;
        summary << L"安装摘要\r\n";
        summary << L"──────────────\r\n";
        summary << L"游戏目录：" << WideFromAnsi(gUi.gameRoot.empty() ? "未选择" : gUi.gameRoot) << L"\r\n";
        summary << L"检测到游戏：" << GameTypeName(gUi.gameType) << L"\r\n";

        const std::string local = gUi.gameRoot.empty() ? "" : ReadInstalledVersion(gUi.gameRoot);
        summary << L"本地版本：" << WideFromUtf8(local.empty() ? "未安装" : local) << L"\r\n";

        if (gUi.isReleaseFetched) {
            summary << L"检测源：" << WideFromUtf8(gUi.currentRelease.versionSource) << L"\r\n";
            summary << L"最新版本：" << WideFromUtf8(gUi.currentRelease.tagName) << L"\r\n";
            summary << L"安装包：" << WideFromUtf8(gUi.currentRelease.assetName) << L"\r\n";
            summary << L"包版本：" << WideFromUtf8(
                gUi.currentRelease.packageVersion.empty() ? gUi.currentRelease.tagName : gUi.currentRelease.packageVersion
            ) << L"\r\n";
        } else {
            summary << L"版本信息：尚未获取\r\n";
        }

        const InstallOptions options = ReadOptionsFromUi();
        summary << L"\r\n组件：\r\n";
        summary << WideFromUtf8(BuildSelectedComponentSummary(options));
        summary << L"\r\n下载源：" << SelectedDownloadSource().label << L"\r\n";
        SetControlText(gUi.summaryEdit, summary.str());
    }

    void ApplyWizardStep(WizardStep step) {
        gUi.step = step;
        SendMessageW(gUi.window, WM_SETREDRAW, FALSE, 0);

        const bool isVersion = step == WizardStep::Version;
        const bool isPath = step == WizardStep::Path;
        const bool isComponents = step == WizardStep::Components;
        const bool isInstall = step == WizardStep::Install;

        ShowControl(gUi.versionSourceText, isVersion);
        ShowControl(gUi.releaseVersionText, isVersion);
        ShowControl(gUi.packageVersionText, isVersion);
        ShowControl(gUi.releaseNotesEdit, isVersion);
        ShowControl(gUi.refreshButton, isVersion);
        ShowControl(gUi.openGtamodxButton, isVersion);
        ShowControl(gUi.openGithubButton, isVersion);
        ShowControl(gUi.notesCaption, isVersion);

        ShowControl(gUi.pathEdit, isPath);
        ShowControl(gUi.browseButton, isPath);
        ShowControl(gUi.gameTypeText, isPath);
        ShowControl(gUi.localVersionText, isPath);

        ShowControl(gUi.moduleIII, isComponents);
        ShowControl(gUi.moduleVC, isComponents);
        ShowControl(gUi.moduleSA, isComponents);
        ShowControl(gUi.rootDependencies, isComponents);
        ShowControl(gUi.downloadSource, isComponents);

        ShowControl(gUi.summaryEdit, isInstall);
        ShowControl(gUi.installButton, isInstall);
        ShowControl(gUi.progressBar, isInstall);
        ShowControl(gUi.logBox, isInstall);

        for (HWND stepButton : gUi.stepButtons) {
            InvalidateRect(stepButton, nullptr, FALSE);
        }

        const wchar_t* titles[] = {
            L"检查版本",
            L"选择游戏目录",
            L"选择安装组件",
            L"确认并安装"
        };
        const wchar_t* subtitles[] = {
            L"优先 GTAMODX 检测 data.version，失败时回退 GitHub Releases",
            L"选择 GTA III / VC / SA 游戏根目录，自动识别版本",
            L"按需勾选游戏模块与根目录依赖，可切换下载代理",
            L"核对摘要后一键下载、解压并写入游戏目录"
        };
        SetControlText(gUi.headerTitle, titles[static_cast<int>(step)]);
        SetControlText(gUi.headerSubtitle, subtitles[static_cast<int>(step)]);

        if (isInstall) {
            UpdateSummaryText();
        }

        EnableWindow(gUi.backButton, !gUi.busy && step != WizardStep::Version);
        SetControlText(gUi.nextButton, step == WizardStep::Install ? L"完成" : L"下一步");

        SendMessageW(gUi.window, WM_SETREDRAW, TRUE, 0);
        RedrawWindow(gUi.window, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    }

    bool CanLeaveStep(WizardStep step) {
        if (step == WizardStep::Version) {
            if (!gUi.isReleaseFetched) {
                MessageBoxW(gUi.window, L"请先检查更新并获取版本信息。", InstallerTitle, MB_ICONWARNING);
                return false;
            }
            return true;
        }
        if (step == WizardStep::Path) {
            if (gUi.gameRoot.empty()) {
                MessageBoxW(gUi.window, L"请先选择游戏目录。", InstallerTitle, MB_ICONWARNING);
                return false;
            }
            if (gUi.gameType == GameType::Unknown) {
                MessageBoxW(gUi.window, L"未能识别为 GTA III / VC / SA 目录，请重新选择。", InstallerTitle, MB_ICONWARNING);
                return false;
            }
            return true;
        }
        if (step == WizardStep::Components) {
            if (!HasSelectedGameModule(ReadOptionsFromUi())) {
                MessageBoxW(gUi.window, L"请至少选择一个 XMenu 游戏模块。", InstallerTitle, MB_ICONWARNING);
                return false;
            }
            return true;
        }
        return true;
    }

    void GoNextStep() {
        if (gUi.step == WizardStep::Install) {
            DestroyWindow(gUi.window);
            return;
        }
        if (!CanLeaveStep(gUi.step)) {
            return;
        }
        ApplyWizardStep(static_cast<WizardStep>(static_cast<int>(gUi.step) + 1));
    }

    void GoBackStep() {
        if (gUi.step == WizardStep::Version) {
            return;
        }
        ApplyWizardStep(static_cast<WizardStep>(static_cast<int>(gUi.step) - 1));
    }

    void ApplySelectedGameRoot(const std::string& gameRoot) {
        gUi.gameRoot = gameRoot;
        gUi.gameType = DetectGameType(gameRoot);

        SetControlText(gUi.pathEdit, WideFromAnsi(gameRoot));
        SetControlText(gUi.gameTypeText, std::wstring(L"检测到游戏：") + GameTypeName(gUi.gameType));

        const std::string installedVersion = ReadInstalledVersion(gameRoot);
        SetControlText(gUi.localVersionText, L"本地版本：" + WideFromUtf8(installedVersion.empty() ? "未安装" : installedVersion));

        const InstallOptions defaults = DefaultInstallOptionsForGame(gUi.gameType);
        SetCheckbox(gUi.moduleIII, defaults.installXMenuIII);
        SetCheckbox(gUi.moduleVC, defaults.installXMenuVC);
        SetCheckbox(gUi.moduleSA, defaults.installXMenuSA);
        SetCheckbox(gUi.rootDependencies, true);
        SetProgressValue(0);
        SetStatus(L"状态：已选择游戏目录");
        AppendUiLog(L"已选择游戏目录：" + WideFromAnsi(gameRoot) + L" → " + GameTypeName(gUi.gameType));
    }

    int AskInstallerDialog(const wchar_t* title, const wchar_t* text, unsigned int flags) {
        if (!gUi.window || GetCurrentThreadId() == gUi.uiThreadId) {
            return MessageBoxW(gUi.window, text ? text : L"", title ? title : InstallerTitle, flags);
        }

        QuestionRequest request;
        request.title = title ? title : InstallerTitle;
        request.text = text ? text : L"";
        request.flags = flags;
        request.completed = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!request.completed) {
            return MessageBoxW(nullptr, request.text.c_str(), request.title.c_str(), flags);
        }
        PostMessageW(gUi.window, MsgQuestion, 0, reinterpret_cast<LPARAM>(&request));
        WaitForSingleObject(request.completed, INFINITE);
        CloseHandle(request.completed);
        return request.result;
    }

    void JoinWorker() {
        if (gUi.worker.joinable()) {
            gUi.worker.join();
        }
    }

    void PostFetchDone(FetchResult* result) {
        if (gUi.window) {
            PostMessageW(gUi.window, MsgFetchDone, 0, reinterpret_cast<LPARAM>(result));
        }
    }

    void PostInstallDone(InstallResult* result) {
        if (gUi.window) {
            PostMessageW(gUi.window, MsgInstallDone, 0, reinterpret_cast<LPARAM>(result));
        }
    }

    bool OnAssetDownloadProgress(ULONG current, ULONG total) {
        if (gUi.cancelRequested.load()) {
            return false;
        }
        if (total > 0) {
            const int percent = static_cast<int>((static_cast<unsigned long long>(current) * 100ull) / total);
            if (gUi.lastPostedPercent.exchange(percent) != percent) {
                PostProgress(percent);
                PostStatus(L"状态：下载安装包 " + std::to_wstring(percent) + L"%");
            }
        }
        return true;
    }

    void StartFetchWorker() {
        if (gUi.busy.exchange(true)) {
            return;
        }
        gUi.cancelRequested = false;
        SetBusyUi(true);
        SetControlText(gUi.refreshButton, L"检查中...");
        SetControlText(gUi.versionSourceText, L"检测源：检查更新中（优先 GTAMODX）...");
        SetStatus(L"状态：检查更新中（优先 GTAMODX）...");
        AppendUiLog(L"开始检查更新：优先 GTAMODX，其次 GitHub。");

        gUi.worker = std::thread([] {
            FetchResult* result = new FetchResult();
            std::string gtamodxVersion;

            const std::string gtamodxPath = TempPathFor("gtamodx.json");
            if (DownloadFile(XMENU_GTAMODX_API, gtamodxPath)) {
                gtamodxVersion = ExtractGtamodxVersion(ReadTextFile(gtamodxPath));
                if (gtamodxVersion.empty()) {
                    PostLog(L"GTAMODX 响应缺少 data.version，将回退 GitHub。");
                } else {
                    PostLog(L"GTAMODX 版本：" + WideFromUtf8(gtamodxVersion));
                }
            } else {
                PostLog(L"GTAMODX API 请求失败，将回退 GitHub。");
            }

            if (gUi.cancelRequested.load()) {
                result->cancelled = true;
                result->error = L"已取消检查更新。";
                PostFetchDone(result);
                return;
            }

            PostStatus(L"状态：获取 GitHub 安装包信息...");
            const std::string githubPath = TempPathFor("release.json");
            if (!DownloadFile(XMENU_GITHUB_API, githubPath)) {
                result->error = gtamodxVersion.empty()
                    ? L"GTAMODX 与 GitHub 均不可用，无法检查更新。"
                    : L"已从 GTAMODX 获取版本，但 GitHub 安装包信息获取失败。\n安装需要 GitHub Release 资源。";
                PostFetchDone(result);
                return;
            }

            ReleaseInfo release = ParseReleaseInfo(ReadTextFile(githubPath));
            if (release.assetUrl.empty()) {
                result->error = L"GitHub release 中未找到可安装的 zip 包。请确认已上传 XMenuIII.VC.SA.zip。";
                PostFetchDone(result);
                return;
            }

            const std::string githubTag = release.packageVersion;
            if (!gtamodxVersion.empty()) {
                release.tagName = gtamodxVersion;
                release.packageVersion = githubTag.empty() ? release.tagName : githubTag;
                release.versionSource = "GTAMODX";
            } else if (!githubTag.empty()) {
                release.tagName = githubTag;
                release.packageVersion = githubTag;
                release.versionSource = "GitHub";
            } else {
                result->error = L"未能解析版本号。";
                PostFetchDone(result);
                return;
            }

            result->ok = true;
            result->release = release;
            PostFetchDone(result);
        });
    }

    void StartInstallWorker() {
        if (!gUi.isReleaseFetched) {
            MessageBoxW(gUi.window, L"请先检查更新。", InstallerTitle, MB_ICONWARNING);
            ApplyWizardStep(WizardStep::Version);
            return;
        }
        if (gUi.gameRoot.empty()) {
            MessageBoxW(gUi.window, L"请先选择游戏目录。", InstallerTitle, MB_ICONWARNING);
            ApplyWizardStep(WizardStep::Path);
            return;
        }

        const InstallOptions options = ReadOptionsFromUi();
        if (!HasSelectedGameModule(options)) {
            MessageBoxW(gUi.window, L"请至少选择一个 XMenu 游戏模块。", InstallerTitle, MB_ICONWARNING);
            ApplyWizardStep(WizardStep::Components);
            return;
        }

        const std::string installedVersion = ReadInstalledVersion(gUi.gameRoot);
        const ReleaseInfo release = gUi.currentRelease;
        const int downloadSourceIndex = SelectedDownloadSourceIndex();
        const DownloadSource downloadSource = SelectedDownloadSource();
        const std::string downloadUrl = BuildDownloadUrl(release.assetUrl, downloadSourceIndex);
        const std::string downloadSourceName = AnsiFromWide(downloadSource.label);
        const std::string installVersion = release.packageVersion.empty() ? release.tagName : release.packageVersion;
        const std::string zipName = release.assetName.empty() ? "release.zip" : release.assetName;
        const std::string zipPath = TempPathFor(zipName);

        std::wstring confirm;
        confirm += installedVersion.empty() ? L"将执行安装：\n\n" : L"将执行更新：\n\n";
        confirm += L"游戏目录：" + WideFromAnsi(gUi.gameRoot) + L"\n";
        confirm += L"本地版本：" + WideFromUtf8(installedVersion.empty() ? "未安装" : installedVersion) + L"\n";
        confirm += L"检测源：" + WideFromUtf8(release.versionSource) + L"\n";
        confirm += L"最新版本：" + WideFromUtf8(release.tagName) + L"\n";
        confirm += L"安装包版本：" + WideFromUtf8(installVersion) + L"\n";
        confirm += L"Release 包：" + WideFromUtf8(release.assetName) + L"\n";
        confirm += L"下载源：" + WideFromUtf8(downloadSourceName) + L"\n";
        confirm += L"安装前将自动校验现有文件完整性。\n\n";
        confirm += L"选择的组件：\n" + WideFromUtf8(BuildSelectedComponentSummary(options)) + L"\n继续安装？";
        if (MessageBoxW(gUi.window, confirm.c_str(), InstallerTitle, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1) != IDYES) {
            AppendUiLog(L"用户取消安装。");
            AppendInstallLog(gUi.gameRoot, "User cancelled install after release check");
            return;
        }

        if (gUi.busy.exchange(true)) {
            return;
        }
        gUi.cancelRequested = false;
        gUi.lastPostedPercent = -1;
        SetBusyUi(true);
        SetProgressValue(0);
        SetStatus(L"状态：准备下载安装包...");
        AppendUiLog(L"开始下载安装包，下载源：" + WideFromUtf8(downloadSourceName));

        gUi.worker = std::thread([options, release, downloadSourceName, downloadUrl, installVersion,
                                  zipPath, attachedRoot = gUi.gameRoot] {
            InstallResult* result = new InstallResult();
            const DWORD startedTick = GetTickCount();
            const std::string startedAt = CurrentTimestamp();

            AppendInstallLog(attachedRoot, "===== XMenu installer started at " + startedAt + " =====");
            AppendInstallLog(attachedRoot, "Game root: " + attachedRoot);
            AppendInstallLog(attachedRoot, "Selected components:\r\n" + BuildSelectedComponentSummary(options));
            AppendInstallLog(attachedRoot, "Version source: " + release.versionSource);
            AppendInstallLog(attachedRoot, "Release version: " + release.tagName);
            AppendInstallLog(attachedRoot, "Package version: " + installVersion);
            AppendInstallLog(attachedRoot, "Selected asset: " + release.assetName);
            AppendInstallLog(attachedRoot, "Asset official URL: " + release.assetUrl);
            AppendInstallLog(attachedRoot, "Download source: " + downloadSourceName);
            AppendInstallLog(attachedRoot, "Download URL: " + downloadUrl);
            AppendInstallLog(attachedRoot, "Downloading asset to: " + zipPath);

            std::string installedVersion;
            std::string integrityReport;
            installedVersion = ReadInstalledVersion(attachedRoot);
            VerifyInstalledFiles(attachedRoot, integrityReport);
            AppendInstallLog(attachedRoot, "Installed version: " + (installedVersion.empty() ? std::string("none") : installedVersion));
            AppendInstallLog(attachedRoot, "Integrity before install: " + integrityReport);
            PostLog(L"检测源：" + WideFromUtf8(release.versionSource) + L"，版本：" + WideFromUtf8(release.tagName));
            PostLog(L"安装前完整性：" + WideFromUtf8(integrityReport));

            try {
                DownloadFileOrThrow(downloadUrl, zipPath, downloadSourceName, &OnAssetDownloadProgress);
            } catch (const std::exception& error) {
                const bool cancelled = gUi.cancelRequested.load();
                result->cancelled = cancelled;
                result->error = cancelled
                    ? L"安装已取消。"
                    : L"下载安装包失败：" + WideFromUtf8(error.what()) + L"\n可以在「选择组件」步骤切换代理后重试。";
                AppendInstallLog(attachedRoot, std::string("ERROR asset download failed: ") + error.what());
                PostInstallDone(result);
                return;
            }

            if (gUi.cancelRequested.load()) {
                result->cancelled = true;
                result->error = L"安装已取消。";
                PostInstallDone(result);
                return;
            }

            PostProgress(-1);
            PostStatus(L"状态：解压安装包...");
            PostLog(L"开始解压安装包...");
            const std::string extractDir = TempPathFor("extract");
            AppendInstallLog(attachedRoot, "Extracting asset to: " + extractDir);
            RunHiddenAndWait("cmd.exe /c rmdir /s /q \"" + extractDir + "\"");
            if (!ExtractZip(zipPath, extractDir)) {
                result->error = L"解压安装包失败。";
                AppendInstallLog(attachedRoot, "ERROR asset extraction failed");
                PostInstallDone(result);
                return;
            }

            if (gUi.cancelRequested.load()) {
                result->cancelled = true;
                result->error = L"安装已取消。";
                PostInstallDone(result);
                return;
            }

            PostProgress(-1);
            PostStatus(L"状态：写入游戏目录...");
            PostLog(L"开始写入游戏目录...");
            if (!InstallRelease(extractDir, attachedRoot, installVersion, options)) {
                result->error = L"安装失败，请查看 plugins\\XMenu\\install.log。";
                AppendInstallLog(attachedRoot, "ERROR install failed");
                PostInstallDone(result);
                return;
            }

            std::string finalReport;
            VerifyInstalledFiles(attachedRoot, finalReport);
            const std::string finishedAt = CurrentTimestamp();
            const std::string duration = FormatDuration(startedTick, GetTickCount());
            AppendInstallLog(attachedRoot, "Integrity after install: " + finalReport);
            AppendInstallLog(attachedRoot, "Installer finished at " + finishedAt + ", duration " + duration);

            result->ok = true;
            result->version = WideFromUtf8(installVersion);
            result->summary = L"XMenu 安装/更新完成。\n\n检测源：" + WideFromUtf8(release.versionSource)
                + L"\n版本：" + WideFromUtf8(installVersion)
                + L"\n包：" + WideFromUtf8(release.assetName)
                + L"\n开始时间：" + WideFromUtf8(startedAt)
                + L"\n结束时间：" + WideFromUtf8(finishedAt)
                + L"\n耗时：" + WideFromUtf8(duration)
                + L"\n" + WideFromUtf8(finalReport);
            PostInstallDone(result);
        });
    }

    HWND CreateLabel(HWND parent, const wchar_t* text, int x, int y, int w, int h, HFONT font) {
        HWND hwnd = CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE, Scale(x), Scale(y), Scale(w), Scale(h), parent, nullptr, gUi.instance, nullptr);
        ApplyFont(hwnd, font);
        return hwnd;
    }

    HWND CreateButton(HWND parent, const wchar_t* text, int id, int x, int y, int w, int h) {
        HWND hwnd = CreateWindowExW(
            0, L"BUTTON", text,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            Scale(x), Scale(y), Scale(w), Scale(h),
            parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), gUi.instance, nullptr
        );
        ApplyFont(hwnd, gUi.normalFont);
        SetWindowSubclass(hwnd, ButtonSubclassProc, 1, 0);
        return hwnd;
    }

    HWND CreateStepButton(HWND parent, const wchar_t* text, int id, int x, int y, int w, int h) {
        HWND hwnd = CreateWindowExW(
            0, L"BUTTON", text,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_LEFT,
            Scale(x), Scale(y), Scale(w), Scale(h),
            parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), gUi.instance, nullptr
        );
        ApplyFont(hwnd, gUi.normalFont);
        SetWindowSubclass(hwnd, ButtonSubclassProc, 1, 0);
        return hwnd;
    }

    HWND CreateCheckbox(HWND parent, const wchar_t* text, int id, int x, int y, int w, int h) {
        HWND hwnd = CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX | BS_OWNERDRAW, Scale(x), Scale(y), Scale(w), Scale(h), parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), gUi.instance, nullptr);
        ApplyFont(hwnd, gUi.normalFont);
        SetWindowSubclass(hwnd, ButtonSubclassProc, 1, 0);
        return hwnd;
    }

    HWND CreateReadonlyEdit(HWND parent, const wchar_t* text, int x, int y, int w, int h) {
        HWND hwnd = CreateWindowExW(0, L"EDIT", text, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY, Scale(x), Scale(y), Scale(w), Scale(h), parent, nullptr, gUi.instance, nullptr);
        ApplyFont(hwnd, gUi.normalFont);
        return hwnd;
    }

    HWND CreateMultilineReadonlyEdit(HWND parent, const wchar_t* text, int x, int y, int w, int h, HFONT font) {
        HWND hwnd = CreateWindowExW(
            0, L"EDIT", text,
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL,
            Scale(x), Scale(y), Scale(w), Scale(h),
            parent, nullptr, gUi.instance, nullptr
        );
        ApplyFont(hwnd, font);
        return hwnd;
    }

    HWND CreateDownloadSourceCombo(HWND parent, int x, int y, int w, int h) {
        HWND hwnd = CreateWindowExW(
            0, L"COMBOBOX", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
            Scale(x), Scale(y), Scale(w), Scale(h),
            parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ControlDownloadSource)), gUi.instance, nullptr
        );
        ApplyFont(hwnd, gUi.normalFont);
        SetWindowTheme(hwnd, gTheme.dark ? L"DarkMode_CFD" : nullptr, nullptr);
        for (const DownloadSource& source : DownloadSources) {
            SendMessageW(hwnd, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(source.label));
        }
        SendMessageW(hwnd, CB_SETCURSEL, DefaultDownloadSourceIndex, 0);
        return hwnd;
    }

    void CreateInstallerControls(HWND window) {
        const int stepIds[] = { ControlStepVersion, ControlStepPath, ControlStepComponents, ControlStepInstall };
        for (int i = 0; i < 4; ++i) {
            gUi.stepButtons[i] = CreateStepButton(window, StepLabels[i], stepIds[i], 16, 92 + i * 48, SidebarWidth - 32, 38);
        }

        gUi.brandTitle = CreateLabel(window, L"XMenu", 20, 24, 140, 30, gUi.titleFont);
        gUi.brandSubtitle = CreateLabel(window, L"Installer", 20, 54, 140, 20, gUi.smallFont);

        gUi.headerTitle = CreateLabel(window, L"检查版本", ContentLeft, 20, 500, 30, gUi.titleFont);
        gUi.headerSubtitle = CreateLabel(window, L"优先 GTAMODX，失败回退 GitHub", ContentLeft, 52, 560, 20, gUi.smallFont);

        // --- 版本页 ---
        gUi.versionSourceText = CreateLabel(window, L"检测源：尚未检查", ContentLeft, 96, 420, 22, gUi.normalFont);
        gUi.releaseVersionText = CreateLabel(window, L"最新版本：—", ContentLeft, 122, 420, 22, gUi.sectionFont);
        gUi.packageVersionText = CreateLabel(window, L"安装包版本：—", ContentLeft, 148, 520, 22, gUi.normalFont);
        gUi.refreshButton = CreateButton(window, L"检查更新", ControlFetchRelease, ContentLeft + 470, 112, 100, 32);
        gUi.openGtamodxButton = CreateButton(window, L"打开 GTAMODX", ControlOpenGtamodx, ContentLeft, 176, 120, 30);
        gUi.openGithubButton = CreateButton(window, L"打开 GitHub", ControlOpenGithub, ContentLeft + 132, 176, 110, 30);
        gUi.notesCaption = CreateLabel(window, L"更新日志（来自 GitHub Release）", ContentLeft, 218, 300, 20, gUi.smallFont);
        gUi.releaseNotesEdit = CreateMultilineReadonlyEdit(
            window,
            L"正在获取版本信息。\r\n优先使用 GTAMODX API 的 data.version。",
            ContentLeft, 242, 570, 250, gUi.normalFont
        );

        // --- 目录页 ---
        gUi.pathEdit = CreateReadonlyEdit(window, L"尚未选择游戏目录", ContentLeft, 124, 450, 30);
        gUi.browseButton = CreateButton(window, L"浏览...", ControlBrowse, ContentLeft + 460, 123, 100, 32);
        gUi.gameTypeText = CreateLabel(window, L"检测到游戏：未选择", ContentLeft, 170, 400, 22, gUi.normalFont);
        gUi.localVersionText = CreateLabel(window, L"本地版本：未检测", ContentLeft, 198, 400, 22, gUi.normalFont);

        // --- 组件页 ---
        gUi.moduleIII = CreateCheckbox(window, L"安装 GTA III 模块（XMenuIII.asi）", ControlModuleIII, ContentLeft, 110, 360, 26);
        gUi.moduleVC = CreateCheckbox(window, L"安装 GTA Vice City 模块（XMenuVC.asi）", ControlModuleVC, ContentLeft, 146, 380, 26);
        gUi.moduleSA = CreateCheckbox(window, L"安装 GTA San Andreas 模块（XMenuSA.asi）", ControlModuleSA, ContentLeft, 182, 400, 26);
        gUi.rootDependencies = CreateCheckbox(window, L"安装 Ultimate ASI Loader / D3D8to9 到游戏根目录", ControlRootDependencies, ContentLeft, 230, 420, 26);
        gUi.downloadSource = CreateDownloadSourceCombo(window, ContentLeft, 306, 360, 200);

        // --- 安装页 ---
        gUi.summaryEdit = CreateMultilineReadonlyEdit(window, L"", ContentLeft, 96, 570, 200, gUi.normalFont);
        gUi.progressBar = CreateWindowExW(0, PROGRESS_CLASSW, L"", WS_CHILD | WS_VISIBLE | PBS_SMOOTH, Scale(ContentLeft), Scale(340), Scale(450), Scale(18), window, nullptr, gUi.instance, nullptr);
        SendMessageW(gUi.progressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendMessageW(gUi.progressBar, PBM_SETPOS, 0, 0);
        RefreshProgressBarTheme();
        gUi.installButton = CreateButton(window, L"开始安装", ControlInstall, ContentLeft + 470, 308, 100, 48);
        gUi.logBox = CreateMultilineReadonlyEdit(
            window,
            L"安装器已启动。\r\n",
            ContentLeft, 372, 570, 120, gUi.smallFont
        );

        // 底部导航
        gUi.statusText = CreateLabel(window, L"状态：就绪", ContentLeft, 502, 570, 24, gUi.normalFont);
        gUi.backButton = CreateButton(window, L"上一步", ControlBack, ContentLeft, 540, 100, 32);
        gUi.cancelButton = CreateButton(window, L"取消", ControlCancel, ContentLeft + 356, 540, 100, 32);
        gUi.nextButton = CreateButton(window, L"下一步", ControlNext, ContentLeft + 470, 540, 100, 32);
        ShowControl(gUi.cancelButton, false);

        // 所有坐标统一由布局函数设置，创建时的位置只作为占位
        RECT client{};
        GetClientRect(window, &client);
        LayoutControls(client.right, client.bottom);

        ApplyWizardStep(WizardStep::Version);
        UpdateVersionLabels();
    }

    void MoveControl(HWND hwnd, int x, int y, int width, int height) {
        if (!hwnd) {
            return;
        }
        SetWindowPos(hwnd, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // 统一布局，左栏固定宽度，内容区左右留白一致，底部按钮右对齐
    void LayoutControls(int clientWidth, int clientHeight) {
        if (clientWidth <= 0 || clientHeight <= 0) {
            return;
        }
        const int contentLeft = Scale(SidebarWidth + ContentPadding);
        int contentRight = clientWidth - Scale(ContentPadding);
        if (contentRight < contentLeft + Scale(360)) {
            contentRight = contentLeft + Scale(360);
        }
        const int contentWidth = contentRight - contentLeft;
        const int bodyTop = Scale(BodyTop);
        const int footerTop = clientHeight - Scale(FooterHeight);

        // 侧边栏
        MoveControl(gUi.brandTitle, Scale(SidebarPadding), Scale(30),
            Scale(SidebarWidth - SidebarPadding * 2), Scale(28));
        MoveControl(gUi.brandSubtitle, Scale(SidebarPadding), Scale(58),
            Scale(SidebarWidth - SidebarPadding * 2), Scale(20));
        const int stepHeight = Scale(ButtonHeight);
        for (int i = 0; i < 4; ++i) {
            MoveControl(gUi.stepButtons[i], Scale(SidebarPadding - 6),
                Scale(120) + i * (stepHeight + Scale(RowGap)),
                Scale(SidebarWidth - (SidebarPadding - 6) * 2), stepHeight);
        }

        // 页头与页脚
        MoveControl(gUi.headerTitle, contentLeft, Scale(30), contentWidth, Scale(30));
        MoveControl(gUi.headerSubtitle, contentLeft, Scale(68), contentWidth, Scale(22));
        const int footerButtonY = footerTop + (Scale(FooterHeight) - Scale(ButtonHeight)) / 2;
        const int footerButtonWidth = Scale(FooterButtonWidth);
        MoveControl(gUi.nextButton, contentRight - footerButtonWidth, footerButtonY,
            footerButtonWidth, Scale(ButtonHeight));
        MoveControl(gUi.backButton, contentRight - footerButtonWidth * 2 - Scale(RowGap), footerButtonY,
            footerButtonWidth, Scale(ButtonHeight));
        MoveControl(gUi.cancelButton, contentRight - footerButtonWidth * 3 - Scale(RowGap) * 2, footerButtonY,
            footerButtonWidth, Scale(ButtonHeight));
        const int statusWidth = contentRight - contentLeft - footerButtonWidth * 3 - Scale(RowGap) * 3;
        MoveControl(gUi.statusText, contentLeft, footerButtonY + Scale(6),
            statusWidth > Scale(160) ? statusWidth : Scale(160), Scale(22));

        const int rowGap = Scale(RowGap);
        const int rowHeight = Scale(RowHeight);
        const int fieldHeight = Scale(FieldHeight);
        const int buttonHeight = Scale(ButtonHeight);

        // 版本页
        MoveControl(gUi.versionSourceText, contentLeft, bodyTop, contentWidth, rowHeight);
        MoveControl(gUi.releaseVersionText, contentLeft, bodyTop + Scale(30), contentWidth, Scale(26));
        MoveControl(gUi.packageVersionText, contentLeft, bodyTop + Scale(60), contentWidth, rowHeight);
        const int versionButtonY = bodyTop + Scale(94);
        const int versionButtonWidth = Scale(136);
        MoveControl(gUi.refreshButton, contentLeft, versionButtonY, versionButtonWidth, buttonHeight);
        MoveControl(gUi.openGtamodxButton, contentLeft + versionButtonWidth + rowGap, versionButtonY,
            Scale(148), buttonHeight);
        MoveControl(gUi.openGithubButton, contentLeft + versionButtonWidth * 2 + rowGap * 2, versionButtonY,
            versionButtonWidth, buttonHeight);
        const int notesCaptionY = versionButtonY + buttonHeight + Scale(18);
        MoveControl(gUi.notesCaption, contentLeft, notesCaptionY, contentWidth, Scale(20));
        const int notesTop = notesCaptionY + Scale(24);
        int notesHeight = footerTop - Scale(20) - notesTop;
        if (notesHeight < Scale(120)) {
            notesHeight = Scale(120);
        }
        MoveControl(gUi.releaseNotesEdit, contentLeft, notesTop, contentWidth, notesHeight);

        // 目录页
        MoveControl(gUi.gameTypeText, contentLeft, bodyTop, contentWidth, rowHeight);
        MoveControl(gUi.localVersionText, contentLeft, bodyTop + Scale(28), contentWidth, rowHeight);
        const int pathRowY = bodyTop + Scale(66);
        const int browseWidth = Scale(108);
        MoveControl(gUi.pathEdit, contentLeft, pathRowY, contentWidth - browseWidth - rowGap, fieldHeight);
        MoveControl(gUi.browseButton, contentLeft + contentWidth - browseWidth, pathRowY, browseWidth, fieldHeight);

        // 组件页
        MoveControl(gUi.moduleIII, contentLeft, bodyTop, contentWidth, Scale(26));
        MoveControl(gUi.moduleVC, contentLeft, bodyTop + Scale(32), contentWidth, Scale(26));
        MoveControl(gUi.moduleSA, contentLeft, bodyTop + Scale(64), contentWidth, Scale(26));
        MoveControl(gUi.rootDependencies, contentLeft, bodyTop + Scale(104), contentWidth, Scale(26));
        MoveControl(gUi.downloadSource, contentLeft, bodyTop + Scale(164), Scale(280), Scale(200));

        // 安装页
        MoveControl(gUi.summaryEdit, contentLeft, bodyTop, contentWidth, Scale(132));
        MoveControl(gUi.progressBar, contentLeft, bodyTop + Scale(146), contentWidth, Scale(10));
        MoveControl(gUi.installButton, contentLeft, bodyTop + Scale(198), Scale(136), Scale(38));
        const int logTop = bodyTop + Scale(252);
        int logHeight = footerTop - Scale(20) - logTop;
        if (logHeight < Scale(120)) {
            logHeight = Scale(120);
        }
        MoveControl(gUi.logBox, contentLeft, logTop, contentWidth, logHeight);
    }

    void DestroyUiBuffers() {
        if (gUi.bufferBitmap) {
            DeleteObject(gUi.bufferBitmap);
            gUi.bufferBitmap = nullptr;
        }
        if (gUi.bufferDc) {
            DeleteDC(gUi.bufferDc);
            gUi.bufferDc = nullptr;
        }
        gUi.bufferWidth = 0;
        gUi.bufferHeight = 0;
    }

    void PaintWindowBackground(HWND window, HDC target) {
        RECT rect{};
        GetClientRect(window, &rect);
        const int width = rect.right;
        const int height = rect.bottom;
        if (width <= 0 || height <= 0) {
            return;
        }

        if (!gUi.bufferDc || gUi.bufferWidth < width || gUi.bufferHeight < height) {
            DestroyUiBuffers();
            gUi.bufferDc = CreateCompatibleDC(target);
            gUi.bufferBitmap = CreateCompatibleBitmap(target, width, height);
            SelectObject(gUi.bufferDc, gUi.bufferBitmap);
            gUi.bufferWidth = width;
            gUi.bufferHeight = height;
        }

        RECT content = rect;
        FillRect(gUi.bufferDc, &content, gUi.windowBrush);

        RECT sidebar = rect;
        sidebar.right = Scale(SidebarWidth);
        FillRect(gUi.bufferDc, &sidebar, gUi.sidebarBrush);

        RECT sidebarBorder = { sidebar.right, 0, sidebar.right + Scale(1), height };
        FillRect(gUi.bufferDc, &sidebarBorder, gUi.dividerBrush);

        // 页头与页脚的分隔线在背景层统一绘制，保证与控件同一条基线
        const int headLine = Scale(HeaderLine);
        RECT headerDivider = { sidebar.right, headLine, width, headLine + Scale(1) };
        FillRect(gUi.bufferDc, &headerDivider, gUi.dividerBrush);

        const int footLine = height - Scale(FooterHeight);
        RECT footerDivider = { sidebar.right, footLine, width, footLine + Scale(1) };
        FillRect(gUi.bufferDc, &footerDivider, gUi.dividerBrush);

        BitBlt(target, 0, 0, width, height, gUi.bufferDc, 0, 0, SRCCOPY);
    }

    LRESULT CALLBACK InstallerWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
        case WM_CREATE:
            gUi.window = window;
            gUi.uiThreadId = GetCurrentThreadId();
            gUi.dpi = QuerySystemDpi();
            LoadSystemTheme();
            ApplyWindowChrome(window);
            InitUiFonts();
            CreateInstallerControls(window);
            PostMessageW(window, WM_COMMAND, MAKEWPARAM(ControlFetchRelease, BN_CLICKED), 0);
            return 0;

        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED) {
                LayoutControls(LOWORD(lParam), HIWORD(lParam));
                InvalidateRect(window, nullptr, FALSE);
            }
            return 0;

        case WM_GETMINMAXINFO: {
            auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
            if (info) {
                info->ptMinTrackSize.x = Scale(MinWindowWidth);
                info->ptMinTrackSize.y = Scale(MinWindowHeight);
            }
            return 0;
        }

        case WM_SETTINGCHANGE:
            if (lParam && wcscmp(reinterpret_cast<const wchar_t*>(lParam), L"ImmersiveColorSet") == 0) {
                LoadSystemTheme();
                RefreshThemeBrushes();
                RefreshProgressBarTheme();
                ApplyWindowChrome(window);
                InvalidateRect(window, nullptr, TRUE);
                RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
            }
            return 0;

        case WM_THEMECHANGED:
            LoadSystemTheme();
            RefreshThemeBrushes();
            RefreshProgressBarTheme();
            ApplyWindowChrome(window);
            InvalidateRect(window, nullptr, TRUE);
            RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
            case ControlBrowse: {
                const std::string gameRoot = PickGameRoot();
                if (!gameRoot.empty()) {
                    ApplySelectedGameRoot(gameRoot);
                }
                return 0;
            }
            case ControlFetchRelease:
                if (!gUi.busy) {
                    StartFetchWorker();
                }
                return 0;
            case ControlInstall:
                if (!gUi.busy) {
                    StartInstallWorker();
                }
                return 0;
            case ControlCancel:
                if (gUi.busy && !gUi.cancelRequested.exchange(true)) {
                    EnableWindow(gUi.cancelButton, FALSE);
                    SetControlText(gUi.cancelButton, L"正在取消...");
                    SetStatus(L"状态：正在取消...");
                    AppendUiLog(L"已请求取消，等待当前步骤结束。");
                }
                return 0;
            case ControlBack:
                if (!gUi.busy) {
                    GoBackStep();
                }
                return 0;
            case ControlNext:
                if (!gUi.busy) {
                    GoNextStep();
                }
                return 0;
            case ControlOpenGtamodx:
                ShellExecuteA(nullptr, "open", XMENU_URL, nullptr, nullptr, SW_SHOWNORMAL);
                return 0;
            case ControlOpenGithub:
                ShellExecuteA(nullptr, "open", XMENU_GITHUB, nullptr, nullptr, SW_SHOWNORMAL);
                return 0;
            case ControlStepVersion:
                if (!gUi.busy) {
                    ApplyWizardStep(WizardStep::Version);
                }
                return 0;
            case ControlStepPath:
                if (!gUi.busy && CanLeaveStep(WizardStep::Version)) {
                    ApplyWizardStep(WizardStep::Path);
                }
                return 0;
            case ControlStepComponents:
                if (!gUi.busy && CanLeaveStep(WizardStep::Version) && CanLeaveStep(WizardStep::Path)) {
                    ApplyWizardStep(WizardStep::Components);
                }
                return 0;
            case ControlStepInstall:
                if (!gUi.busy && CanLeaveStep(WizardStep::Version) && CanLeaveStep(WizardStep::Path) && CanLeaveStep(WizardStep::Components)) {
                    ApplyWizardStep(WizardStep::Install);
                }
                return 0;
            case ControlDownloadSource:
                if (HIWORD(wParam) == CBN_SELCHANGE && gUi.step == WizardStep::Install) {
                    UpdateSummaryText();
                }
                return 0;
            case ControlModuleIII:
            case ControlModuleVC:
            case ControlModuleSA:
            case ControlRootDependencies: {
                // 自绘勾选框不再自动切换状态，点击时手动翻转
                HWND control = reinterpret_cast<HWND>(lParam);
                if (control && HIWORD(wParam) == BN_CLICKED) {
                    SetCheckbox(control, !IsCheckboxChecked(control));
                    InvalidateRect(control, nullptr, FALSE);
                }
                return 0;
            }
            default:
                return 0;
            }

        case WM_DRAWITEM: {
            const DRAWITEMSTRUCT* item = reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);
            if (!item) {
                return FALSE;
            }
            if (item->CtlID >= ControlStepVersion && item->CtlID <= ControlStepInstall) {
                DrawStepButton(*item);
                return TRUE;
            }
            if (item->CtlID == ControlModuleIII || item->CtlID == ControlModuleVC
                || item->CtlID == ControlModuleSA || item->CtlID == ControlRootDependencies) {
                DrawCheckboxControl(*item);
                return TRUE;
            }
            DrawCommandButton(*item);
            return TRUE;
        }

        case WM_CTLCOLORSTATIC: {
            const HWND control = reinterpret_cast<HWND>(lParam);
            HDC hdc = reinterpret_cast<HDC>(wParam);
            if (control == gUi.logBox) {
                SetBkMode(hdc, OPAQUE);
                SetTextColor(hdc, gTheme.Color(UiColorRole::LogText));
                SetBkColor(hdc, gTheme.Color(UiColorRole::LogBackground));
                return reinterpret_cast<LRESULT>(gUi.logBrush);
            }
            if (control == gUi.pathEdit || control == gUi.releaseNotesEdit || control == gUi.summaryEdit) {
                SetBkMode(hdc, OPAQUE);
                SetTextColor(hdc, gTheme.Color(UiColorRole::TextPrimary));
                SetBkColor(hdc, gTheme.Color(UiColorRole::EditBackground));
                return reinterpret_cast<LRESULT>(gUi.editBrush);
            }
            if (control == gUi.brandTitle || control == gUi.brandSubtitle) {
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, gTheme.Color(UiColorRole::TextOnAccent));
                return reinterpret_cast<LRESULT>(gUi.sidebarBrush);
            }
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, gTheme.Color(UiColorRole::TextSecondary));
            return reinterpret_cast<LRESULT>(gUi.windowBrush);
        }

        case WM_CTLCOLOREDIT: {
            SetBkMode(reinterpret_cast<HDC>(wParam), OPAQUE);
            SetTextColor(reinterpret_cast<HDC>(wParam), gTheme.Color(UiColorRole::TextPrimary));
            SetBkColor(reinterpret_cast<HDC>(wParam), gTheme.Color(UiColorRole::EditBackground));
            return reinterpret_cast<LRESULT>(gUi.editBrush);
        }

        case WM_CTLCOLORLISTBOX: {
            SetBkColor(reinterpret_cast<HDC>(wParam), gTheme.Color(UiColorRole::EditBackground));
            SetTextColor(reinterpret_cast<HDC>(wParam), gTheme.Color(UiColorRole::TextPrimary));
            return reinterpret_cast<LRESULT>(gUi.editBrush);
        }

        case WM_CTLCOLORBTN: {
            const HWND control = reinterpret_cast<HWND>(lParam);
            if (control == gUi.moduleIII || control == gUi.moduleVC || control == gUi.moduleSA || control == gUi.rootDependencies) {
                SetBkMode(reinterpret_cast<HDC>(wParam), TRANSPARENT);
                return reinterpret_cast<LRESULT>(gUi.windowBrush);
            }
            return DefWindowProcW(window, message, wParam, lParam);
        }

        case WM_ERASEBKGND:
            PaintWindowBackground(window, reinterpret_cast<HDC>(wParam));
            return 1;

        case WM_TIMER:
            if (wParam == TimerMarquee && gUi.marquee && gUi.progressBar) {
                gUi.marqueePosition += gUi.marqueeDirection * 2.6f;
                if (gUi.marqueePosition >= 100.0f) {
                    gUi.marqueePosition = 100.0f;
                    gUi.marqueeDirection = -1.0f;
                } else if (gUi.marqueePosition <= 0.0f) {
                    gUi.marqueePosition = 0.0f;
                    gUi.marqueeDirection = 1.0f;
                }
                SendMessageW(gUi.progressBar, PBM_SETPOS, static_cast<WPARAM>(gUi.marqueePosition), 0);
                return 0;
            }
            return DefWindowProcW(window, message, wParam, lParam);

        case MsgStatus: {
            std::wstring* status = reinterpret_cast<std::wstring*>(lParam);
            if (status) {
                SetStatus(*status);
                delete status;
            }
            return 0;
        }

        case MsgLog: {
            std::wstring* line = reinterpret_cast<std::wstring*>(lParam);
            if (line) {
                AppendUiLog(*line);
                delete line;
            }
            return 0;
        }

        case MsgProgress:
            if (wParam == 0) {
                StartMarquee();
            } else {
                SetProgressValue(static_cast<int>(wParam) - 1);
            }
            return 0;

        case MsgQuestion: {
            QuestionRequest* request = reinterpret_cast<QuestionRequest*>(lParam);
            if (request && request->completed) {
                request->result = MessageBoxW(gUi.window, request->text.c_str(), request->title.c_str(), request->flags);
                SetEvent(request->completed);
            }
            return 0;
        }

        case MsgFetchDone: {
            JoinWorker();
            FetchResult* result = reinterpret_cast<FetchResult*>(lParam);
            SetBusyUi(false);
            SetControlText(gUi.refreshButton, L"检查更新");
            if (result) {
                if (result->ok) {
                    gUi.currentRelease = result->release;
                    gUi.isReleaseFetched = true;
                    UpdateVersionLabels();
                    SetStatus(L"状态：已获取版本信息（" + WideFromUtf8(result->release.versionSource) + L"）");
                    AppendUiLog(L"最新版本：" + WideFromUtf8(result->release.tagName)
                        + L"，安装包：" + WideFromUtf8(result->release.assetName)
                        + L"，源：" + WideFromUtf8(result->release.versionSource));
                } else {
                    SetStatus(result->cancelled ? L"状态：已取消检查更新" : L"状态：检查更新失败");
                    SetControlText(gUi.versionSourceText, result->cancelled ? L"检测源：已取消" : L"检测源：检查更新失败");
                    AppendUiLog(L"检查更新失败：" + result->error);
                    if (!result->cancelled) {
                        MessageBoxW(gUi.window, result->error.c_str(), InstallerTitle, MB_ICONERROR);
                    }
                }
                delete result;
            }
            return 0;
        }

        case MsgInstallDone: {
            JoinWorker();
            InstallResult* result = reinterpret_cast<InstallResult*>(lParam);
            SetBusyUi(false);
            SetProgressValue(0);
            if (result) {
                if (result->ok) {
                    SetControlText(gUi.localVersionText, L"本地版本：" + result->version);
                    SetStatus(L"状态：安装完成");
                    UpdateSummaryText();
                    AppendUiLog(L"安装/更新完成。");
                    MessageBoxW(gUi.window, result->summary.c_str(), InstallerTitle, MB_ICONINFORMATION);
                } else {
                    SetStatus(result->cancelled ? L"状态：已取消" : L"状态：安装失败");
                    AppendUiLog(result->cancelled ? L"安装已取消。" : result->error);
                    if (!result->cancelled) {
                        MessageBoxW(gUi.window, result->error.c_str(), InstallerTitle, MB_ICONERROR);
                    }
                }
                delete result;
            }
            return 0;
        }

        case WM_CLOSE:
            if (gUi.busy) {
                SetStatus(L"状态：正在处理中，请等待完成或点击取消。");
                return 0;
            }
            DestroyWindow(window);
            return 0;

        case WM_DESTROY:
            JoinWorker();
            StopMarquee();
            DestroyUiBuffers();
            DestroyUiFonts();
            gUi.buttonHover.clear();
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProcW(window, message, wParam, lParam);
        }
    }

    int RunInstallerUi(HINSTANCE instance, int showCommand) {
        EnableDpiAwareness();
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        INITCOMMONCONTROLSEX icc{};
        icc.dwSize = sizeof(icc);
        icc.dwICC = ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES;
        InitCommonControlsEx(&icc);

        gUi.instance = instance;
        gUi.uiThreadId = GetCurrentThreadId();
        gUi.dpi = QuerySystemDpi();

        WNDCLASSW windowClass{};
        windowClass.lpfnWndProc = InstallerWindowProc;
        windowClass.hInstance = instance;
        windowClass.lpszClassName = L"XMenuInstallerWindow";
        windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        windowClass.hbrBackground = nullptr;
        windowClass.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(101));
        RegisterClassW(&windowClass);

        const int windowWidth = Scale(WindowWidth);
        const int windowHeight = Scale(WindowHeight);
        const int screenW = GetSystemMetrics(SM_CXSCREEN);
        const int screenH = GetSystemMetrics(SM_CYSCREEN);
        const int posX = (screenW - windowWidth) / 2;
        const int posY = (screenH - windowHeight) / 2;

        HWND window = CreateWindowExW(
            WS_EX_COMPOSITED, windowClass.lpszClassName, InstallerTitle,
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPCHILDREN,
            posX, posY, windowWidth, windowHeight,
            nullptr, nullptr, instance, nullptr
        );
        if (!window) {
            CoUninitialize();
            return 1;
        }

        ShowWindow(window, showCommand);
        UpdateWindow(window);

        MSG msg{};
        while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        CoUninitialize();
        return static_cast<int>(msg.wParam);
    }
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand) {
    return RunInstallerUi(instance, showCommand);
}
