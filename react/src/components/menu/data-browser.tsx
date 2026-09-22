import { useEffect, useMemo, useState } from 'react'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { useI18n } from '@/lib/i18n'
import { call } from '@/lib/bridge'

export type DataItem = {
  category: string
  name: string
  id?: number
  isModel?: boolean
  modelId?: number
  x?: number
  y?: number
  z?: number
  interior?: number
}

type DataBrowserProps = {
  method: string
  disabled?: boolean
  max?: number
  onPick: (item: DataItem) => void
}

const MaxVisible = 120

// 数据包列表统一走这里，分类筛选加关键字过滤，避免一次渲染上千个条目
export function DataBrowser({ method, disabled, max = MaxVisible, onPick }: DataBrowserProps) {
  const { t } = useI18n()
  const [items, setItems] = useState<DataItem[]>([])
  const [category, setCategory] = useState('')
  const [keyword, setKeyword] = useState('')
  const [loaded, setLoaded] = useState(false)

  useEffect(() => {
    let cancelled = false
    call<{ items: DataItem[] }>(method)
      .then((payload) => {
        if (cancelled) return
        setItems(payload.items ?? [])
        setLoaded(true)
      })
      .catch(() => {
        if (!cancelled) setLoaded(true)
      })
    return () => {
      cancelled = true
    }
  }, [method])

  const categories = useMemo(() => {
    const unique = new Set<string>()
    for (const item of items) {
      if (item.category) unique.add(item.category)
    }
    return [...unique]
  }, [items])

  const visible = useMemo(() => {
    const needle = keyword.trim().toLowerCase()
    return items
      .filter((item) => (category ? item.category === category : true))
      .filter((item) => {
        if (!needle) return true
        const label = `${t(item.name)} ${t(item.category)}`.toLowerCase()
        return label.includes(needle) || String(item.id ?? '').includes(needle)
      })
      .slice(0, max)
  }, [items, category, keyword, max, t])

  return (
    <div className="flex flex-col gap-3">
      <div className="flex flex-wrap items-center gap-2">
        <Input
          className="h-8 w-48"
          value={keyword}
          onChange={(event) => setKeyword(event.target.value)}
          placeholder={t('common.search')}
        />
        <Button variant={category ? 'outline' : 'default'} size="sm" onClick={() => setCategory('')}>
          {t('common.toggles')}
        </Button>
        {categories.slice(0, 8).map((value) => (
          <Button
            key={value}
            variant={category === value ? 'default' : 'outline'}
            size="sm"
            onClick={() => setCategory(value)}
          >
            {t(value)}
          </Button>
        ))}
      </div>
      <div className="grid max-h-72 gap-2 overflow-y-auto md:grid-cols-2">
        {visible.map((item) => (
          <Button
            key={`${item.category}-${item.name}-${item.id ?? ''}`}
            variant="outline"
            size="sm"
            disabled={disabled}
            className="justify-start truncate"
            onClick={() => onPick(item)}
          >
            {t(item.name)}
            {item.id !== undefined ? <span className="ml-2 opacity-60">{item.id}</span> : null}
          </Button>
        ))}
        {loaded && visible.length === 0 ? (
          <div className="text-xs text-muted-foreground">{t('ped.noListData')}</div>
        ) : null}
      </div>
    </div>
  )
}
