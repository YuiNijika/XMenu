import { useMemo, useState } from 'react'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { useI18n } from '@/lib/i18n'

type KeyBrowserProps = {
  prefix: string
  game?: string
  disabled?: boolean
  max?: number
  onPick: (segments: string[], label: string) => void
}

const MaxVisible = 400

// 词条键里已经列好了游戏内的动画、过场与粒子清单，这里直接当数据源用
export function KeyBrowser({ prefix, game, disabled, max = MaxVisible, onPick }: KeyBrowserProps) {
  const { t, dictionary } = useI18n()
  const [group, setGroup] = useState('')
  const [keyword, setKeyword] = useState('')

  const items = useMemo(() => {
    const base = prefix.endsWith('.') ? prefix : `${prefix}.`
    const list: { segments: string[]; label: string; group: string }[] = []
    for (const [key, value] of Object.entries(dictionary)) {
      if (!key.startsWith(base)) continue
      const segments = key.split('.')
      if (game && segments[2] !== game) continue
      list.push({
        segments,
        label: value,
        group: segments[3] ?? '',
      })
    }
    return list
  }, [dictionary, prefix, game])

  const groups = useMemo(() => {
    const unique = new Set<string>()
    for (const item of items) {
      if (item.group) unique.add(item.group)
    }
    return [...unique].sort()
  }, [items])

  const visible = useMemo(() => {
    const needle = keyword.trim().toLowerCase()
    return items
      .filter((item) => (group ? item.group === group : true))
      .filter((item) => {
        if (!needle) return true
        return `${item.label} ${item.segments.join(' ')}`.toLowerCase().includes(needle)
      })
      .slice(0, max)
  }, [items, group, keyword, max])

  return (
    <div className="flex flex-col gap-3">
      <div className="flex flex-wrap items-center gap-2">
        <Input
          className="h-8 w-44"
          value={keyword}
          onChange={(event) => setKeyword(event.target.value)}
          placeholder={t('react.search')}
          aria-label={t('react.search')}
        />
        <Button variant={group ? 'outline' : 'default'} size="sm" onClick={() => setGroup('')}>
          {t('common.toggles')}
        </Button>
        {groups.slice(0, 10).map((value) => (
          <Button key={value} variant={group === value ? 'default' : 'outline'} size="sm" onClick={() => setGroup(value)}>
            {value}
          </Button>
        ))}
      </div>
      <div className="grid max-h-64 gap-2 overflow-y-auto md:grid-cols-3">
        {visible.map((item) => (
          <Button
            key={item.segments.join('.')}
            variant="outline"
            size="sm"
            disabled={disabled}
            className="justify-start truncate"
            onClick={() => onPick(item.segments, item.label)}
          >
            {item.label}
          </Button>
        ))}
        {visible.length === 0 ? <div className="text-xs text-muted-foreground">{t('scene.noListData')}</div> : null}
      </div>
    </div>
  )
}
