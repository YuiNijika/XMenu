export type Dictionary = Record<string, string>

interface LanguageIndex {
    fallback?: string
    files?: string[]
}

export const DefaultLanguage = 'zh'
const DataRoot = './data/i18n'

export async function fetchJson<T>(path: string): Promise<T | null> {
    try {
        const response = await fetch(path, { cache: 'no-cache' })
        return response.ok ? await response.json() as T : null
    } catch {
        return null
    }
}

export function validEntries(value: unknown): Dictionary {
    const entries: Dictionary = {}
    if (!value || typeof value !== 'object' || Array.isArray(value)) return entries
    for (const [key, text] of Object.entries(value)) {
        if (typeof text === 'string' && text.trim() && text !== key) entries[key] = text
    }
    return entries
}

export async function loadDictionary(lang: string, fallbackLanguage = DefaultLanguage): Promise<Dictionary> {
    const loaded = new Map<string, Promise<{ entries: Dictionary; fallback?: string }>>()
    const read = (code: string) => {
        let result = loaded.get(code)
        if (!result) {
            result = (async () => {
                if (!/^[a-z0-9_-]+$/i.test(code)) return { entries: {} }
                const index = await fetchJson<LanguageIndex>(`${DataRoot}/${code}/index.json`)
                const files = Array.isArray(index?.files) ? index.files.filter((file) =>
                    typeof file === 'string' && /^[a-z0-9_.-]+\.json$/i.test(file)) : []
                const parts = await Promise.all(files.map((file) => fetchJson<unknown>(`${DataRoot}/${code}/${file}`)))
                return {
                    entries: Object.assign({}, ...parts.map(validEntries)) as Dictionary,
                    fallback: typeof index?.fallback === 'string' ? index.fallback : undefined,
                }
            })()
            loaded.set(code, result)
        }
        return result
    }
    const chain = async (code: string, visited = new Set<string>()): Promise<Dictionary> => {
        if (!code || visited.has(code)) return {}
        const next = new Set(visited).add(code)
        const language = await read(code)
        const fallback = language.fallback ? await chain(language.fallback, next) : {}
        return { ...fallback, ...language.entries }
    }
    const [defaults, requested, configured, current] = await Promise.all([
        chain(DefaultLanguage), chain(lang), chain(fallbackLanguage), read(lang),
    ])
    return { ...defaults, ...requested, ...configured, ...current.entries }
}
