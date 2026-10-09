import { useEffect, useRef, useState } from 'react'
import { ArrowLeft, ArrowRight, Download, ExternalLink, Eye, Heart, ImageOff, LoaderCircle, MessageCircle, RefreshCw } from 'lucide-react'
import { Button } from '@/components/ui/button'
import { NativeSelect, NativeSelectOption } from '@/components/ui/native-select'
import { call } from '@/lib/bridge'
import { useI18n } from '@/lib/i18n'

interface ModItem {
  id: string
  short_id: string
  slug: string | null
  title: string
  summary: string | null
  cover_image: string | null
  author: { name: string } | null
  original_author: string | null
  reposted: boolean
  category: { name: string; slug: string } | null
  game: string | null
  downloads: number
  views: number
  like_count: number
  comments_count: number
}

interface CatalogState {
  items: ModItem[]
  pagination: { page: number; limit: number; total: number; total_pages: number }
  loading: boolean
  requested: boolean
  error: string
  gameSlug: string
  type: string
  requestedPage: number
  requestedLimit: number
}

const CATEGORY_OPTIONS = [
  ['plugins & scripts', 'mods.categoryPlugins'],
  ['vehicles', 'mods.categoryVehicles'],
  ['peds', 'mods.categoryPeds'],
  ['maps', 'mods.categoryMaps'],
  ['graphics', 'mods.categoryGraphics'],
  ['tools', 'mods.categoryTools'],
] as const

function safeCover(value: string | null): string | undefined {
  if (!value) return undefined
  try {
    const url = new URL(value)
    return url.protocol === 'https:' ? url.href : undefined
  } catch {
    return undefined
  }
}

