import { useState } from 'react'
import { Switch } from '@/components/ui/switch'
import { runActionQuiet } from '@/lib/actions'
import { isUsable, type CapabilityReport } from '@/lib/bridge'
import { useI18n } from '@/lib/i18n'

export type ToggleItem = {
  // ParamToggles 由外层统一给方法名，单项允许省略
  method?: string
  label: string
  // 参数名与词条键不同时使用，例如 bulletAssist.config 的 tracking
  key?: string
  // 后端默认开启时给出，避免初始显示与实际状态相反
  defaultChecked?: boolean
}

type ToggleGridProps = {
  report: CapabilityReport | null
  items: ToggleItem[]
}

// 开关网格，按能力表逐项禁用，各机型可见的开关自然不同
export function ToggleGrid({ report, items }: ToggleGridProps) {
  const { t } = useI18n()
  const [state, setState] = useState<Record<string, boolean>>({})
  if (items.length === 0) {
    return <div className="text-xs text-muted-foreground">{t('ped.noListData')}</div>
  }

  return (
    <div className="grid gap-3 md:grid-cols-2">
      {items.map((item) => (
        <label
          key={item.method ?? item.label}
          className="flex items-center justify-between gap-3 text-sm"
          title={report && !isUsable(report, item.method ?? '') ? t('react.unsupportedHint') : undefined}
        >
          <span className="min-w-0 truncate">{t(item.label)}</span>
          <Switch
            checked={state[item.method ?? item.label] ?? item.defaultChecked ?? false}
            disabled={report ? !isUsable(report, item.method ?? '') : false}
            onCheckedChange={(checked) => {
              setState((previous) => ({ ...previous, [item.method ?? item.label]: checked }))
              void runActionQuiet(item.method ?? '', { enable: checked })
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
        <label
          key={item.label}
          className="flex items-center justify-between gap-3 text-sm"
          title={report && !isUsable(report, method) ? t('react.unsupportedHint') : undefined}
        >
          <span className="min-w-0 truncate">{t(item.label)}</span>
          <Switch
            checked={state[item.key ?? item.label] ?? item.defaultChecked ?? false}
            disabled={report ? !isUsable(report, method) : false}
            onCheckedChange={(checked) => {
              const name = item.key ?? item.label
              setState((previous) => ({ ...previous, [name]: checked }))
              void runActionQuiet(method, { [name]: checked })
            }}
          />
        </label>
      ))}
    </div>
  )
}
