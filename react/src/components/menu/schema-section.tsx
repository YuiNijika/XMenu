import { useEffect, useState } from 'react'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { Separator } from '@/components/ui/separator'
import { Switch } from '@/components/ui/switch'
import { runActionQuiet } from '@/lib/actions'
import {
  call,
  type CapabilityReport,
  type UiControl,
  type UiSchemaPayload,
  type UiSelectOption,
} from '@/lib/bridge'
import { useI18n } from '@/lib/i18n'

type SchemaSectionProps = {
  payload: UiSchemaPayload
  report: CapabilityReport | null
  tabId: string
  pageId: string
  sectionId: string
  // 只对当前载具之类的对象生效的分区，由页面判断是否该显示
  visible?: boolean
}

type ControlValue = boolean | number | string

function capabilityOk(payload: UiSchemaPayload, name?: string): boolean {
  if (!name) return true
  const state = payload.capabilities?.[name]
  return state === 'supported' || state === 'partial'
}

function gamesOk(games: string[] | undefined, game: string | undefined): boolean {
  if (!games || games.length === 0) return true
  return game ? games.includes(game) : false
}

// 按界面注册表绘制一个分区。能力表由宿主随注册表一起返回，
// 因此这里的门控与 ImGui 用的是同一份判断，不会出现两边不一致
export function SchemaSection({ payload, report, tabId, pageId, sectionId, visible = true }: SchemaSectionProps) {
  const { t } = useI18n()
  const tab = payload.schema.tabs.find((item) => item.id === tabId)
  const page = tab?.pages?.find((item) => item.id === pageId)
  const section = page?.sections?.find((item) => item.id === sectionId)

  const [values, setValues] = useState<Record<string, ControlValue>>({})
  const [options, setOptions] = useState<Record<string, UiSelectOption[]>>({})

  const usable = section ? capabilityOk(payload, section.capability) : true
  const controls = section
    ? (section.controls ?? []).filter(
        (control) => gamesOk(control.games, report?.game) && capabilityOk(payload, control.capability),
      )
    : []

  const stateKeys = controls
    .filter((control) => control.kind !== 'action')
    .map((control) => control.id)
    .join('|')

  // 初始值向宿主取一次，避免开关显示与真实状态相反
  useEffect(() => {
    if (!stateKeys) return
    let alive = true
    const pending = stateKeys.split('|').map(async (id) => {
      try {
        // 下拉连同选项一起取，省一次往返
        const select = controls.find((item) => item.id === id && item.kind === 'select')
        if (select) {
          const result = await call<{ value?: unknown; items?: UiSelectOption[] }>('ui.options', { id })
          if (result?.items) {
            setOptions((previous) => ({ ...previous, [id]: result.items ?? [] }))
          }
          return [id, result?.value] as const
        }
        const result = await call<{ value?: unknown }>('ui.get', { id })
        return [id, result?.value] as const
      } catch {
        return [id, undefined] as const
      }
    })
    void Promise.all(pending).then((entries) => {
      if (!alive) return
      const next: Record<string, ControlValue> = {}
      for (const [id, value] of entries) {
        if (typeof value === 'boolean' || typeof value === 'number' || typeof value === 'string') {
          next[id] = value
        }
      }
      setValues(next)
    })
    return () => {
      alive = false
    }
  }, [stateKeys])

  if (!visible || !section || !gamesOk(section.games, report?.game) || controls.length === 0) return null

  // 依赖另一项的显隐，取不到被依赖项的值时先显示，避免加载期间控件闪进闪出
  const shown = controls.filter((control) => {
    const expression = control.visibleWhen
    if (!expression) return true
    // 前面带叹号表示反过来，例如「不用列表界面时才显示交互模式」
    const inverted = expression.startsWith('!')
    const dependsOn = inverted ? expression.slice(1) : expression
    const current = values[dependsOn]
    if (current === undefined) return true
    const on = current === true
    return inverted ? !on : on
  })

  const columns = section.columns ?? 1
  const inline = Boolean(section.inline) && columns > 1

  const update = (id: string, next: ControlValue) => {
    setValues((previous) => ({ ...previous, [id]: next }))
  }

  const rows = shown.map((control) => (
    <ControlRow
      key={control.id}
      control={control}
      disabled={!usable}
      value={values[control.id]}
      options={options[control.id]}
      className={inline ? 'flex-none' : undefined}
      onValue={(next) => update(control.id, next)}
    />
  ))

  return (
    <div className="grid gap-3">
      <Separator />
      <div className="text-sm font-medium">{t(section.labelKey)}</div>
      {section.hintKey ? <p className="text-xs text-muted-foreground">{t(section.hintKey)}</p> : null}
      {inline ? (
        <div className="flex flex-wrap gap-2">{rows}</div>
      ) : (
        <div
          className="grid gap-3"
          style={{ gridTemplateColumns: `repeat(${columns}, minmax(0, 1fr))` }}
        >
          {rows}
        </div>
      )}
    </div>
  )
}

function ControlRow({
  control,
  disabled,
  value,
  options,
  className,
  onValue,
}: {
  control: UiControl
  disabled: boolean
  value?: ControlValue
  options?: UiSelectOption[]
  className?: string
  onValue: (next: ControlValue) => void
}) {
  const { t } = useI18n()
  const [text, setText] = useState(value === undefined ? '' : String(value))

  useEffect(() => {
    if (value !== undefined) setText(String(value))
  }, [value])

  if (control.kind === 'toggle') {
    return (
      <label className="flex items-center justify-between gap-3 text-sm">
        <span className="min-w-0 truncate">{t(control.labelKey)}</span>
        <Switch
          checked={value === true}
          disabled={disabled}
          onCheckedChange={(next) => {
            onValue(next)
            void runActionQuiet('ui.set', { id: control.id, value: next })
          }}
        />
      </label>
    )
  }

  if (control.kind === 'select') {
    return (
      <label className="flex items-center justify-between gap-3 text-sm">
        <span className="min-w-0 truncate">{t(control.labelKey)}</span>
        <select
          className="h-8 min-w-0 max-w-[60%] rounded-md border border-input bg-transparent px-2 text-sm"
          value={value === undefined ? '' : String(value)}
          disabled={disabled}
          onChange={(event) => {
            const next = event.target.value
            onValue(next)
            void runActionQuiet('ui.set', { id: control.id, value: next })
          }}
        >
          {(options ?? []).map((option) => (
            <option key={option.value} value={option.value}>
              {option.translated ? t(option.label) : option.label}
            </option>
          ))}
        </select>
      </label>
    )
  }

  if (control.kind === 'float' || control.kind === 'int') {
    return (
      <div className="flex items-center gap-2 text-sm">
        <span className="min-w-0 flex-1 truncate">{t(control.labelKey)}</span>
        <Input
          className="w-20"
          inputMode="numeric"
          value={text}
          disabled={disabled}
          onChange={(event) => setText(event.target.value)}
        />
        <Button
          variant="outline"
          disabled={disabled}
          onClick={() => {
            const next = Number(text)
            if (Number.isNaN(next)) return
            onValue(next)
            void runActionQuiet('ui.set', { id: control.id, value: next })
          }}
        >
          {t('react.apply')}
        </Button>
      </div>
    )
  }

  return (
    <Button
      variant="outline"
      className={className}
      disabled={disabled}
      onClick={() => void runActionQuiet('ui.run', { id: control.id })}
    >
      {t(control.labelKey)}
    </Button>
  )
}
