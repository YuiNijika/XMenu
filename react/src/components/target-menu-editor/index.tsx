import { useEffect, useId, useRef, useState } from 'react'
import { ArrowDown, ArrowDownLeft, ArrowDownRight, ArrowUp, ArrowUpLeft, ArrowUpRight, Pencil, Trash2 } from 'lucide-react'
import { Input } from '@/components/ui/input'
import { Switch } from '@/components/ui/switch'
import { ContextMenu, ContextMenuTrigger, ContextMenuContent, ContextMenuItem } from '@/components/ui/context-menu'
import { Dialog, DialogContent, DialogTitle } from '@/components/ui/dialog'
import { useI18n } from '@/lib/i18n'
import type { TargetActionInfo, TargetBinding } from '@/lib/bridge'

interface Props {
  kind: 'ped' | 'vehicle'
  menu: TargetBinding[]
  actions: TargetActionInfo[]
  colorChannels: number
  onChange: (kind: 'ped' | 'vehicle', slot: number, binding: Partial<TargetBinding>) => Promise<void>
  onSwap: (menu: TargetBinding[]) => Promise<void>
}

const directions = [
  { key: 'top', Icon: ArrowUp },
  { key: 'topRight', Icon: ArrowUpRight },
  { key: 'bottomRight', Icon: ArrowDownRight },
  { key: 'bottom', Icon: ArrowDown },
  { key: 'bottomLeft', Icon: ArrowDownLeft },
  { key: 'topLeft', Icon: ArrowUpLeft },
] as const

