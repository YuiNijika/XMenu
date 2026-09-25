// 面板外观：强调色色相。单一来源，持久化在 localStorage，
// 应用到 DOM 的方式与 MusicStorm 一致——只发一个 --accent-hue，其余颜色在 CSS 里派生

import { useEffect, useState } from "react"

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

export type AppearancePrefs = {
    accent: AccentTone
    /** 0–359，仅 accent === "custom" 时生效 */
    customHue: number
}

const DEFAULT_CUSTOM_HUE = 280
const DEFAULT_HUE = 260

export function normalizeHue(hue: number): number {
    const rounded = Math.round(hue)
    return Math.min(359, Math.max(0, rounded < 0 ? rounded + 360 : rounded))
}

export function readAppearancePrefs(): AppearancePrefs {
    if (typeof window === "undefined") {
        return { accent: "neutral", customHue: DEFAULT_CUSTOM_HUE }
    }
    try {
        const stored = JSON.parse(window.localStorage.getItem(STORAGE_KEY) ?? "{}") as Partial<AppearancePrefs>
        const accent = stored.accent === "custom" || PRESET_IDS.has(String(stored.accent))
            ? (stored.accent as AccentTone)
            : "neutral"
        const customHue = typeof stored.customHue === "number" ? normalizeHue(stored.customHue) : DEFAULT_CUSTOM_HUE
        return { accent, customHue }
    } catch {
        return { accent: "neutral", customHue: DEFAULT_CUSTOM_HUE }
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

export function applyAppearanceToDom(prefs: AppearancePrefs): void {
    const root = document.documentElement
    root.style.setProperty("--accent-hue", String(resolveAccentHue(prefs)))
    root.dataset.accent = prefs.accent
}

// 面板内响应式读取：变更即应用并持久化
export function useAppearancePrefs() {
    const [prefs, setPrefs] = useState<AppearancePrefs>(() => readAppearancePrefs())

    useEffect(() => {
        applyAppearanceToDom(prefs)
        writeAppearancePrefs(prefs)
    }, [prefs])

    return [prefs, setPrefs] as const
}
