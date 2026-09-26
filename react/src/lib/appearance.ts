import type { Dispatch, SetStateAction } from "react"

export type AccentTone =
    | "neutral"
    | "rose"
    | "pink"
    | "orange"
    | "amber"
    | "green"
    | "teal"
    | "cyan"
    | "blue"
    | "indigo"
    | "violet"
    | "custom"

export type AccentOption = {
    id: Exclude<AccentTone, "custom">
    labelKey: string
    hue: number
}

// 默认色调，强调色只作用于交互元素
export const ACCENT_OPTIONS: AccentOption[] = [
    { id: "neutral", labelKey: "react.accent.neutral", hue: 260 },
    { id: "rose", labelKey: "react.accent.rose", hue: 350 },
    { id: "pink", labelKey: "react.accent.pink", hue: 340 },
    { id: "orange", labelKey: "react.accent.orange", hue: 45 },
    { id: "amber", labelKey: "react.accent.amber", hue: 75 },
    { id: "green", labelKey: "react.accent.green", hue: 155 },
    { id: "teal", labelKey: "react.accent.teal", hue: 185 },
    { id: "cyan", labelKey: "react.accent.cyan", hue: 205 },
    { id: "blue", labelKey: "react.accent.blue", hue: 230 },
    { id: "indigo", labelKey: "react.accent.indigo", hue: 265 },
    { id: "violet", labelKey: "react.accent.violet", hue: 300 },
]

const PRESET_IDS: Set<string> = new Set(ACCENT_OPTIONS.map((item) => item.id))

const STORAGE_KEY = "xmenu-appearance"

/** 明暗模式，与 MusicStorm 的 Theme 一致：system 时按 prefers-color-scheme 解析 */
export type ThemeMode = "light" | "dark" | "system"
export type ResolvedTheme = "light" | "dark"

export type AppearancePrefs = {
    accent: AccentTone
    /** 0–359，仅 accent === "custom" 时生效 */
    customHue: number
    theme: ThemeMode
}

const DEFAULT_CUSTOM_HUE = 280
const DEFAULT_HUE = 260
/** 面板一直是深色，没有存过时沿用深色，避免升级后观感突变 */
const DEFAULT_THEME: ThemeMode = "dark"

export const THEME_MODES: ThemeMode[] = ["system", "light", "dark"]

/** 明暗三个选项对应的词条键，顶栏调色板与设置页共用同一份 */
export const THEME_LABEL_KEYS: Record<ThemeMode, string> = {
    system: "react.theme.system",
    light: "react.theme.light",
    dark: "react.theme.dark",
}

export function normalizeHue(hue: number): number {
    const rounded = Math.round(hue)
    return Math.min(359, Math.max(0, rounded < 0 ? rounded + 360 : rounded))
}

export function getSystemTheme(): ResolvedTheme {
    if (typeof window === "undefined" || typeof window.matchMedia !== "function") {
        return "dark"
    }
    return window.matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light"
}

export function resolveTheme(theme: ThemeMode): ResolvedTheme {
    return theme === "system" ? getSystemTheme() : theme
}

export function readAppearancePrefs(): AppearancePrefs {
    const fallback: AppearancePrefs = { accent: "neutral", customHue: DEFAULT_CUSTOM_HUE, theme: DEFAULT_THEME }
    if (typeof window === "undefined") {
        return fallback
    }
    try {
        const stored = JSON.parse(window.localStorage.getItem(STORAGE_KEY) ?? "{}") as Partial<AppearancePrefs>
        const accent = stored.accent === "custom" || PRESET_IDS.has(String(stored.accent))
            ? (stored.accent as AccentTone)
            : "neutral"
        const customHue = typeof stored.customHue === "number" ? normalizeHue(stored.customHue) : DEFAULT_CUSTOM_HUE
        const theme: ThemeMode =
            stored.theme === "light" || stored.theme === "dark" || stored.theme === "system"
                ? stored.theme
                : DEFAULT_THEME
        return { accent, customHue, theme }
    } catch {
        return fallback
    }
}

export function writeAppearancePrefs(prefs: AppearancePrefs): void {
    if (typeof window === "undefined") {
        return
    }
    window.localStorage.setItem(STORAGE_KEY, JSON.stringify(prefs))
}

export function resolveAccentHue(prefs: AppearancePrefs): number {
    if (prefs.accent === "custom") {
        return prefs.customHue
    }
    return ACCENT_OPTIONS.find((item) => item.id === prefs.accent)?.hue ?? DEFAULT_HUE
}

export function isNeutralAccent(prefs: AppearancePrefs): boolean {
    return prefs.accent === "neutral"
}

// 与 MusicStorm 的 applyTheme 同一步骤：切 .dark 类并写 data-theme
export function applyThemeToDom(resolved: ResolvedTheme): void {
    const root = document.documentElement
    root.classList.toggle("dark", resolved === "dark")
    root.dataset.theme = resolved
}

export function applyAppearanceToDom(prefs: AppearancePrefs): void {
    const root = document.documentElement
    root.style.setProperty("--accent-hue", String(resolveAccentHue(prefs)))
    root.dataset.accent = prefs.accent
    applyThemeToDom(resolveTheme(prefs.theme))
}

export type AppearancePrefsSetter = Dispatch<SetStateAction<AppearancePrefs>>
