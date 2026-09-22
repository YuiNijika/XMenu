import { useState } from 'react'
import { Switch } from '@/components/ui/switch'
import { runActionQuiet } from '@/lib/actions'
import { isUsable, type CapabilityReport } from '@/lib/bridge'
import { useI18n } from '@/lib/i18n'

export type ToggleItem = {
  method: string
  label: string
}

type ToggleGridProps = {
  report: CapabilityReport | null
  items: ToggleItem[]
}

// 开关网格，按能力表逐项禁用，各机型可见的开关自然不同
export function ToggleGrid({ report, items }: ToggleGridProps) {
  const { t } = useI18n()
  const [state, setState] = useState<Record<string, boolean>>({})

  return (
    <div className="grid gap-3 md:grid-cols-2">
      {items.map((item) => (
        <label key={item.method} className="flex items-center justify-between gap-3 text-sm">
          <span className="min-w-0 truncate">{t(item.label)}</span>
          <Switch
            checked={state[item.method] ?? false}
            disabled={!isUsable(report, item.method)}
            onCheckedChange={(checked) => {
              setState((previous) => ({ ...previous, [item.method]: checked }))
              void runActionQuiet(item.method, { enable: checked })
            }}
          />
        </label>
      ))}
    </div>
  )
}

type ParamTogglesProps = {
  report: CapabilityReport | null
  method: string
  items: ToggleItem[]
}

// 同一方法承载多个布尔参数时使用，每次只提交被改动的那个键
export function ParamToggles({ report, method, items }: ParamTogglesProps) {
  const { t } = useI18n()
  const [state, setState] = useState<Record<string, boolean>>({})

  return (
    <div className="grid gap-3 md:grid-cols-2">
      {items.map((item) => (
        <label key={item.label} className="flex items-center justify-between gap-3 text-sm">
          <span className="min-w-0 truncate">{t(item.label)}</span>
          <Switch
            checked={state[item.label] ?? false}
            disabled={!isUsable(report, method)}
            onCheckedChange={(checked) => {
              setState((previous) => ({ ...previous, [item.label]: checked }))
              void runActionQuiet(method, { [item.label]: checked })
            }}
          />
        </label>
      ))}
    </div>
  )
}