export function TargetMenuEditor({ kind, menu, actions, colorChannels, onChange, onSwap }: Props) {
  const { t } = useI18n()
  const id = useId()
  const [pending, setPending] = useState(false)
  const [error, setError] = useState(false)
  const [localMenu, setLocalMenu] = useState(menu)
  const [draggingSlot, setDraggingSlot] = useState<number | null>(null)
  const [editingSlot, setEditingSlot] = useState<number | null>(null)
  const [dragOffset, setDragOffset] = useState({ x: 0, y: 0 })
  const gesture = useRef<{ x: number; y: number; moved: boolean } | null>(null)
  const wheelRef = useRef<HTMLDivElement | null>(null)
  const busy = useRef(false)

  useEffect(() => {
    if (!busy.current && !gesture.current) setLocalMenu(menu)
  }, [menu])

  const commit = async (slot: number, binding: Partial<TargetBinding>) => {
    if (busy.current) return
    busy.current = true
    setPending(true)
    setError(false)
    try {
      await onChange(kind, slot, binding)
      setLocalMenu((current) => current.map((item, index) => index === slot ? { ...item, ...binding } : item))
    } catch {
      setError(true)
    } finally {
      busy.current = false
      setPending(false)
    }
  }

  const swapSlots = async (from: number, to: number) => {
    if (busy.current || from === to || !localMenu[from] || !localMenu[to]) {
      setDraggingSlot(null)
      return
    }
    const next = [...localMenu]
    const first = next[from]
    next[from] = next[to]
    next[to] = first
    busy.current = true
    setLocalMenu(next)
    setPending(true)
    setError(false)
    try {
      await onSwap(next)
    } catch {
      setLocalMenu(menu)
      setError(true)
    } finally {
      busy.current = false
      setPending(false)
      setDraggingSlot(null)
    }
  }

  const slotAtPoint = (clientX: number, clientY: number): number | null => {
    const rect = wheelRef.current?.getBoundingClientRect()
    if (!rect) return null
    if (clientX < rect.left || clientX > rect.right || clientY < rect.top || clientY > rect.bottom) return null
    const x = clientX - (rect.left + rect.width / 2)
    const y = clientY - (rect.top + rect.height / 2)
    if (Math.hypot(x, y) < rect.width * 0.18) return null
    let angle = Math.atan2(y, x) + Math.PI / 2
    if (angle < 0) angle += Math.PI * 2
    return Math.floor((angle + Math.PI / 6) / (Math.PI / 3)) % 6
  }

  const handlePointerDown = (event: React.PointerEvent<HTMLButtonElement>, slot: number) => {
    if (busy.current || event.button !== 0) return
    event.currentTarget.setPointerCapture(event.pointerId)
    gesture.current = { x: event.clientX, y: event.clientY, moved: false }
    setDragOffset({ x: 0, y: 0 })
    setDraggingSlot(slot)
  }

  const handlePointerUp = (event: React.PointerEvent<HTMLButtonElement>) => {
    const targetSlot = slotAtPoint(event.clientX, event.clientY)
    if (gesture.current?.moved && draggingSlot !== null && targetSlot !== null) void swapSlots(draggingSlot, targetSlot)
    else if (gesture.current && draggingSlot !== null) {
      if (!gesture.current.moved) setEditingSlot(draggingSlot)
      setDraggingSlot(null)
    }
    else setDraggingSlot(null)
    gesture.current = null
  }

  return (
    <section className="min-w-0 space-y-3" aria-labelledby={`${id}-title`} aria-busy={pending}>
      <h3 id={`${id}-title`} className="sr-only">
        {t(kind === 'ped' ? 'targeting.pedMenu' : 'targeting.vehicleMenu')}
      </h3>
      <div
        ref={wheelRef}
        className="relative mx-auto aspect-square w-full max-w-[27rem] rounded-full border border-primary/30 bg-primary/[0.04] p-3"
        aria-label={t('targeting.dragHint')}
      >
        <div className="absolute inset-[27%] flex items-center justify-center rounded-full border border-border/70 bg-background/80 text-center text-xs text-muted-foreground">
          {t(kind === 'ped' ? 'targeting.pedMenu' : 'targeting.vehicleMenu')}
        </div>
        {localMenu.map((binding, slot) => {
          const direction = directions[slot]
          const info = actions.find((action) => action.action === binding.action)
          if (!direction || !info) return null
          const positions = [
            'left-1/2 top-2 -translate-x-1/2',
            'right-2 top-[23%]',
            'right-2 bottom-[23%]',
            'bottom-2 left-1/2 -translate-x-1/2',
            'left-2 bottom-[23%]',
            'left-2 top-[23%]',
          ]
          return (
            <ContextMenu key={`wheel-${slot}`} onOpenChange={(open) => {
              if (open) { setDraggingSlot(null); gesture.current = null }
            }}>
            <ContextMenuTrigger render={<button
              type="button"
              data-target-slot={slot}
              disabled={pending}
              title={t(info.labelKey)}
              style={{ translate: draggingSlot === slot && gesture.current?.moved ? `${dragOffset.x}px ${dragOffset.y}px` : undefined }}
              className={`absolute ${positions[slot]} flex h-[19%] w-[34%] min-w-0 touch-none flex-col items-center justify-center gap-1 rounded-lg border border-border/80 bg-background/95 px-2 py-2 text-center shadow-sm focus-visible:outline-2 focus-visible:outline-ring ${draggingSlot === slot ? 'z-10 border-primary opacity-75' : 'hover:border-primary/70'}`}
              aria-label={`${t(`targeting.slot.${direction.key}`)} · ${t(info.labelKey)}`}
              onPointerDown={(event) => handlePointerDown(event, slot)}
              onPointerMove={(event) => {
                if (!gesture.current) return
                const x = event.clientX - gesture.current.x
                const y = event.clientY - gesture.current.y
                if (Math.hypot(x, y) > 8) gesture.current.moved = true
                if (gesture.current.moved) setDragOffset({ x, y })
              }}
              onPointerUp={handlePointerUp}
              onPointerCancel={() => { setDraggingSlot(null); gesture.current = null }}
              onClick={(event) => { if (event.detail === 0) { setError(false); setEditingSlot(slot) } }}
              />}
            >
              <direction.Icon className="size-4 text-muted-foreground" aria-hidden="true" />
              <span className="max-w-full break-words text-xs font-medium leading-tight">{t(info.labelKey)}</span>
            </ContextMenuTrigger>
            <ContextMenuContent>
              <ContextMenuItem disabled={pending} onClick={() => { setError(false); setEditingSlot(slot) }}>
                <Pencil />{t('targeting.customize')}
              </ContextMenuItem>
              <ContextMenuItem variant="destructive" disabled={pending || binding.action === 0}
                onClick={() => void commit(slot, { action: 0, value: 0, secondary: 0, tertiary: 0, quaternary: 0, enabled: true })}>
                <Trash2 />{t('targeting.removeSlot')}
              </ContextMenuItem>
            </ContextMenuContent>
            </ContextMenu>
          )
        })}
      </div>
      <Dialog open={editingSlot !== null} onOpenChange={(open) => { if (!open && !pending) setEditingSlot(null) }}>
      <DialogContent className="max-h-[85dvh] overflow-y-auto rounded-lg" aria-describedby={undefined}>
        <DialogTitle>{t('targeting.customize')}</DialogTitle>
        {localMenu.map((binding, slot) => {
          if (slot !== editingSlot) return null
          const direction = directions[slot]
          const info = actions.find((action) => action.action === binding.action)
          if (!direction || !info) return null
          const selectId = `${id}-${slot}`
          const numericField = (field: 'value' | 'secondary' | 'tertiary' | 'quaternary', key: string, min: number, max: number) => (
            <label className="grid min-w-0 gap-1 text-xs text-muted-foreground" key={field}>
              <span>{t(key)}</span>
              <Input
                key={`${field}-${binding[field]}`}
                type="number"
                name={`${kind}-${slot}-${field}`}
                autoComplete="off"
                min={min}
                max={max}
                step={1}
                inputMode="numeric"
                defaultValue={binding[field]}
                disabled={pending}
                onBlur={(event) => {
                  const raw = event.currentTarget.value
                  const number = Number(raw)
                  if (raw.trim() === '' || !Number.isFinite(number)) {
                    event.currentTarget.value = String(binding[field])
                    return
                  }
                  const value = Math.round(Math.min(max, Math.max(min, number)))
                  event.currentTarget.value = String(value)
                  if (value !== binding[field]) void commit(slot, { [field]: value })
                }}
                onKeyDown={(event) => {
                  if (event.key === 'Enter') event.currentTarget.blur()
                  if (event.key === 'Escape') {
                    event.currentTarget.value = String(binding[field])
                    event.currentTarget.blur()
                  }
                }}
              />
            </label>
          )
          return (
            <div key={slot} className="min-w-0 space-y-3 p-3">
              <div className="flex min-w-0 items-center gap-2">
                <direction.Icon className="size-4 shrink-0 text-muted-foreground" aria-hidden="true" />
                <span className="w-20 shrink-0 text-xs">
                  {t(`targeting.slot.${direction.key}`)}
                </span>
                <div
                  id={selectId}
                  role="group" aria-label={t('targeting.customize')}
                  className="grid max-h-64 min-w-0 flex-1 grid-cols-2 gap-1 overflow-y-auto"
                >
                  {actions.map((action) => (
                    <button type="button" key={action.action} disabled={pending || !action.supported}
                      aria-pressed={action.action === binding.action}
                      title={!action.supported ? t('targeting.unsupported') : t(action.labelKey)}
                      className={`min-h-10 rounded-md border px-2 py-2 text-left text-xs break-words disabled:opacity-40 focus-visible:outline-2 focus-visible:outline-ring ${action.action === binding.action ? 'border-primary bg-primary/10' : 'border-transparent hover:bg-accent'}`}
                      onClick={() => void commit(slot, {
                      action: action.action,
                      value: action.defaultValue,
                      secondary: action.parameter === 4 ? 100 : 0,
                      tertiary: 0,
                      quaternary: 0,
                      enabled: true,
                    })}>
                      {t(action.labelKey)}{action.supported ? '' : ` (${t('targeting.unsupported')})`}
                    </button>
                  ))}
                </div>
              </div>
              {info.parameter === 2 ? (
                <label className="flex items-center justify-between gap-3 text-xs">
                  <span>{t('targeting.parameter.enabled')}</span>
                  <Switch checked={binding.enabled} disabled={pending} onCheckedChange={(enabled) => void commit(slot, { enabled })} />
                </label>
              ) : info.parameter !== 0 ? (
                <div className="grid grid-cols-2 gap-2">
                  {numericField('value',
                    info.parameter === 3 ? 'targeting.parameter.primary'
                      : info.parameter === 4 ? 'targeting.parameter.weapon'
                      : info.parameter === 5 ? 'targeting.parameter.door'
                      : info.parameter === 6 ? 'targeting.parameter.seat'
                      : 'targeting.parameter.value',
                    info.min, info.max)}
                  {info.parameter === 3 ? numericField('secondary', 'targeting.parameter.secondary', 0, 255) : null}
                  {info.parameter === 3 && colorChannels === 4 ? numericField('tertiary', 'targeting.parameter.tertiary', 0, 255) : null}
                  {info.parameter === 3 && colorChannels === 4 ? numericField('quaternary', 'targeting.parameter.quaternary', 0, 255) : null}
                  {info.parameter === 4 ? numericField('secondary', 'targeting.parameter.ammo', 0, 99999) : null}
                </div>
              ) : null}
            </div>
          )
        })}
      {error ? <p role="alert" className="text-xs text-destructive">{t('targeting.saveFailed')}</p> : null}
      </DialogContent>
      </Dialog>
      {error ? <p role="alert" className="text-xs text-destructive">{t('targeting.saveFailed')}</p> : null}
    </section>
  )
}
