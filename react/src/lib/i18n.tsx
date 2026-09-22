import { createContext, useCallback, useContext, useEffect, useMemo, useRef, useState } from 'react'
import { call, isBridgeAvailable, on } from '@/lib/bridge'

type Dictionary = Record<string, string>

type I18nContextValue = {
  lang: string
  ready: boolean
  t: (key: string, fallback?: string) => string
}

type DictionaryPayload = {
  lang: string
  entries: Dictionary
}

type LanguageIndex = {
  code?: string
  files?: string[]
}

const DefaultLanguage = 'zh'
const DataRoot = './data/i18n'

const I18nContext = createContext<I18nContextValue>({
  lang: DefaultLanguage,
  ready: false,
  t: (key, fallback) => fallback ?? key,
})

let activeTranslate: (key: string, fallback?: string) => string = (key, fallback) => fallback ?? key

// 桥之外的地方也能取词条，比如动作结果提示
export function translateKey(key: string, fallback?: string): string {
  return activeTranslate(key, fallback)
}

async function fetchJson<T>(path: string): Promise<T | null> {
  try {
    const response = await fetch(path, { cache: 'no-cache' })
    if (!response.ok) {
      return null
    }
    return (await response.json()) as T
  } catch {
    return null
  }
}

// 与 ImGui 完全同源：读 data/i18n 下的 json，按 index.json 列出的文件逐个合并
async function loadFromFiles(lang: string): Promise<Dictionary> {
  const index = await fetchJson<LanguageIndex>(`${DataRoot}/${lang}/index.json`)
  const files = index?.files ?? []
  if (files.length === 0) {
    return {}
  }

  const parts = await Promise.all(files.map((file) => fetchJson<Dictionary>(`${DataRoot}/${lang}/${file}`)))
  const merged: Dictionary = {}
  for (const part of parts) {
    if (!part) {
      continue
    }
    for (const [key, value] of Object.entries(part)) {
      if (typeof value === 'string') {
        merged[key] = value
      }
    }
  }
  return merged
}

// 语言以宿主为准，宿主不可用时读配置，最后才退回默认语言
async function resolveLanguage(): Promise<string> {
  if (isBridgeAvailable()) {
    try {
      const info = await call<{ lang?: string }>('menu.info')
      if (info?.lang) {
        return info.lang
      }
    } catch {
      // 桥不可用时继续往下找
    }
  }
  const config = await fetchJson<{ menu?: { language?: string } }>('./config.json')
  return config?.menu?.language ?? DefaultLanguage
}

export function I18nProvider({ children }: { children: React.ReactNode }) {
  const [lang, setLang] = useState(DefaultLanguage)
  const [entries, setEntries] = useState<Dictionary>({})
  const [ready, setReady] = useState(false)
  const mounted = useRef(true)

  const load = useCallback(async (requested?: string) => {
    const target = requested ?? (await resolveLanguage())
    let dictionary = await loadFromFiles(target)

    // 文件缺失时退回宿主下发的词条，保证面板仍有文案
    if (Object.keys(dictionary).length === 0 && isBridgeAvailable()) {
      try {
        const payload = await call<DictionaryPayload>('i18n.dictionary')
        dictionary = payload.entries ?? {}
      } catch {
        dictionary = {}
      }
    }

    if (!mounted.current) {
      return
    }
    setLang(target)
    setEntries(dictionary)
    setReady(true)
  }, [])

  useEffect(() => {
    mounted.current = true
    void load()
    on('i18n.changed', (payload) => {
      const code = (payload as { lang?: string } | null)?.lang
      void load(code)
    })
    return () => {
      mounted.current = false
    }
  }, [load])

  const value = useMemo<I18nContextValue>(() => {
    const translate = (key: string, fallback?: string) => entries[key] ?? fallback ?? key
    activeTranslate = translate
    return { lang, ready, t: translate }
  }, [lang, ready, entries])

  return <I18nContext.Provider value={value}>{children}</I18nContext.Provider>
}

export function useI18n(): I18nContextValue {
  return useContext(I18nContext)
}