export function ModCatalog() {
  const { t, lang } = useI18n()
  const [state, setState] = useState<CatalogState | null>(null)
  const [error, setError] = useState('')
  const [type, setType] = useState('')
  const [limit, setLimit] = useState(12)
  const [submitting, setSubmitting] = useState(false)
  const alive = useRef(false)
  const initialized = useRef(false)
  const pending = useRef(false)

  useEffect(() => {
    alive.current = true
    let cancelled = false
    let timer: ReturnType<typeof setTimeout>
    const poll = async () => {
      try {
        let next = await call<CatalogState>('mods.snapshot')
        if (!next.requested) next = await call<CatalogState>('mods.request', { page: 1, limit: 12 })
        if (cancelled) return
        setState(next)
        setError('')
        if (!initialized.current) {
          initialized.current = true
          setType(next.type)
          setLimit(next.requestedLimit)
        }
      } catch {
        if (!cancelled) setError('mods.failed')
      } finally {
        if (!cancelled) timer = setTimeout(poll, 800)
      }
    }
    void poll()
    return () => {
      alive.current = false
      cancelled = true
      clearTimeout(timer)
    }
  }, [])

  const request = async (page: number, nextLimit = limit, nextType = type) => {
    if (pending.current) return
    pending.current = true
    setSubmitting(true)
    setError('')
    try {
      const next = await call<CatalogState>('mods.request', { page, limit: nextLimit, type: nextType })
      if (alive.current) setState(next)
    } catch {
      if (alive.current) setError('mods.failed')
    } finally {
      pending.current = false
      if (alive.current) setSubmitting(false)
    }
  }
  const open = async (method: string, id?: string) => {
    try {
      const result = await call<{ ok: boolean }>(method, id ? { id } : {})
      if (!result.ok) throw new Error('Open failed')
    } catch {
      if (alive.current) setError('mods.openFailed')
    }
  }
  const busy = submitting || !state || state.loading
  const failure = error || state?.error
  const format = new Intl.NumberFormat(({ zh: 'zh-CN', jp: 'ja', en: 'en', ru: 'ru' } as Record<string, string>)[lang] ?? 'en')
  const count = (value: number) => format.format(value)

  return (
    <section aria-label={t('tab.mods')} className="min-w-0" aria-busy={busy}>
      <form
        className="mb-5 flex flex-wrap items-end gap-2.5 border-b border-border/60 pb-4"
        onSubmit={(event) => { event.preventDefault(); void request(1) }}
      >
        <label className="grid min-w-0 gap-1.5 text-xs text-muted-foreground">
          {t('mods.category')}
          <NativeSelect
            value={type}
            disabled={busy}
            onChange={(event) => { setType(event.target.value); void request(1, limit, event.target.value) }}
          >
            <NativeSelectOption value="">{t('mods.allCategories')}</NativeSelectOption>
            {CATEGORY_OPTIONS.map(([value, label]) => (
              <NativeSelectOption key={value} value={value}>{t(label)}</NativeSelectOption>
            ))}
          </NativeSelect>
        </label>
        <label className="grid gap-1.5 text-xs text-muted-foreground">
          {t('mods.pageSize')}
          <NativeSelect
            value={limit}
            disabled={busy}
            onChange={(event) => { const value = Number(event.target.value); setLimit(value); void request(1, value) }}
          >
            {[6, 12, 20].map((value) => <NativeSelectOption key={value} value={value}>{value}</NativeSelectOption>)}
          </NativeSelect>
        </label>
        <Button type="submit" variant="secondary" disabled={busy}>{t('mods.filter')}</Button>
        <Button
          type="button" variant="outline" size="icon" disabled={busy}
          title={t('mods.refresh')} aria-label={t('mods.refresh')}
          onClick={() => void request(state?.requestedPage ?? 1, state?.requestedLimit ?? limit, state?.type ?? type)}
        >
          <RefreshCw aria-hidden="true" />
        </Button>
        <Button type="button" variant="outline" onClick={() => void open('mods.game')}>
          <ExternalLink aria-hidden="true" />{t('mods.gamePage')}
        </Button>
      </form>

      <div className="mb-4 flex flex-wrap items-center justify-between gap-3">
        <span role="status" aria-live="polite" className="flex items-center gap-2 text-sm text-muted-foreground">
          {busy ? <><LoaderCircle className="size-4 animate-spin motion-reduce:animate-none" aria-hidden="true" />{t('mods.loading')}</>
            : state ? `${count(state.pagination.total)} ${t('mods.works')}` : null}
        </span>
        {state ? (
          <nav aria-label={t('mods.paginationLabel')} className="flex items-center gap-2">
            <Button variant="outline" size="icon" disabled={busy || state.requestedPage <= 1}
              title={t('mods.previous')} aria-label={t('mods.previous')}
              onClick={() => void request(state.requestedPage - 1, state.requestedLimit, state.type)}>
              <ArrowLeft aria-hidden="true" />
            </Button>
            <span className="min-w-14 text-center text-sm tabular-nums">
              {state.pagination.total_pages ? state.pagination.page : 0} / {state.pagination.total_pages}
            </span>
            <Button variant="outline" size="icon" disabled={busy || state.requestedPage >= state.pagination.total_pages}
              title={t('mods.next')} aria-label={t('mods.next')}
              onClick={() => void request(state.requestedPage + 1, state.requestedLimit, state.type)}>
              <ArrowRight aria-hidden="true" />
            </Button>
          </nav>
        ) : null}
      </div>
      {failure ? (
        <div role="alert" className="mb-4 flex flex-wrap items-center gap-3 border-l-2 border-destructive py-2 pl-3 text-sm">
          <span>{t(failure)}</span>
          <Button variant="outline" disabled={busy} onClick={() => void request(state?.requestedPage ?? 1)}>{t('mods.retry')}</Button>
        </div>
      ) : null}
      {!busy && !failure && !state?.items.length ? <p role="status" className="py-12 text-center text-sm text-muted-foreground">{t('mods.empty')}</p> : null}
      <div className="grid min-w-0 grid-cols-1 gap-3 sm:grid-cols-2 md:grid-cols-3 lg:grid-cols-4 2xl:grid-cols-5">
        {busy && !state?.items.length ? Array.from({ length: 10 }, (_, index) => <SkeletonCard key={`skeleton-${index}`} />) : null}
        {(state?.items ?? []).map((item) => (
          <article key={item.id} data-mod-card className="flex min-w-0 flex-col overflow-hidden rounded-md border border-border bg-card">
            <Cover item={item} category={item.category?.name ?? t('mods.uncategorized')} reposted={item.reposted ? t('mods.reposted') : ''} />
            <div className="flex min-w-0 flex-1 flex-col gap-1.5 p-2.5">
              <div className="flex min-w-0 items-center justify-between gap-2 text-[11px] leading-4 text-muted-foreground">
                <span className="break-words">{item.category?.name ?? t('mods.uncategorized')}</span>
                {item.reposted ? <span className="shrink-0">{t('mods.reposted')}</span> : null}
              </div>
              <h2 className="line-clamp-1 break-words text-sm font-semibold leading-5 [overflow-wrap:anywhere]" title={item.title}>{item.title}</h2>
              <p className="line-clamp-1 min-h-5 break-words text-xs leading-5 text-muted-foreground">{item.summary ?? ''}</p>
              <p className="line-clamp-1 break-words text-[11px] leading-4 text-muted-foreground">
                {(item.reposted ? item.original_author : item.author?.name) || t('mods.unknownAuthor')}
              </p>
              <dl className="mt-auto flex flex-wrap gap-x-2 gap-y-1 pt-1 text-[11px] tabular-nums text-muted-foreground">
                {[[Download, 'mods.downloads', item.downloads], [Eye, 'mods.views', item.views],
                  [Heart, 'mods.likes', item.like_count], [MessageCircle, 'mods.comments', item.comments_count]].map(([Icon, key, value]) => {
                    const StatIcon = Icon as typeof Download
                    return <div key={key as string} title={t(key as string)} className="flex items-center gap-1">
                      <dt><StatIcon className="size-3.5" aria-hidden="true" /><span className="sr-only">{t(key as string)}</span></dt>
                      <dd>{count(value as number)}</dd>
                    </div>
                  })}
              </dl>
              <Button variant="outline" size="sm" className="mt-1 h-8 w-full text-xs" disabled={!item.slug && !item.short_id} onClick={() => void open('mods.open', item.id)}>
                <Download aria-hidden="true" />{t('mods.download')}<ExternalLink className="ml-auto size-3.5" aria-hidden="true" />
              </Button>
            </div>
          </article>
        ))}
      </div>
    </section>
  )
}

