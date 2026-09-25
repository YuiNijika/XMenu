import { useEffect, useState } from 'react'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { Slider } from '@/components/ui/slider'
import { Switch } from '@/components/ui/switch'
import { Separator } from '@/components/ui/separator'
import { ParamToggles, ToggleGrid } from '@/components/menu/toggle-grid'
import { SchemaSection } from '@/components/menu/schema-section'
import { runAction, runActionQuiet } from '@/lib/actions'
import {
  call,
  fetchUiSchema,
  isUsable,
  type CapabilityReport,
  type UiSchemaPayload,
  type WorldTime,
} from '@/lib/bridge'
import { usePolling } from '@/lib/hooks'
import { useI18n } from '@/lib/i18n'

const WeatherFields = [
  { key: 'rain', label: 'world.rain' },
  { key: 'fog', label: 'world.fog' },
  { key: 'clouds', label: 'world.clouds' },
  { key: 'wind', label: 'world.wind' },
  { key: 'sandstorm', label: 'world.sandstorm' },
  { key: 'extraSunny', label: 'world.extraSunny' },
  { key: 'wetRoads', label: 'world.wetRoads' },
  { key: 'gameSpeed', label: 'world.gameSpeed' },
]

type PageProps = {
  report: CapabilityReport | null
}

type WeatherEntry = { id: number; key: string }

