import { useEffect, useState } from 'react'
import { RotateCcw } from 'lucide-react'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { Switch } from '@/components/ui/switch'
import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'
import { Tabs, TabsContent, TabsList, TabsTrigger } from '@/components/ui/tabs'
import { TargetMenuEditor } from '@/components/target-menu-editor'
import { runActionQuiet } from '@/lib/actions'
import { call, type TargetActionsPayload, type TargetBinding, type TargetSnapshot, type TargetSnapshotPayload } from '@/lib/bridge'
import { usePolling } from '@/lib/hooks'
import { useI18n } from '@/lib/i18n'

function targetLabel(target: TargetSnapshot, t: (key: string, fallback?: string) => string) {
  return target.kind === 'vehicle'
    ? t('targeting.vehicle', '车辆')
    : t('targeting.ped', 'NPC')
}

export function TargetingPage() {
  const { t } = useI18n()
  const { value, refresh } = usePolling<TargetSnapshotPayload>('targeting.snapshot', 300)
  const { value: actions } = usePolling<TargetActionsPayload>('targeting.actions', 10000)
  const enabled = value?.enabled ?? false
  const drawLinks = value?.drawLinks ?? true
  const mouseSelect = value?.mouseSelect ?? true
  const includePeds = value?.includePeds ?? true
  const includeVehicles = value?.includeVehicles ?? true
  const [radius, setRadius] = useState('80')
  const [hitRadius, setHitRadius] = useState('160')
  const [maxTargets, setMaxTargets] = useState('16')
  const [menuPending, setMenuPending] = useState(false)
  const [menuError, setMenuError] = useState(false)

  const remoteRadius = value?.radius
  const remoteHitRadius = value?.hitRadius
  const remoteMaxTargets = value?.maxTargets
  useEffect(() => {
    if (typeof remoteRadius === 'number') setRadius(String(Math.round(remoteRadius)))
  }, [remoteRadius])
  useEffect(() => {
    if (typeof remoteHitRadius === 'number') setHitRadius(String(Math.round(remoteHitRadius)))
  }, [remoteHitRadius])
  useEffect(() => {
    if (typeof remoteMaxTargets === 'number') setMaxTargets(String(remoteMaxTargets))
  }, [remoteMaxTargets])

  const targets = Array.isArray(value?.items) ? value.items : []
  const selected = value?.selected ?? targets.find((item) => item.selected)

  const applyConfig = (next: Partial<{
    enabled: boolean
    drawLinks: boolean
    mouseSelect: boolean
    includePeds: boolean
    includeVehicles: boolean
  }>) => {
    void runActionQuiet('targeting.config', next)
  }

  const applyNumberConfig = (key: 'radius' | 'hitRadius' | 'maxTargets', raw: string) => {
    const number = Number(raw)
    if (raw.trim() === '' || !Number.isFinite(number)) return
    void runActionQuiet('targeting.config', { [key]: number })
  }

  const updateMenu = async (kind: 'ped' | 'vehicle', slot: number, binding: Partial<TargetBinding>) => {
    await call('targeting.config', { kind, slot, binding })
    refresh()
  }

  const resetMenu = async () => {
    setMenuPending(true)
    setMenuError(false)
    try {
      await call('targeting.config', { reset: true })
      refresh()
    } catch {
      setMenuError(true)
    } finally {
      setMenuPending(false)
    }
  }

  return (
    <div className="flex flex-col gap-4">
      <Card>
        <CardHeader className="border-b">
          <div className="flex items-start justify-between gap-3">
            <div>
              <CardTitle>{t('targeting.title')}</CardTitle>
              <CardDescription>{t('targeting.hint')}</CardDescription>
            </div>
            <Badge variant={enabled ? 'default' : 'outline'}>
              {enabled
                ? t('targeting.armed', '链路已启用')
                : t('targeting.disarmed', '链路已关闭')}
            </Badge>
          </div>
        </CardHeader>
        <CardContent className="grid gap-4 pt-4">
          <div className="grid gap-2 sm:grid-cols-2">
            <label className="flex items-center justify-between gap-3 rounded-md border border-border/60 px-3 py-2 text-sm">
              <span>{t('targeting.enabled')}</span>
              <Switch checked={enabled} onCheckedChange={(checked) => applyConfig({ enabled: checked })} />
            </label>
            <label className="flex items-center justify-between gap-3 rounded-md border border-border/60 px-3 py-2 text-sm">
              <span>{t('targeting.drawLinks')}</span>
              <Switch checked={drawLinks} onCheckedChange={(checked) => applyConfig({ drawLinks: checked })} />
            </label>
            <label className="flex items-center justify-between gap-3 rounded-md border border-border/60 px-3 py-2 text-sm">
              <span>{t('targeting.mouseSelect', '看门狗式锁定')}</span>
              <Switch checked={mouseSelect} onCheckedChange={(checked) => applyConfig({ mouseSelect: checked })} />
            </label>
            <label className="flex items-center justify-between gap-3 rounded-md border border-border/60 px-3 py-2 text-sm">
              <span>{t('targeting.vehicles')}</span>
              <Switch checked={includeVehicles} onCheckedChange={(checked) => applyConfig({ includeVehicles: checked })} />
            </label>
            <label className="flex items-center justify-between gap-3 rounded-md border border-border/60 px-3 py-2 text-sm">
              <span>{t('targeting.peds')}</span>
              <Switch checked={includePeds} onCheckedChange={(checked) => applyConfig({ includePeds: checked })} />
            </label>
          </div>

          <div className="grid gap-3 border-t border-border/60 pt-4 sm:grid-cols-2">
            <label className="grid gap-1.5 text-xs text-muted-foreground">
              <span>{t('targeting.radius')}</span>
              <Input
                type="number"
                name="targeting-radius"
                autoComplete="off"
                min={5}
                max={250}
                step={5}
                value={radius}
                onChange={(event) => setRadius(event.target.value)}
                onBlur={() => applyNumberConfig('radius', radius)}
                inputMode="numeric"
              />
            </label>
            <label className="grid gap-1.5 text-xs text-muted-foreground">
              <span>{t('targeting.hitRadius', '中键选取范围')}</span>
              <Input
                type="number"
                name="targeting-hit-radius"
                autoComplete="off"
                min={40}
                max={400}
                step={10}
                value={hitRadius}
                onChange={(event) => setHitRadius(event.target.value)}
                onBlur={() => applyNumberConfig('hitRadius', hitRadius)}
                inputMode="numeric"
              />
            </label>
            <label className="grid gap-1.5 text-xs text-muted-foreground">
              <span>{t('targeting.maxTargets')}</span>
              <Input type="number" name="targeting-max-targets" autoComplete="off" min={1} max={64} step={1} value={maxTargets}
                onChange={(event) => setMaxTargets(event.target.value)}
                onBlur={() => applyNumberConfig('maxTargets', maxTargets)} inputMode="numeric" />
            </label>
          </div>

          <div className="rounded-md border border-primary/30 bg-primary/5 px-3 py-2 text-xs text-muted-foreground">
            {enabled
              ? t('targeting.closeHint', '关闭菜单后链路仍会保留；用准星瞄准目标并按住鼠标中键，移动鼠标选择动作，松开执行。')
              : t('targeting.enableHint', '启用链路后会持续扫描附近车辆和 NPC。')}
          </div>
        </CardContent>
      </Card>

      <section className="min-w-0 space-y-4 border-t pt-4" aria-labelledby="target-menu-title">
        <div className="flex items-center justify-between gap-3">
          <h2 id="target-menu-title" className="text-base font-semibold">{t('targeting.customMenu')}</h2>
          <Button variant="outline" size="icon" disabled={menuPending}
            aria-label={t('targeting.resetMenu')} title={t('targeting.resetMenu')} onClick={() => void resetMenu()}>
            <RotateCcw className="size-4" aria-hidden="true" />
          </Button>
        </div>
        {menuError ? <p role="alert" className="text-xs text-destructive">{t('targeting.saveFailed')}</p> : null}
        {actions && value?.pedMenu && value.vehicleMenu ? (
          <Tabs defaultValue="ped">
            <TabsList aria-label={t('targeting.customMenu')}>
              <TabsTrigger value="ped">{t('targeting.pedMenu')}</TabsTrigger>
              <TabsTrigger value="vehicle">{t('targeting.vehicleMenu')}</TabsTrigger>
            </TabsList>
            <TabsContent value="ped">
              <TargetMenuEditor kind="ped" menu={value.pedMenu} actions={actions.ped}
                colorChannels={actions.colorChannels} onChange={updateMenu}
                onSwap={async (pedMenu) => { await call('targeting.config', { pedMenu }); refresh() }} />
            </TabsContent>
            <TabsContent value="vehicle">
              <TargetMenuEditor kind="vehicle" menu={value.vehicleMenu} actions={actions.vehicle}
                colorChannels={actions.colorChannels} onChange={updateMenu}
                onSwap={async (vehicleMenu) => { await call('targeting.config', { vehicleMenu }); refresh() }} />
            </TabsContent>
          </Tabs>
        ) : <p role="status" className="text-sm text-muted-foreground">{t('targeting.loading')}</p>}
      </section>

      <Card>
        <CardHeader>
          <CardTitle>{t('targeting.nearby')}</CardTitle>
          <CardDescription>{t('targeting.selectHint')}</CardDescription>
        </CardHeader>
        <CardContent className="grid gap-4">
          {targets.length ? (
            <div className="grid gap-2">
              {targets.map((item) => (
                <div
                  key={`${item.kind}-${item.id}`}
                  className={`flex items-center justify-between gap-3 border px-3 py-2 text-left ${item.selected ? 'border-primary bg-primary/10' : 'border-border/60'}`}
                >
                  <span className="flex min-w-0 items-center gap-2">
                    <Badge variant={item.kind === 'vehicle' ? 'outline' : 'secondary'}>
                      {targetLabel(item, t)}
                    </Badge>
                    <span className="truncate text-sm">#{item.id} · {t('targeting.model')} {item.modelId}</span>
                  </span>
                  <span className="shrink-0 text-xs text-muted-foreground">
                    {item.distance.toFixed(1)}m · {item.health.toFixed(0)} HP
                  </span>
                </div>
              ))}
            </div>
          ) : (
            <p className="text-sm text-muted-foreground">{t('targeting.empty')}</p>
          )}

          <div className="grid gap-3 border-t border-border/60 pt-4">
            <div className="flex items-center justify-between gap-3">
              <div className="text-sm font-medium">
                {selected
                  ? `${t('targeting.selected', '已锁定')} · ${targetLabel(selected, t)} #${selected.id}`
                  : t('targeting.noneSelected', '尚未锁定目标')}
              </div>
              {selected ? <Badge variant="outline">{selected.health.toFixed(0)} HP</Badge> : null}
            </div>
            <p className="text-xs text-muted-foreground">{t('targeting.gameInputHint')}</p>
          </div>
        </CardContent>
      </Card>
    </div>
  )
}
