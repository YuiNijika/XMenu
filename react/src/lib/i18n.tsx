import { createContext, useCallback, useContext, useEffect, useMemo, useRef, useState } from 'react'
import { call, isBridgeAvailable, on } from '@/lib/bridge'
import { DefaultLanguage, fetchJson, loadDictionary, validEntries, type Dictionary } from '@/lib/i18n-loader'

type I18nContextValue = {
  lang: string
  ready: boolean
  dictionary: Dictionary
  t: (key: string, fallback?: string) => string
}

type DictionaryPayload = {
  lang: string
  entries: Dictionary
}

type LanguageSettings = {
  lang: string
  fallbackLanguage: string
}

const I18nContext = createContext<I18nContextValue>({
  lang: DefaultLanguage,
  ready: false,
  dictionary: {},
  t: (key, fallback) => fallback ?? key,
})

let activeTranslate: (key: string, fallback?: string) => string = (key, fallback) => fallback ?? key

// 桥之外的地方也能取词条，比如动作结果提示
export function translateKey(key: string, fallback?: string): string {
  return activeTranslate(key, fallback)
}

// 语言以宿主为准，宿主不可用时读配置，最后才退回默认语言
async function resolveLanguage(): Promise<LanguageSettings> {
  if (isBridgeAvailable()) {
    try {
      const info = await call<Partial<LanguageSettings>>('menu.info')
      if (info?.lang) {
        return { lang: info.lang, fallbackLanguage: info.fallbackLanguage ?? DefaultLanguage }
      }
    } catch {
      // 桥不可用时继续往下找
    }
  }
  const config = await fetchJson<{ menu?: { language?: string; fallbackLanguage?: string } }>('./config.json')
  return {
    lang: config?.menu?.language ?? DefaultLanguage,
    fallbackLanguage: config?.menu?.fallbackLanguage ?? DefaultLanguage,
  }
}

export function I18nProvider({ children }: { children: React.ReactNode }) {
  const [lang, setLang] = useState(DefaultLanguage)
  const [entries, setEntries] = useState<Dictionary>({})
  const [ready, setReady] = useState(false)
  const mounted = useRef(true)
  const generation = useRef(0)

  const load = useCallback(async (requested?: string) => {
    const request = ++generation.current
    const settings = await resolveLanguage()
    const target = requested ?? settings.lang
    const [local, remote] = await Promise.all([
      loadDictionary(target, settings.fallbackLanguage),
      isBridgeAvailable() ? call<DictionaryPayload>('i18n.dictionary').catch(() => null) : null,
    ])
    const dictionary = {
      ...local,
      ...validEntries(remote?.lang === target ? remote.entries : null),
    }

    if (!mounted.current || request !== generation.current) {
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
    return { lang, ready, dictionary: entries, t: translate }
  }, [lang, ready, entries])

  return <I18nContext.Provider value={value}>{children}</I18nContext.Provider>
}

export function useI18n(): I18nContextValue {
  return useContext(I18nContext)
}