function Cover({ item, category, reposted }: { item: ModItem; category: string; reposted: string }) {
  const [failed, setFailed] = useState(false)
  const url = safeCover(item.cover_image)
  return (
    <div className="relative flex aspect-video w-full items-center justify-center overflow-hidden border-b border-border bg-muted">
      {url && !failed ? <img src={url} alt={item.title} width={640} height={360} loading="lazy" referrerPolicy="no-referrer"
        onError={() => setFailed(true)} className="h-full w-full object-cover" />
        : <ImageOff className="size-7 text-muted-foreground" aria-hidden="true" />}
      <div className="pointer-events-none absolute inset-x-2 top-2 flex items-start justify-between gap-2">
        <span className="max-w-[75%] truncate rounded bg-black/70 px-2 py-0.5 text-[10px] font-semibold leading-4 text-white backdrop-blur-sm" title={category}>{category}</span>
        {reposted ? <span className="shrink-0 rounded bg-black/70 px-2 py-0.5 text-[10px] font-semibold leading-4 text-white backdrop-blur-sm">{reposted}</span> : null}
      </div>
    </div>
  )
}

function SkeletonCard() {
  return (
    <article data-mod-card aria-hidden="true" className="flex min-w-0 flex-col overflow-hidden rounded-md border border-border bg-card">
      <div className="aspect-video w-full animate-pulse bg-muted" />
      <div className="flex min-w-0 flex-1 flex-col gap-2 p-2.5">
        <div className="h-3 w-1/3 animate-pulse rounded-sm bg-muted" />
        <div className="h-4 w-5/6 animate-pulse rounded-sm bg-muted" />
        <div className="h-3 w-2/3 animate-pulse rounded-sm bg-muted" />
        <div className="mt-auto h-3 w-1/2 animate-pulse rounded-sm bg-muted" />
      </div>
    </article>
  )
}
