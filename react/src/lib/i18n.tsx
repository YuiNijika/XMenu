import { createContext, useCallback, useContext, useEffect, useMemo, useState } from 'react'
import { call, isBridgeAvailable, on } from '@/lib/bridge'

type Dictionary = Record<string, string>

type I18nContextValue = {
  lang: string
  ready: boolean
  t: (key: string, fallback?: string) => string
}

const I18nContext = createContext<I18nContextValue>({
  lang: 'zh',
  ready: false,
  t: (key, fallback) => fallback ?? key,
})

type DictionaryPayload = {
  lang: string
  entries: Dictionary
}

// 词条来自 XMenu 的语言包，与 ImGui 界面完全共用，不额外维护一份
export function I18nProvider({ children }: { children: React.ReactNode }) {
  const [lang, setLang] = useState('zh')
  const [entries, setEntries] = useState<Dictionary>({})
  const [ready, setReady] = useState(false)

  const load = useCallback(() => {
    if (!isBridgeAvailable()) {
      setReady(true)
      return
    }
    call<DictionaryPayload>('i18n.dictionary')
      .then((payload) => {
        setLang(payload.lang)
        setEntries(payload.entries ?? {})
        setReady(true)
      })
      .catch(() => setReady(true))
  }, [])

  useEffect(() => {
    load()
    on('i18n.changed', () => load())
  }, [load])

  const value = useMemo<I18nContextValue>(
    () => ({
      lang,
      ready,
      t: (key, fallback) => entries[key] ?? fallback ?? key,
    }),
    [lang, ready, entries],
  )

  return <I18nContext.Provider value={value}>{children}</I18nContext.Provider>
}

export function useI18n(): I18nContextValue {
  return useContext(I18nContext)
}
