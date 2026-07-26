// --------------------------------------------------------------------------
// 仅供 main.cpp #include 使用
// --------------------------------------------------------------------------
#ifndef XMENU_INSTALLER_MAIN_CPP
#error "_ui_rewrite_tail.cpp must be #included from installer/main.cpp (not compiled as its own TU)"
#endif
    enum class WizardStep {
        Version = 0,
        Path = 1,
        Components = 2,
        Install = 3
    };

    struct InstallerUiState {
        HINSTANCE instance = nullptr;
        HWND window = nullptr;

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

        HFONT titleFont = nullptr;
        HFONT sectionFont = nullptr;
        HFONT normalFont = nullptr;
        HFONT smallFont = nullptr;
        HBRUSH windowBrush = nullptr;
        HBRUSH sidebarBrush = nullptr;
        HBRUSH cardBrush = nullptr;

        std::string gameRoot;
        GameType gameType = GameType::Unknown;
        ReleaseInfo currentRelease;
        bool isReleaseFetched = false;
        WizardStep step = WizardStep::Version;
        bool busy = false;
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
    constexpr int ControlStepVersion = 1100;
    constexpr int ControlStepPath = 1101;
    constexpr int ControlStepComponents = 1102;
    constexpr int ControlStepInstall = 1103;

    constexpr int SidebarWidth = 180;
    constexpr int ContentLeft = 204;
    constexpr int WindowWidth = 900;
    constexpr int WindowHeight = 640;

    InstallerUiState gUi;

    void PumpUi() {
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    void SetStatus(const std::wstring& status) {
        if (gUi.statusText) {
            SetWindowTextW(gUi.statusText, status.c_str());
        }
        PumpUi();
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

    HFONT CreateUiFont(int pointSize, int weight) {
        HDC dc = GetDC(nullptr);
        const int height = -MulDiv(pointSize, GetDeviceCaps(dc, LOGPIXELSY), 72);
        ReleaseDC(nullptr, dc);
        return CreateFontW(
            height, 0, 0, 0, weight, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI"
        );
    }

    void InitUiFonts() {
        gUi.titleFont = CreateUiFont(16, FW_SEMIBOLD);
        gUi.sectionFont = CreateUiFont(11, FW_SEMIBOLD);
        gUi.normalFont = CreateUiFont(9, FW_NORMAL);
        gUi.smallFont = CreateUiFont(8, FW_NORMAL);
        gUi.windowBrush = CreateSolidBrush(RGB(245, 247, 250));
        gUi.sidebarBrush = CreateSolidBrush(RGB(28, 34, 46));
        gUi.cardBrush = CreateSolidBrush(RGB(255, 255, 255));
    }

    void DestroyUiFonts() {
        if (gUi.titleFont) { DeleteObject(gUi.titleFont); gUi.titleFont = nullptr; }
        if (gUi.sectionFont) { DeleteObject(gUi.sectionFont); gUi.sectionFont = nullptr; }
        if (gUi.normalFont) { DeleteObject(gUi.normalFont); gUi.normalFont = nullptr; }
        if (gUi.smallFont) { DeleteObject(gUi.smallFont); gUi.smallFont = nullptr; }
        if (gUi.windowBrush) { DeleteObject(gUi.windowBrush); gUi.windowBrush = nullptr; }
        if (gUi.sidebarBrush) { DeleteObject(gUi.sidebarBrush); gUi.sidebarBrush = nullptr; }
        if (gUi.cardBrush) { DeleteObject(gUi.cardBrush); gUi.cardBrush = nullptr; }
    }

    void SetProgress(int value) {
        if (gUi.progressBar) {
            SendMessageW(gUi.progressBar, PBM_SETPOS, value, 0);
        }
    }

    void OnDownloadProgress(ULONG current, ULONG total) {
        if (total > 0) {
            const int percent = static_cast<int>((static_cast<unsigned long long>(current) * 100ull) / total);
            SetProgress(percent);
            std::wostringstream status;
            status << L"下载安装包：" << percent << L"%";
            SetStatus(status.str());
        } else {
            SetStatus(L"下载安装包：正在接收数据...");
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
            SetControlText(gUi.releaseNotesEdit, L"点击「检查更新」获取版本信息。\r\n优先 GTAMODX，失败时回退 GitHub。");
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

    void SetBusy(bool busy) {
        gUi.busy = busy;
        const BOOL enable = busy ? FALSE : TRUE;
        EnableWindow(gUi.refreshButton, enable);
        EnableWindow(gUi.browseButton, enable);
        EnableWindow(gUi.backButton, enable && gUi.step != WizardStep::Version);
        EnableWindow(gUi.nextButton, enable);
        EnableWindow(gUi.installButton, enable);
        for (HWND stepButton : gUi.stepButtons) {
            EnableWindow(stepButton, enable);
        }
    }

    void ApplyWizardStep(WizardStep step) {
        gUi.step = step;

        const bool isVersion = step == WizardStep::Version;
        const bool isPath = step == WizardStep::Path;
        const bool isComponents = step == WizardStep::Components;
        const bool isInstall = step == WizardStep::Install;

        // 版本页
        ShowControl(gUi.versionSourceText, isVersion);
        ShowControl(gUi.releaseVersionText, isVersion);
        ShowControl(gUi.packageVersionText, isVersion);
        ShowControl(gUi.releaseNotesEdit, isVersion);
        ShowControl(gUi.refreshButton, isVersion);
        ShowControl(gUi.openGtamodxButton, isVersion);
        ShowControl(gUi.openGithubButton, isVersion);
        ShowControl(gUi.notesCaption, isVersion);

        // 目录页
        ShowControl(gUi.pathEdit, isPath);
        ShowControl(gUi.browseButton, isPath);
        ShowControl(gUi.gameTypeText, isPath);
        ShowControl(gUi.localVersionText, isPath);

        // 组件页
        ShowControl(gUi.moduleIII, isComponents);
        ShowControl(gUi.moduleVC, isComponents);
        ShowControl(gUi.moduleSA, isComponents);
        ShowControl(gUi.rootDependencies, isComponents);
        ShowControl(gUi.downloadSource, isComponents);

        // 安装页
        ShowControl(gUi.summaryEdit, isInstall);
        ShowControl(gUi.installButton, isInstall);
        ShowControl(gUi.statusText, isInstall);
        ShowControl(gUi.progressBar, isInstall);
        ShowControl(gUi.logBox, isInstall);

        // 步骤按钮选中态用标题前缀模拟
        const wchar_t* labels[] = {
            L"1  检查版本",
            L"2  选择目录",
            L"3  选择组件",
            L"4  确认安装"
        };
        for (int i = 0; i < 4; ++i) {
            std::wstring text = labels[i];
            if (static_cast<int>(step) == i) {
                text = L"● " + text;
            } else {
                text = L"○ " + text;
            }
            SetControlText(gUi.stepButtons[i], text);
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
        if (step == WizardStep::Install) {
            SetControlText(gUi.nextButton, L"完成");
        } else {
            SetControlText(gUi.nextButton, L"下一步");
        }
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
            // 完成：关闭窗口
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

    void FetchReleaseInfo() {
        SetBusy(true);
        SetStatus(L"状态：检查更新中（优先 GTAMODX）...");
        AppendUiLog(L"开始检查更新：优先 GTAMODX，其次 GitHub。");

        ReleaseInfo release;
        std::string gtamodxVersion;
        std::string githubBody;
        std::string githubTag;

        // 1) GTAMODX: data.version
        const std::string gtamodxPath = TempPathFor("gtamodx.json");
        if (DownloadFile(XMENU_GTAMODX_API, gtamodxPath)) {
            gtamodxVersion = ExtractGtamodxVersion(ReadTextFile(gtamodxPath));
            if (!gtamodxVersion.empty()) {
                AppendUiLog(L"GTAMODX 版本：" + WideFromUtf8(gtamodxVersion));
            } else {
                AppendUiLog(L"GTAMODX 响应缺少 data.version，将回退 GitHub。");
            }
        } else {
            AppendUiLog(L"GTAMODX API 请求失败，将回退 GitHub。");
        }

        // 2) GitHub: 安装包 + 更新日志（必须，用于下载）
        SetStatus(L"状态：获取 GitHub 安装包信息...");
        const std::string githubPath = TempPathFor("release.json");
        if (!DownloadFile(XMENU_GITHUB_API, githubPath)) {
            SetStatus(L"状态：获取安装包失败");
            AppendUiLog(L"GitHub Releases API 请求失败。");
            if (gtamodxVersion.empty()) {
                MessageBoxW(gUi.window, L"GTAMODX 与 GitHub 均不可用，无法检查更新。", InstallerTitle, MB_ICONERROR);
            } else {
                MessageBoxW(gUi.window,
                    L"已从 GTAMODX 获取版本，但 GitHub 安装包信息获取失败。\n安装需要 GitHub Release 资源。",
                    InstallerTitle, MB_ICONERROR);
            }
            SetBusy(false);
            return;
        }

        release = ParseReleaseInfo(ReadTextFile(githubPath));
        githubTag = release.packageVersion;
        githubBody = release.body;

        if (release.assetUrl.empty()) {
            SetStatus(L"状态：未找到可安装的 release 包");
            AppendUiLog(L"GitHub release 中未找到可安装的 zip 包。");
            MessageBoxW(gUi.window, L"GitHub release 中未找到可安装的 zip 包。请上传 XMenuIII.VC.SA.zip。", InstallerTitle, MB_ICONERROR);
            SetBusy(false);
            return;
        }

        // 版本：GTAMODX 优先，失败用 GitHub tag
        if (!gtamodxVersion.empty()) {
            release.tagName = gtamodxVersion;
            release.versionSource = "GTAMODX";
            release.packageVersion = githubTag.empty() ? release.tagName : githubTag;
            AppendUiLog(L"版本检测源：GTAMODX");
        } else if (!githubTag.empty()) {
            release.tagName = githubTag;
            release.packageVersion = githubTag;
            release.versionSource = "GitHub";
            AppendUiLog(L"版本检测源：GitHub（GTAMODX 不可用）");
        } else {
            SetStatus(L"状态：未获取到版本号");
            AppendUiLog(L"未能解析任何版本号。");
            MessageBoxW(gUi.window, L"未能解析版本号。", InstallerTitle, MB_ICONERROR);
            SetBusy(false);
            return;
        }

        release.body = githubBody;
        gUi.currentRelease = release;
        gUi.isReleaseFetched = true;
        UpdateVersionLabels();

        SetStatus(L"状态：已获取版本信息（" + WideFromUtf8(release.versionSource) + L"）");
        AppendUiLog(L"最新版本：" + WideFromUtf8(release.tagName)
            + L"，安装包：" + WideFromUtf8(release.assetName)
            + L"，源：" + WideFromUtf8(release.versionSource));
        SetBusy(false);
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
        SetProgress(0);
        SetStatus(L"状态：已选择游戏目录");
        AppendUiLog(L"已选择游戏目录：" + WideFromAnsi(gameRoot) + L" → " + GameTypeName(gUi.gameType));
    }

    bool ExecuteInstallFromUi() {
        if (!gUi.isReleaseFetched) {
            MessageBoxW(gUi.window, L"请先检查更新。", InstallerTitle, MB_ICONWARNING);
            ApplyWizardStep(WizardStep::Version);
            return false;
        }
        if (gUi.gameRoot.empty()) {
            MessageBoxW(gUi.window, L"请先选择游戏目录。", InstallerTitle, MB_ICONWARNING);
            ApplyWizardStep(WizardStep::Path);
            return false;
        }

        InstallOptions options = ReadOptionsFromUi();
        if (!HasSelectedGameModule(options)) {
            MessageBoxW(gUi.window, L"请至少选择一个 XMenu 游戏模块。", InstallerTitle, MB_ICONWARNING);
            ApplyWizardStep(WizardStep::Components);
            return false;
        }

        SetBusy(true);
        SetProgress(0);

        const DWORD startedTick = GetTickCount();
        const std::string startedAt = CurrentTimestamp();
        AppendInstallLog(gUi.gameRoot, "===== XMenu installer started at " + startedAt + " =====");
        AppendInstallLog(gUi.gameRoot, "Game root: " + gUi.gameRoot);
        AppendInstallLog(gUi.gameRoot, "Selected components:\r\n" + BuildSelectedComponentSummary(options));
        AppendInstallLog(gUi.gameRoot, "Version source: " + gUi.currentRelease.versionSource);

        const std::string installedVersion = ReadInstalledVersion(gUi.gameRoot);
        std::string integrityReport;
        VerifyInstalledFiles(gUi.gameRoot, integrityReport);
        AppendInstallLog(gUi.gameRoot, "Installed version: " + (installedVersion.empty() ? std::string("none") : installedVersion));
        AppendInstallLog(gUi.gameRoot, "Integrity before install: " + integrityReport);

        const ReleaseInfo release = gUi.currentRelease;
        AppendUiLog(L"检测源：" + WideFromUtf8(release.versionSource) + L"，版本：" + WideFromUtf8(release.tagName));
        AppendInstallLog(gUi.gameRoot, "Release version: " + release.tagName);
        AppendInstallLog(gUi.gameRoot, "Package version: " + (release.packageVersion.empty() ? release.tagName : release.packageVersion));
        AppendInstallLog(gUi.gameRoot, "Selected asset: " + release.assetName);
        AppendInstallLog(gUi.gameRoot, "Asset official URL: " + release.assetUrl);

        const int downloadSourceIndex = SelectedDownloadSourceIndex();
        const DownloadSource& downloadSource = SelectedDownloadSource();
        const std::string downloadUrl = BuildDownloadUrl(release.assetUrl, downloadSourceIndex);
        const std::string downloadSourceName = AnsiFromWide(downloadSource.label);

        std::wstring confirm;
        confirm += installedVersion.empty() ? L"将执行安装：\n\n" : L"将执行更新：\n\n";
        confirm += L"本地版本：" + WideFromUtf8(installedVersion.empty() ? "未安装" : installedVersion) + L"\n";
        confirm += L"检测源：" + WideFromUtf8(release.versionSource) + L"\n";
        confirm += L"最新版本：" + WideFromUtf8(release.tagName) + L"\n";
        confirm += L"安装包版本：" + WideFromUtf8(release.packageVersion.empty() ? release.tagName : release.packageVersion) + L"\n";
        confirm += L"Release 包：" + WideFromUtf8(release.assetName) + L"\n";
        confirm += L"下载源：" + WideFromUtf8(downloadSourceName) + L"\n";
        confirm += L"完整性状态：" + WideFromUtf8(integrityReport) + L"\n\n";
        confirm += L"选择的组件：\n" + WideFromUtf8(BuildSelectedComponentSummary(options)) + L"\n继续安装？";
        if (MessageBoxW(gUi.window, confirm.c_str(), InstallerTitle, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1) != IDYES) {
            AppendUiLog(L"用户取消安装。");
            AppendInstallLog(gUi.gameRoot, "User cancelled install after release check");
            SetBusy(false);
            return false;
        }

        // 安装写入 manifest 用 packageVersion（实际包），展示用 tagName
        const std::string installVersion = release.packageVersion.empty() ? release.tagName : release.packageVersion;

        const std::string zipPath = TempPathFor(release.assetName.empty() ? "release.zip" : release.assetName);
        SetProgress(0);
        SetStatus(L"状态：准备下载安装包...");
        AppendUiLog(L"开始下载安装包，下载源：" + WideFromUtf8(downloadSourceName));
        AppendInstallLog(gUi.gameRoot, "Download source: " + downloadSourceName);
        AppendInstallLog(gUi.gameRoot, "Download URL: " + downloadUrl);
        AppendInstallLog(gUi.gameRoot, "Downloading asset to: " + zipPath);
        try {
            DownloadFileOrThrow(downloadUrl, zipPath, downloadSourceName, OnDownloadProgress);
        } catch (const std::exception& error) {
            const std::string errorText = error.what();
            SetStatus(L"状态：下载安装包失败");
            AppendUiLog(L"下载安装包失败：" + WideFromUtf8(errorText));
            AppendInstallLog(gUi.gameRoot, "ERROR asset download failed: " + errorText);
            const std::wstring message = L"下载安装包失败。\n\n下载源：" + WideFromUtf8(downloadSourceName)
                + L"\n错误：" + WideFromUtf8(errorText)
                + L"\n\n可以在「选择组件」步骤切换代理后重试。";
            MessageBoxW(gUi.window, message.c_str(), InstallerTitle, MB_ICONERROR);
            SetBusy(false);
            return false;
        }

        const std::string extractDir = TempPathFor("extract");
        RunHiddenAndWait("cmd.exe /c rmdir /s /q \"" + extractDir + "\"");
        SetProgress(100);
        SetStatus(L"状态：解压安装包...");
        AppendUiLog(L"开始解压安装包...");
        AppendInstallLog(gUi.gameRoot, "Extracting asset to: " + extractDir);
        if (!ExtractZip(zipPath, extractDir)) {
            AppendUiLog(L"解压安装包失败。");
            AppendInstallLog(gUi.gameRoot, "ERROR asset extraction failed");
            MessageBoxW(gUi.window, L"解压安装包失败。", InstallerTitle, MB_ICONERROR);
            SetBusy(false);
            return false;
        }

        AppendUiLog(L"开始写入游戏目录...");
        SetStatus(L"状态：写入游戏目录...");
        const bool ok = InstallRelease(extractDir, gUi.gameRoot, installVersion, options);
        const DWORD finishedTick = GetTickCount();
        const std::string finishedAt = CurrentTimestamp();
        const std::string duration = FormatDuration(startedTick, finishedTick);

        if (ok) {
            std::string finalReport;
            VerifyInstalledFiles(gUi.gameRoot, finalReport);
            AppendInstallLog(gUi.gameRoot, "Integrity after install: " + finalReport);
            AppendInstallLog(gUi.gameRoot, "Installer finished at " + finishedAt + ", duration " + duration);
            AppendUiLog(L"安装/更新完成。");
            SetControlText(gUi.localVersionText, L"本地版本：" + WideFromUtf8(installVersion));
            SetStatus(L"状态：安装完成");
            UpdateSummaryText();

            const std::wstring message = L"XMenu 安装/更新完成。\n\n检测源：" + WideFromUtf8(release.versionSource)
                + L"\n版本：" + WideFromUtf8(installVersion)
                + L"\n包：" + WideFromUtf8(release.assetName)
                + L"\n开始时间：" + WideFromUtf8(startedAt)
                + L"\n结束时间：" + WideFromUtf8(finishedAt)
                + L"\n耗时：" + WideFromUtf8(duration)
                + L"\n" + WideFromUtf8(finalReport);
            MessageBoxW(gUi.window, message.c_str(), InstallerTitle, MB_ICONINFORMATION);
        } else {
            AppendInstallLog(gUi.gameRoot, "ERROR install failed at " + finishedAt + ", duration " + duration);
            AppendUiLog(L"安装失败。请查看 plugins\\XMenu\\install.log。");
            SetStatus(L"状态：安装失败");
        }

        SetBusy(false);
        return ok;
    }

    HWND CreateLabel(HWND parent, const wchar_t* text, int x, int y, int w, int h, HFONT font) {
        HWND hwnd = CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, w, h, parent, nullptr, gUi.instance, nullptr);
        ApplyFont(hwnd, font);
        return hwnd;
    }

    HWND CreateButton(HWND parent, const wchar_t* text, int id, int x, int y, int w, int h) {
        HWND hwnd = CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP, x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), gUi.instance, nullptr);
        ApplyFont(hwnd, gUi.normalFont);
        return hwnd;
    }

    HWND CreateCheckbox(HWND parent, const wchar_t* text, int id, int x, int y, int w, int h) {
        HWND hwnd = CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), gUi.instance, nullptr);
        ApplyFont(hwnd, gUi.normalFont);
        return hwnd;
    }

    HWND CreateReadonlyEdit(HWND parent, const wchar_t* text, int x, int y, int w, int h) {
        HWND hwnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", text, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY, x, y, w, h, parent, nullptr, gUi.instance, nullptr);
        ApplyFont(hwnd, gUi.normalFont);
        return hwnd;
    }

    HWND CreateMultilineReadonlyEdit(HWND parent, const wchar_t* text, int x, int y, int w, int h) {
        HWND hwnd = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", text,
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL,
            x, y, w, h, parent, nullptr, gUi.instance, nullptr
        );
        ApplyFont(hwnd, gUi.normalFont);
        return hwnd;
    }

    HWND CreateDownloadSourceCombo(HWND parent, int x, int y, int w, int h) {
        HWND hwnd = CreateWindowExW(
            0, L"COMBOBOX", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
            x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ControlDownloadSource)), gUi.instance, nullptr
        );
        ApplyFont(hwnd, gUi.normalFont);
        for (const DownloadSource& source : DownloadSources) {
            SendMessageW(hwnd, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(source.label));
        }
        SendMessageW(hwnd, CB_SETCURSEL, DefaultDownloadSourceIndex, 0);
        return hwnd;
    }

    void CreateInstallerControls(HWND window) {
        // 侧边栏步骤
        const int stepIds[] = { ControlStepVersion, ControlStepPath, ControlStepComponents, ControlStepInstall };
        const wchar_t* stepLabels[] = { L"1  检查版本", L"2  选择目录", L"3  选择组件", L"4  确认安装" };
        for (int i = 0; i < 4; ++i) {
            gUi.stepButtons[i] = CreateWindowExW(
                0, L"BUTTON", stepLabels[i],
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_LEFT | BS_FLAT,
                16, 90 + i * 48, SidebarWidth - 32, 36,
                window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(stepIds[i])), gUi.instance, nullptr
            );
            ApplyFont(gUi.stepButtons[i], gUi.normalFont);
        }

        gUi.brandTitle = CreateLabel(window, L"XMenu", 20, 24, 140, 28, gUi.titleFont);
        gUi.brandSubtitle = CreateLabel(window, L"Installer", 20, 52, 140, 20, gUi.smallFont);

        // 内容区标题
        gUi.headerTitle = CreateLabel(window, L"检查版本", ContentLeft, 20, 500, 28, gUi.titleFont);
        gUi.headerSubtitle = CreateLabel(window, L"优先 GTAMODX，失败回退 GitHub", ContentLeft, 50, 560, 20, gUi.smallFont);

        // --- 版本页 ---
        gUi.versionSourceText = CreateLabel(window, L"检测源：尚未检查", ContentLeft, 96, 420, 22, gUi.normalFont);
        gUi.releaseVersionText = CreateLabel(window, L"最新版本：—", ContentLeft, 122, 420, 22, gUi.sectionFont);
        gUi.packageVersionText = CreateLabel(window, L"安装包版本：—", ContentLeft, 148, 520, 22, gUi.normalFont);
        gUi.refreshButton = CreateButton(window, L"检查更新", ControlFetchRelease, ContentLeft + 470, 112, 100, 32);
        gUi.openGtamodxButton = CreateButton(window, L"打开 GTAMODX", ControlOpenGtamodx, ContentLeft, 176, 120, 28);
        gUi.openGithubButton = CreateButton(window, L"打开 GitHub", ControlOpenGithub, ContentLeft + 132, 176, 110, 28);
        gUi.notesCaption = CreateLabel(window, L"更新日志（来自 GitHub Release）", ContentLeft, 218, 300, 20, gUi.smallFont);
        gUi.releaseNotesEdit = CreateMultilineReadonlyEdit(
            window,
            L"启动后将自动检查更新。\r\n优先使用 GTAMODX API 的 data.version。",
            ContentLeft, 242, 570, 250
        );

        // --- 目录页 ---
        gUi.pathEdit = CreateReadonlyEdit(window, L"尚未选择游戏目录", ContentLeft, 124, 450, 30);
        gUi.browseButton = CreateButton(window, L"浏览...", ControlBrowse, ContentLeft + 460, 123, 100, 32);
        gUi.gameTypeText = CreateLabel(window, L"检测到游戏：未选择", ContentLeft, 170, 400, 22, gUi.normalFont);
        gUi.localVersionText = CreateLabel(window, L"本地版本：未检测", ContentLeft, 198, 400, 22, gUi.normalFont);

        // --- 组件页 ---
        gUi.moduleIII = CreateCheckbox(window, L"安装 GTA III 模块（XMenuIII.dll）", ControlModuleIII, ContentLeft, 110, 360, 26);
        gUi.moduleVC = CreateCheckbox(window, L"安装 GTA Vice City 模块（XMenuVC.dll）", ControlModuleVC, ContentLeft, 146, 380, 26);
        gUi.moduleSA = CreateCheckbox(window, L"安装 GTA San Andreas 模块（XMenuSA.dll）", ControlModuleSA, ContentLeft, 182, 400, 26);
        gUi.rootDependencies = CreateCheckbox(window, L"安装 Ultimate ASI Loader / D3D8to9 到游戏根目录", ControlRootDependencies, ContentLeft, 230, 420, 26);
        gUi.downloadSource = CreateDownloadSourceCombo(window, ContentLeft, 306, 360, 160);

        // --- 安装页 ---
        gUi.summaryEdit = CreateMultilineReadonlyEdit(window, L"", ContentLeft, 96, 570, 200);
        gUi.statusText = CreateLabel(window, L"状态：就绪", ContentLeft, 310, 420, 22, gUi.normalFont);
        gUi.progressBar = CreateWindowExW(0, PROGRESS_CLASSW, L"", WS_CHILD | WS_VISIBLE, ContentLeft, 340, 450, 18, window, nullptr, gUi.instance, nullptr);
        SendMessageW(gUi.progressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendMessageW(gUi.progressBar, PBM_SETPOS, 0, 0);
        gUi.installButton = CreateButton(window, L"开始安装", ControlInstall, ContentLeft + 470, 308, 100, 48);
        gUi.logBox = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"安装器已启动。\r\n",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
            ContentLeft, 372, 570, 120, window, nullptr, gUi.instance, nullptr
        );
        ApplyFont(gUi.logBox, gUi.smallFont);

        // 底部导航
        gUi.backButton = CreateButton(window, L"上一步", ControlBack, ContentLeft, 540, 100, 32);
        gUi.nextButton = CreateButton(window, L"下一步", ControlNext, ContentLeft + 470, 540, 100, 32);

        ApplyWizardStep(WizardStep::Version);
        UpdateVersionLabels();
    }

    LRESULT CALLBACK InstallerWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
        case WM_CREATE:
            gUi.window = window;
            InitUiFonts();
            CreateInstallerControls(window);
            // 启动后自动检查更新
            PostMessageW(window, WM_COMMAND, MAKEWPARAM(ControlFetchRelease, BN_CLICKED), 0);
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
                    FetchReleaseInfo();
                }
                return 0;
            case ControlInstall:
                if (!gUi.busy) {
                    ExecuteInstallFromUi();
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
            default:
                return 0;
            }

        case WM_CTLCOLORSTATIC: {
            const HWND control = reinterpret_cast<HWND>(lParam);
            HDC hdc = reinterpret_cast<HDC>(wParam);
            if (control == gUi.pathEdit || control == gUi.logBox || control == gUi.releaseNotesEdit || control == gUi.summaryEdit) {
                SetBkMode(hdc, OPAQUE);
                SetTextColor(hdc, RGB(30, 30, 30));
                return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
            }
            if (control == gUi.brandTitle || control == gUi.brandSubtitle) {
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(235, 240, 248));
                return reinterpret_cast<LRESULT>(gUi.sidebarBrush ? gUi.sidebarBrush : GetStockObject(DKGRAY_BRUSH));
            }
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(40, 44, 52));
            return reinterpret_cast<LRESULT>(gUi.windowBrush ? gUi.windowBrush : GetStockObject(WHITE_BRUSH));
        }

        case WM_CTLCOLORBTN: {
            const HWND control = reinterpret_cast<HWND>(lParam);
            if (control == gUi.moduleIII || control == gUi.moduleVC || control == gUi.moduleSA || control == gUi.rootDependencies) {
                SetBkMode(reinterpret_cast<HDC>(wParam), TRANSPARENT);
                return reinterpret_cast<LRESULT>(gUi.windowBrush ? gUi.windowBrush : GetStockObject(WHITE_BRUSH));
            }
            // 侧边栏步骤按钮深色背景
            for (HWND stepButton : gUi.stepButtons) {
                if (control == stepButton) {
                    SetBkMode(reinterpret_cast<HDC>(wParam), TRANSPARENT);
                    SetTextColor(reinterpret_cast<HDC>(wParam), RGB(230, 234, 240));
                    return reinterpret_cast<LRESULT>(gUi.sidebarBrush ? gUi.sidebarBrush : GetStockObject(DKGRAY_BRUSH));
                }
            }
            return DefWindowProcW(window, message, wParam, lParam);
        }

        case WM_ERASEBKGND: {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            RECT rect{};
            GetClientRect(window, &rect);

            RECT sidebar = rect;
            sidebar.right = SidebarWidth;
            FillRect(hdc, &sidebar, gUi.sidebarBrush ? gUi.sidebarBrush : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));

            RECT content = rect;
            content.left = SidebarWidth;
            FillRect(hdc, &content, gUi.windowBrush ? gUi.windowBrush : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
            return 1;
        }

        case WM_DESTROY:
            DestroyUiFonts();
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProcW(window, message, wParam, lParam);
        }
    }

    int RunInstallerUi(HINSTANCE instance, int showCommand) {
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        INITCOMMONCONTROLSEX icc{};
        icc.dwSize = sizeof(icc);
        icc.dwICC = ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES;
        InitCommonControlsEx(&icc);

        gUi.instance = instance;

        WNDCLASSW windowClass{};
        windowClass.lpfnWndProc = InstallerWindowProc;
        windowClass.hInstance = instance;
        windowClass.lpszClassName = L"XMenuInstallerWindow";
        windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        windowClass.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(101));
        RegisterClassW(&windowClass);

        const int screenW = GetSystemMetrics(SM_CXSCREEN);
        const int screenH = GetSystemMetrics(SM_CYSCREEN);
        const int posX = (screenW - WindowWidth) / 2;
        const int posY = (screenH - WindowHeight) / 2;

        HWND window = CreateWindowExW(
            0, windowClass.lpszClassName, InstallerTitle,
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            posX, posY, WindowWidth, WindowHeight,
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