export function WorldPage({ report }: PageProps) {
  const [schema, setSchema] = useState<UiSchemaPayload | null>(null)

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

  const { value } = usePolling<WorldTime>('world.getTime', 1000)
  const { t } = useI18n()
  const [hour, setHour] = useState('12')
  const [minute, setMinute] = useState('0')
  const [weather, setWeather] = useState(0)
  const [speed, setSpeed] = useState([1])
  const [gravity, setGravity] = useState('0.008')
  const [freeze, setFreeze] = useState(false)
  const [fps, setFps] = useState('0')
  const [days, setDays] = useState('0')
  const [pickupModel, setPickupModel] = useState('1240')
  const [pickupType, setPickupType] = useState('3')
  const [pickupQuantity, setPickupQuantity] = useState('1')
  const [pickupMoney, setPickupMoney] = useState('0')
  const [interval, setInterval] = useState('5')
  const [weatherValues, setWeatherValues] = useState<Record<string, string>>({})
  const [weatherList, setWeatherList] = useState<WeatherEntry[]>([])

  useEffect(() => {
    call<{ items: WeatherEntry[] }>('world.weatherCatalog')
      .then((payload) => setWeatherList(payload.items ?? []))
      .catch(() => setWeatherList([]))
  }, [])

  useEffect(() => {
    if (!value) return
    setHour(String(value.hour))
    setMinute(String(value.minute))
  }, [value])

  return (
    <div className="grid gap-5 md:grid-cols-2">
      <Card className="md:col-span-2">
        <CardHeader>
          <CardTitle>{t('world.gameRules')}</CardTitle>
          <CardDescription>{t('react.actionsHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-5">
          <div className="flex flex-col gap-3">
            <div className="text-sm font-medium">{t('world.time')}</div>
            <div className="flex items-end gap-3">
              <div className="w-24">
                <div className="mb-2 text-xs text-muted-foreground">{t('react.hour')}</div>
                <Input value={hour} onChange={(event) => setHour(event.target.value)} inputMode="numeric" />
              </div>
              <div className="w-24">
                <div className="mb-2 text-xs text-muted-foreground">{t('react.minute')}</div>
                <Input value={minute} onChange={(event) => setMinute(event.target.value)} inputMode="numeric" />
              </div>
              <Button
                disabled={!isUsable(report, 'world.setTime')}
                onClick={() =>
                  void runAction('world.setTime', { hour: Number(hour) || 0, minute: Number(minute) || 0 }, 'react.set')
                }
              >
                {t('react.set')}
              </Button>
            </div>
            <label className="flex items-center justify-between text-sm">
              <span>{t('world.freezeTime')}</span>
              <Switch
                checked={freeze}
                disabled={!isUsable(report, 'world.freezeTime')}
                onCheckedChange={(checked) => {
                  setFreeze(checked)
                  void runActionQuiet('ui.set', { id: 'world.freezeTime', value: checked })
                }}
              />
            </label>
          </div>

          <Separator />

          <ToggleGrid
            report={report}
            items={[
              { method: 'world.fasterClock', label: 'world.fasterClock' },
              { method: 'world.disableReplay', label: 'world.disableReplay' },
              { method: 'world.disableCheats', label: 'world.disableCheats' },
              { method: 'world.freePayNSpray', label: 'world.freePayNSpray' },
              { method: 'world.noWaterPhysics', label: 'world.noWaterPhysics' },
              { method: 'world.solidWater', label: 'world.solidWater' },
            ]}
          />

          <Separator />

          <div className="flex flex-wrap items-end gap-3">
            <div className="w-28">
              <div className="mb-2 text-xs text-muted-foreground">{t('world.fpsLimit')}</div>
              <Input value={fps} onChange={(event) => setFps(event.target.value)} inputMode="numeric" />
            </div>
            <Button
              variant="outline"
              disabled={!isUsable(report, 'world.fpsLimit')}
              onClick={() => void runAction('world.fpsLimit', { value: Number(fps) || 0 }, 'world.setFps')}
            >
              {t('react.set')}
            </Button>
            <div className="w-28">
              <div className="mb-2 text-xs text-muted-foreground">{t('world.daysPassed')}</div>
              <Input value={days} onChange={(event) => setDays(event.target.value)} inputMode="numeric" />
            </div>
            <Button
              variant="outline"
              disabled={!isUsable(report, 'world.daysPassed')}
              onClick={() => void runAction('world.daysPassed', { value: Number(days) || 0 }, 'world.setDays')}
            >
              {t('react.set')}
            </Button>
            <Button
              variant="outline"
              disabled={!isUsable(report, 'world.syncClock')}
              onClick={() => void runAction('ui.run', { id: 'world.syncClock' }, 'world.syncRealTime')}
            >
              {t('world.syncRealTime')}
            </Button>
            <Button
              variant="outline"
              onClick={() => void runAction('world.destroyVehicles', undefined, 'react.destroyVehicles')}
            >
              {t('react.destroyVehicles')}
            </Button>
            <Button variant="outline" onClick={() => void runAction('world.destroyPeds', undefined, 'react.destroyPeds')}>
              {t('react.destroyPeds')}
            </Button>
          </div>

          {schema ? (
            <div className="grid gap-4">
              <SchemaSection payload={schema} report={report} tabId="world" pageId="worldMain" sectionId="timeLock" />
              <SchemaSection payload={schema} report={report} tabId="world" pageId="worldMain" sectionId="gameRules" />
            </div>
          ) : null}
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>{t('world.weather')}</CardTitle>
          <CardDescription>{t('react.weatherHint')}</CardDescription>
        </CardHeader>
        <CardContent className="grid grid-cols-2 gap-2">
          {weatherList.map((item) => (
            <Button
              key={item.id}
              variant={weather === item.id ? 'default' : 'outline'}
              disabled={!isUsable(report, 'world.weather')}
              onClick={() => {
                setWeather(item.id)
                void runAction('world.weather', { id: item.id, lock: true }, item.key)
              }}
            >
              {t(item.key)}
            </Button>
          ))}
          <Button
            variant="outline"
            className="col-span-2"
            disabled={!isUsable(report, 'world.weatherRelease')}
            onClick={() => void runAction('world.weatherRelease', undefined, 'world.revertWeather')}
          >
            {t('world.revertWeather')}
          </Button>
        </CardContent>
      </Card>

      <Card className="md:col-span-2">
        <CardHeader>
          <CardTitle>{t('react.environment')}</CardTitle>
          <CardDescription>{t('react.environmentHint')}</CardDescription>
        </CardHeader>
        <CardContent className="grid gap-6 md:grid-cols-2">
          <div>
            <div className="mb-2 text-xs text-muted-foreground">
              {t('world.gameSpeed')} {speed[0].toFixed(1)}x
            </div>
            <Slider
              value={speed}
              min={0.2}
              max={3}
              step={0.1}
              disabled={!isUsable(report, 'world.gameSpeed')}
              onValueChange={(value) => {
                const next = Array.isArray(value) ? [...value] : [value]
                setSpeed(next)
                void runActionQuiet('world.gameSpeed', { value: next[0] })
              }}
            />
          </div>
          <div className="flex items-end gap-3">
            <div className="flex-1">
              <div className="mb-2 text-xs text-muted-foreground">{t('world.gravity')}</div>
              <Input value={gravity} onChange={(event) => setGravity(event.target.value)} />
            </div>
            <Button
              variant="outline"
              disabled={!isUsable(report, 'world.gravity')}
              onClick={() => void runAction('world.gravity', { value: Number(gravity) || 0 }, 'react.apply')}
            >
              {t('react.apply')}
            </Button>
          </div>
        </CardContent>
      </Card>

      <Card className="md:col-span-2">
        <CardHeader>
          <CardTitle>{t('world.pickup')}</CardTitle>
          <CardDescription>{t('world.pickupTip')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-wrap items-end gap-3">
          <div className="w-28">
            <div className="mb-2 text-xs text-muted-foreground">{t('world.pickupModelId')}</div>
            <Input value={pickupModel} onChange={(event) => setPickupModel(event.target.value)} inputMode="numeric" />
          </div>
          <div className="w-24">
            <div className="mb-2 text-xs text-muted-foreground">{t('world.pickupType')}</div>
            <Input value={pickupType} onChange={(event) => setPickupType(event.target.value)} inputMode="numeric" />
          </div>
          <div className="w-24">
            <div className="mb-2 text-xs text-muted-foreground">{t('world.pickupQuantity')}</div>
            <Input
              value={pickupQuantity}
              onChange={(event) => setPickupQuantity(event.target.value)}
              inputMode="numeric"
            />
          </div>
          <div className="w-28">
            <div className="mb-2 text-xs text-muted-foreground">{t('world.pickupMoneyPerDay')}</div>
            <Input
              value={pickupMoney}
              onChange={(event) => setPickupMoney(event.target.value)}
              inputMode="numeric"
            />
          </div>
          <Button
            disabled={!isUsable(report, 'world.pickup')}
            onClick={() =>
              void runAction(
                'world.pickup',
                {
                  modelId: Number(pickupModel) || 1240,
                  type: Number(pickupType) || 3,
                  quantity: Number(pickupQuantity) || 1,
                  moneyPerDay: Number(pickupMoney) || 0,
                },
                'world.spawnPickup',
              )
            }
          >
            {t('world.spawnPickup')}
          </Button>
          <Button
            variant="outline"
            disabled={!isUsable(report, 'world.removePickups')}
            onClick={() => void runAction('world.removePickups', undefined, 'world.removeLastPickup')}
          >
            {t('world.removeLastPickup')}
          </Button>
        </CardContent>
      </Card>

      <Card className="md:col-span-2">
        <CardHeader>
          <CardTitle>{t('world.cheats')}</CardTitle>
          <CardDescription>{t('react.actionsHint')}</CardDescription>
        </CardHeader>
        <CardContent>
          <ToggleGrid
            report={report}
            items={[
              { method: 'cheats.flyingCars', label: 'vehicle.flyingCars' },
              { method: 'cheats.allCarsHaveNitro', label: 'vehicle.infNitro' },
              { method: 'cheats.perfectHandling', label: 'vehicle.perfectHandling' },
              { method: 'cheats.greenLights', label: 'vehicle.greenLights' },
              { method: 'cheats.boatFly', label: 'vehicle.boatFly' },
              { method: 'cheats.driveWater', label: 'vehicle.driveWater' },
              { method: 'cheats.tankMode', label: 'vehicle.tankMode' },
              { method: 'cheats.aimDrive', label: 'vehicle.aimDrive' },
              { method: 'cheats.noDerail', label: 'vehicle.noDerail' },
              { method: 'cheats.flipNoBurn', label: 'vehicle.flipNoBurn' },
              { method: 'cheats.stayOnBike', label: 'vehicle.stayOnBike' },
              { method: 'cheats.bikeFly', label: 'vehicle.bikeFly' },
            ]}
          />
        </CardContent>
      </Card>

      <Card className="md:col-span-2">
        <CardHeader>
          <CardTitle>{t('world.randomCheats')}</CardTitle>
          <CardDescription>{t('world.randomCheatsList')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-4">
          <ParamToggles
            report={report}
            method="cheats.random"
            items={[
              { method: 'cheats.random', key: 'enabled', label: 'world.enableRandomCheats' },
              { method: 'cheats.random', key: 'showProgress', label: 'world.showRandomCheatsProgress' },
            ]}
          />
          <div className="flex flex-wrap items-end gap-3">
            <div className="w-32">
              <div className="mb-2 text-xs text-muted-foreground">{t('world.randomCheatsInterval')}</div>
              <Input value={interval} onChange={(event) => setInterval(event.target.value)} inputMode="numeric" />
            </div>
            <Button
              variant="outline"
              disabled={!isUsable(report, 'cheats.random')}
              onClick={() => void runAction('cheats.random', { intervalSeconds: Number(interval) || 5 }, 'world.randomCheats')}
            >
              {t('react.apply')}
            </Button>
            <Button
              variant="outline"
              disabled={!isUsable(report, 'cheats.randomList')}
              onClick={() => void runAction('cheats.randomList', undefined, 'world.randomCheatsList')}
            >
              {t('world.randomCheatsList')}
            </Button>
          </div>
        </CardContent>
      </Card>

      <Card className="md:col-span-2">
        <CardHeader>
          <CardTitle>{t('world.environment')}</CardTitle>
          <CardDescription>{t('react.environmentHint')}</CardDescription>
        </CardHeader>
        <CardContent className="grid gap-4 md:grid-cols-4">
          {WeatherFields.map((field) => (
            <div key={field.key}>
              <div className="mb-2 text-xs text-muted-foreground">{t(field.label)}</div>
              <div className="flex items-center gap-2">
                <Input
                  value={weatherValues[field.key] ?? '0'}
                  onChange={(event) =>
                    setWeatherValues((previous) => ({ ...previous, [field.key]: event.target.value }))
                  }
                  inputMode="decimal"
                />
                <Button
                  variant="outline"
                  size="sm"
                  disabled={!isUsable(report, 'world.environment')}
                  onClick={() =>
                    void runActionQuiet('world.environment', {
                      [field.key]: Number(weatherValues[field.key] ?? '0') || 0,
                    })
                  }
                >
                  {t('react.apply')}
                </Button>
              </div>
            </div>
          ))}
        </CardContent>
      </Card>
    </div>
  )
}
