// 外观单一来源：明暗 + 强调色。结构与 MusicStorm 的 theme-provider 对齐——
// 一个 context 持有偏好，读写都经过它，顶栏调色板与设置页因此永远看到同一份状态

import {
    createContext,
    useCallback,
    useContext,
    useEffect,
    useLayoutEffect,
    useMemo,
    useState,
    type ReactNode,
} from "react"

import {
    applyAppearanceToDom,
    getSystemTheme,
    normalizeHue,
    readAppearancePrefs,
    resolveTheme,
    writeAppearancePrefs,
    type AccentTone,
    type AppearancePrefs,
    type ResolvedTheme,
    type ThemeMode,
} from "@/lib/appearance"

type AppearanceContextValue = {
    prefs: AppearancePrefs
    theme: ThemeMode
    resolvedTheme: ResolvedTheme
    setTheme: (theme: ThemeMode) => void
    /** 深浅对切，跟随系统时按当前解析结果取反 */
    toggleTheme: () => void
    setAccent: (accent: AccentTone) => void
    /** 切到自定义色调并写入色相 0–359 */
    setCustomHue: (hue: number) => void
}

const AppearanceContext = createContext<AppearanceContextValue | null>(null)

export function AppearanceProvider({ children }: { children: ReactNode }) {
    const [prefs, setPrefs] = useState<AppearancePrefs>(() => readAppearancePrefs())
    const [resolvedTheme, setResolvedTheme] = useState<ResolvedTheme>(() =>
        resolveTheme(prefs.theme),
    )

    // 首帧就要落地：放在 layout effect 里，浏览器绘制前已经切好深浅，不会闪一下浅色
    useLayoutEffect(() => {
        applyAppearanceToDom(prefs)
        writeAppearancePrefs(prefs)
        setResolvedTheme(resolveTheme(prefs.theme))
    }, [prefs])

    // 跟随系统时才监听，选了固定深浅就不用再管系统变化
    useEffect(() => {
        if (prefs.theme !== "system" || typeof window === "undefined" || typeof window.matchMedia !== "function") {
            return
        }
        const media = window.matchMedia("(prefers-color-scheme: dark)")
        const onChange = () => {
            setResolvedTheme(getSystemTheme())
            applyAppearanceToDom({ ...prefs, theme: "system" })
        }
        media.addEventListener("change", onChange)
        return () => media.removeEventListener("change", onChange)
    }, [prefs])

    const setTheme = useCallback((theme: ThemeMode) => {
        setPrefs((previous) => ({ ...previous, theme }))
    }, [])

    const toggleTheme = useCallback(() => {
        setPrefs((previous) => ({
            ...previous,
            theme: resolveTheme(previous.theme) === "dark" ? "light" : "dark",
        }))
    }, [])

    const setAccent = useCallback((accent: AccentTone) => {
        setPrefs((previous) => ({ ...previous, accent }))
    }, [])

    const setCustomHue = useCallback((hue: number) => {
        setPrefs((previous) => ({
            ...previous,
            accent: "custom",
            customHue: normalizeHue(hue),
        }))
    }, [])

    const value = useMemo(
        () => ({
            prefs,
            theme: prefs.theme,
            resolvedTheme,
            setTheme,
            toggleTheme,
            setAccent,
            setCustomHue,
        }),
        [prefs, resolvedTheme, setTheme, toggleTheme, setAccent, setCustomHue],
    )

    return <AppearanceContext.Provider value={value}>{children}</AppearanceContext.Provider>
}

export function useAppearance() {
    const context = useContext(AppearanceContext)
    if (!context) {
        throw new Error("useAppearance must be used within AppearanceProvider")
    }
    return context
}
