import { useEffect, useState } from 'react'
import { SchemaSection } from '@/components/menu/schema-section'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { Separator } from '@/components/ui/separator'
import { Switch } from '@/components/ui/switch'
import { ToggleGrid } from '@/components/menu/toggle-grid'
import { DataBrowser } from '@/components/menu/data-browser'
import { runAction, runActionQuiet } from '@/lib/actions'
import {
  fetchUiSchema,
  isUsable,
  type CapabilityReport,
  type UiSchemaPayload,
  type VehicleSnapshot,
} from '@/lib/bridge'
import { usePolling } from '@/lib/hooks'
import { useI18n } from '@/lib/i18n'

type PageProps = {
  report: CapabilityReport | null
}

export function VehiclePage({ report }: PageProps) {
  const { value } = usePolling<VehicleSnapshot>('vehicle.snapshot', 500)
  const { t } = useI18n()
  const [model, setModel] = useState('411')
  const [primary, setPrimary] = useState('0')
  const [secondary, setSecondary] = useState('0')
  const [locked, setLocked] = useState(false)
  const [health, setHealth] = useState('1000')
  const [paintjob, setPaintjob] = useState('0')
  const [tertiary, setTertiary] = useState('0')
  const [quaternary, setQuaternary] = useState('0')
  const [density, setDensity] = useState('0.8')
  const [siren, setSiren] = useState(false)
  const [speed, setSpeed] = useState('60')
  const [modId, setModId] = useState('1000')
  const [seat, setSeat] = useState('0')
  const [schema, setSchema] = useState<UiSchemaPayload | null>(null)

  // 界面注册表只取一次，网页端与 ImGui 从此共用同一份控件与能力门控
  useEffect(() => {
    let alive = true
    void fetchUiSchema()
      .then((payload) => {
        if (alive) setSchema(payload)
      })
      .catch(() => {})
    return () => {
      alive = false
    }
  }, [])

  return (
    <div className="grid gap-5 lg:grid-cols-2">
      <Card>
        <CardHeader>
          <CardTitle>{t('react.currentVehicle')}</CardTitle>
          <CardDescription>{t('react.noVehicle')}</CardDescription>
        </CardHeader>
        <CardContent className="grid grid-cols-2 gap-4 text-sm">
          <Stat label={t('react.model')} value={value?.valid ? String(value.modelId) : '--'} />
          <Stat label={t('react.durability')} value={value?.valid ? value.health.toFixed(0) : '--'} />
          <Stat
            label={t('react.colors')}
            value={value?.valid ? `${value.colors.primary} / ${value.colors.secondary}` : '--'}
          />
          <Stat
            label={t('react.lights')}
            value={value?.valid ? (value.lights ? t('react.on') : t('react.off')) : '--'}
          />
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>{t('react.spawn')}</CardTitle>
          <CardDescription>{t('react.spawnHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-4">
          <div className="flex items-end gap-3">
            <div className="flex-1">
              <div className="mb-2 text-xs text-muted-foreground">{t('react.modelId')}</div>
              <Input value={model} onChange={(event) => setModel(event.target.value)} inputMode="numeric" />
            </div>
            <Button
              disabled={!isUsable(report, 'vehicle.spawn')}
              onClick={() => void runAction('vehicle.spawn', { model: Number(model) || 0 }, 'react.spawn')}
            >
              {t('react.spawn')}
            </Button>
          </div>
          <Separator />
          {/* 生成选项由注册表的生成选项分区维护，宿主侧读的是同一份状态 */}
          <p className="text-xs text-muted-foreground">{t('vehicle.spawnAsDriver')}</p>
        </CardContent>
      </Card>

      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('react.bodyColors')}</CardTitle>
          <CardDescription>{t('react.bodyColorsHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-wrap items-end gap-3">
          <div className="w-32">
            <div className="mb-2 text-xs text-muted-foreground">{t('vehicle.color1')}</div>
            <Input value={primary} onChange={(event) => setPrimary(event.target.value)} inputMode="numeric" />
          </div>
          <div className="w-32">
            <div className="mb-2 text-xs text-muted-foreground">{t('vehicle.color2')}</div>
            <Input value={secondary} onChange={(event) => setSecondary(event.target.value)} inputMode="numeric" />
          </div>
          <Button
            variant="outline"
            disabled={!isUsable(report, 'vehicle.colors')}
            onClick={() =>
              void runAction(
                'vehicle.colors',
                { primary: Number(primary) || 0, secondary: Number(secondary) || 0 },
                'react.apply',
              )
            }
          >
            {t('react.apply')}
          </Button>
        </CardContent>
      </Card>
      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('vehicle.applyPaintjobMod')}</CardTitle>
          <CardDescription>{t('react.actionsHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-wrap items-end gap-3">
          <div className="w-28">
            <div className="mb-2 text-xs text-muted-foreground">{t('react.durability')}</div>
            <Input value={health} onChange={(event) => setHealth(event.target.value)} inputMode="numeric" />
          </div>
          <Button
            variant="outline"
            disabled={!isUsable(report, 'vehicle.health')}
            onClick={() => void runAction('ui.set', { id: 'vehicle.health', value: Number(health) || 0 }, 'react.durability')}
          >
            {t('react.set')}
          </Button>
          <div className="w-28">
            <div className="mb-2 text-xs text-muted-foreground">{t('vehicle.applyPaintjobMod')}</div>
            <Input value={paintjob} onChange={(event) => setPaintjob(event.target.value)} inputMode="numeric" />
          </div>
          <Button
            variant="outline"
            disabled={!isUsable(report, 'vehicle.paintjob')}
            onClick={() => void runAction('vehicle.paintjob', { index: Number(paintjob) || 0 }, 'vehicle.applyPaintjobMod')}
          >
            {t('react.apply')}
          </Button>
          <label className="flex items-center gap-2 text-sm">
            <span>{t('overlay.vehicleLocked')}</span>
            <Switch
              checked={locked}
              disabled={!isUsable(report, 'vehicle.locked')}
              onCheckedChange={(checked) => {
                setLocked(checked)
                void runActionQuiet('ui.set', { id: 'vehicle.locked', value: checked })
              }}
            />
          </label>
          <Button
            variant="outline"
            disabled={!isUsable(report, 'vehicle.doors')}
            onClick={() => void runAction('vehicle.doors', { index: 0 }, 'vehicle.doors')}
          >
            {t('vehicle.doors')}
          </Button>
        </CardContent>
      </Card>
      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('vehicle.sectionRuntime')}</CardTitle>
          <CardDescription>{t('react.actionsHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-wrap items-end gap-3">
          <div className="w-24">
            <div className="mb-2 text-xs text-muted-foreground">{t('vehicle.color3')}</div>
            <Input value={tertiary} onChange={(event) => setTertiary(event.target.value)} inputMode="numeric" />
          </div>
          <div className="w-24">
            <div className="mb-2 text-xs text-muted-foreground">{t('vehicle.color4')}</div>
            <Input value={quaternary} onChange={(event) => setQuaternary(event.target.value)} inputMode="numeric" />
          </div>
          <Button
            variant="outline"
            disabled={!isUsable(report, 'vehicle.colors4')}
            onClick={() =>
              void runAction(
                'vehicle.colors4',
                {
                  tertiary: Number(tertiary) || 0,
                  quaternary: Number(quaternary) || 0,
                },
                'react.apply',
              )
            }
          >
            {t('react.apply')}
          </Button>
          <div className="w-28">
            <div className="mb-2 text-xs text-muted-foreground">{t('vehicle.trafficDensity')}</div>
            <Input value={density} onChange={(event) => setDensity(event.target.value)} inputMode="decimal" />
          </div>
          <Button
            variant="outline"
            disabled={!isUsable(report, 'vehicle.trafficDensity')}
            onClick={() =>
              void runAction('ui.set', { id: 'vehicle.trafficDensity', value: Number(density) || 1 }, 'react.apply')
            }
          >
            {t('react.apply')}
          </Button>
          <Button
            variant="outline"
            disabled={!isUsable(report, 'vehicle.blowUpAll')}
            onClick={() => void runAction('ui.run', { id: 'vehicle.blowUpAll' }, 'vehicle.blowUpAll')}
          >
            {t('vehicle.blowUpAll')}
          </Button>
          <Button
            variant="outline"
            disabled={!isUsable(report, 'vehicle.popDoor')}
            onClick={() => void runAction('vehicle.popDoor', { index: 0 }, 'vehicle.popDoor')}
          >
            {t('vehicle.popDoor')}
          </Button>
          <label className="flex items-center gap-2 text-sm">
            <span>{t('vehicle.sirenAlarm')}</span>
            <Switch
              checked={siren}
              disabled={!isUsable(report, 'vehicle.siren')}
              onCheckedChange={(checked) => {
                setSiren(checked)
                void runActionQuiet('ui.set', { id: 'vehicle.siren', value: checked })
              }}
            />
          </label>
        </CardContent>
      </Card>

      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('vehicle.lockSpeed')}</CardTitle>
          <CardDescription>{t('react.actionsHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-wrap items-end gap-3">
          <div className="w-28">
            <div className="mb-2 text-xs text-muted-foreground">{t('vehicle.targetSpeed')}</div>
            <Input value={speed} onChange={(event) => setSpeed(event.target.value)} inputMode="decimal" />
          </div>
          <Button
            variant="outline"
            disabled={!isUsable(report, 'vehicle.targetSpeed')}
            onClick={() => void runAction('ui.set', { id: 'vehicle.targetSpeed', value: Number(speed) || 60 }, 'vehicle.applyTargetSpeed')}
          >
            {t('react.apply')}
          </Button>
          <Button
            variant="outline"
            disabled={!isUsable(report, 'vehicle.speedLock')}
            onClick={() => void runAction('vehicle.speedLock', { speed: Number(speed) || 60 }, 'vehicle.lockSpeed')}
          >
            {t('vehicle.lockSpeed')}
          </Button>
          <Button
            variant="outline"
            disabled={!isUsable(report, 'vehicle.restoreSpeed')}
            onClick={() => void runAction('vehicle.restoreSpeed', undefined, 'vehicle.restoreDefaultSpeed')}
          >
            {t('vehicle.restoreDefaultSpeed')}
          </Button>
          <div className="w-28">
            <div className="mb-2 text-xs text-muted-foreground">{t('vehicle.modId')}</div>
            <Input value={modId} onChange={(event) => setModId(event.target.value)} inputMode="numeric" />
          </div>
          <Button
            variant="outline"
            disabled={!isUsable(report, 'vehicle.upgrade')}
            onClick={() => void runAction('vehicle.upgrade', { modelId: Number(modId) || 1000, enable: true }, 'vehicle.modId')}
          >
            {t('react.apply')}
          </Button>
        </CardContent>
      </Card>

      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('vehicle.spawnById')}</CardTitle>
          <CardDescription>{t('vehicle.spawnIdTip')}</CardDescription>
        </CardHeader>
        <CardContent>
          <DataBrowser
            method="data.vehicles"
            disabled={!isUsable(report, 'vehicle.spawn')}
            onPick={(item) =>
              void runAction('vehicle.spawn', { model: item.id ?? 0, asDriver: true, cleanupPrevious: true }, item.name)
            }
          />
        </CardContent>
      </Card>

      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('vehicle.sectionRuntime')}</CardTitle>
          <CardDescription>{t('react.actionsHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-4">
          <ToggleGrid
            report={report}
            items={[
              { method: 'vehicle.noDamage', label: 'vehicle.noDamage' },
              { method: 'vehicle.invisible', label: 'vehicle.invisible' },
            ]}
          />
          <div className="flex flex-wrap items-end gap-3">
            <div className="w-24">
              <div className="mb-2 text-xs text-muted-foreground">{t('vehicle.seatIndex')}</div>
              <Input value={seat} onChange={(event) => setSeat(event.target.value)} inputMode="numeric" />
            </div>
            <Button
              variant="outline"
              disabled={!isUsable(report, 'vehicle.seat')}
              onClick={() => void runAction('vehicle.seat', { index: Number(seat) || 0 }, 'vehicle.warpToSeat')}
            >
              {t('vehicle.warpToSeat')}
            </Button>
            <Button
              variant="outline"
              disabled={!isUsable(report, 'vehicle.resetColors')}
              onClick={() => void runAction('vehicle.resetColors', undefined, 'vehicle.resetColors')}
            >
              {t('vehicle.resetColors')}
            </Button>
          </div>
        </CardContent>
      </Card>

      {schema ? (
        <Card className="lg:col-span-2">
          <CardHeader>
            <CardTitle>{t('vehicle.sectionCheat')}</CardTitle>
            <CardDescription>{t('react.uiHint')}</CardDescription>
          </CardHeader>
          <CardContent className="grid gap-4">
            <SchemaSection payload={schema} report={report} tabId="vehicle" pageId="vehicleMain" sectionId="actions" />
            <SchemaSection payload={schema} report={report} tabId="vehicle" pageId="vehicleMain" sectionId="runtime" />
            <SchemaSection
              payload={schema}
              report={report}
              tabId="vehicle"
              pageId="vehicleMain"
              sectionId="status"
              visible={value?.valid === true}
            />
            <SchemaSection
              payload={schema}
              report={report}
              tabId="vehicle"
              pageId="vehicleMain"
              sectionId="proof"
              visible={value?.valid === true}
            />
            <SchemaSection
              payload={schema}
              report={report}
              tabId="vehicle"
              pageId="vehicleMain"
              sectionId="health"
              visible={value?.valid === true}
            />
            <SchemaSection
              payload={schema}
              report={report}
              tabId="vehicle"
              pageId="vehicleMain"
              sectionId="special"
              visible={value?.valid === true}
            />
            <SchemaSection payload={schema} report={report} tabId="vehicle" pageId="vehicleMain" sectionId="cheat" />
            <SchemaSection payload={schema} report={report} tabId="vehicle" pageId="vehicleMain" sectionId="effect" />
            <SchemaSection payload={schema} report={report} tabId="vehicle" pageId="vehicleMain" sectionId="autoDrive" />
            <SchemaSection payload={schema} report={report} tabId="vehicle" pageId="vehicleMain" sectionId="traffic" />
            <SchemaSection payload={schema} report={report} tabId="vehicle" pageId="vehicleMain" sectionId="speed" />
            <SchemaSection
              payload={schema}
              report={report}
              tabId="vehicle"
              pageId="vehicleSpawn"
              sectionId="spawnOptions"
            />
          </CardContent>
        </Card>
      ) : null}

    </div>
  )
}


function Stat({ label, value }: { label: string; value: string }) {
  return (
    <div className="rounded-lg border border-border/60 bg-muted/30 px-3 py-2">
      <div className="text-xs text-muted-foreground">{label}</div>
      <div className="mt-1 font-medium">{value}</div>
    </div>
  )
}
