import { useEffect, useState } from 'react'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { Slider } from '@/components/ui/slider'
import { Switch } from '@/components/ui/switch'
import { ParamToggles, ToggleGrid } from '@/components/menu/toggle-grid'
import { runAction, runActionQuiet } from '@/lib/actions'
import { isUsable, type CapabilityReport, type WorldTime } from '@/lib/bridge'
import { usePolling } from '@/lib/hooks'
import { useI18n } from '@/lib/i18n'

type PageProps = {
  report: CapabilityReport | null
}

const WeatherNames = [
  'weather.sunny',
  'weather.cloudy',
  'weather.rainy',
  'weather.foggy',
  'weather.extrasunny',
  'weather.sandstorm_desert',
  'weather.underwater',
  'weather.extracolours_1',
]

export function WorldPage({ report }: PageProps) {
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

  useEffect(() => {
    if (!value) return
    setHour(String(value.hour))
    setMinute(String(value.minute))
  }, [value])

  return (
    <div className="grid gap-5 lg:grid-cols-2">
      <Card>
        <CardHeader>
          <CardTitle>{t('world.time')}</CardTitle>
          <CardDescription>{value ? `${value.hour} / ${value.minute}` : t('react.unknown')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-4">
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
                void runActionQuiet('world.freezeTime', { enable: checked })
              }}
            />
          </label>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>{t('world.weather')}</CardTitle>
          <CardDescription>{t('react.weatherHint')}</CardDescription>
        </CardHeader>
        <CardContent className="grid grid-cols-2 gap-2">
          {WeatherNames.map((name, index) => (
            <Button
              key={name}
              variant={weather === index ? 'default' : 'outline'}
              disabled={!isUsable(report, 'world.weather')}
              onClick={() => {
                setWeather(index)
                void runAction('world.weather', { id: index, lock: true }, name)
              }}
            >
              {t(name)}
            </Button>
          ))}
        </CardContent>
      </Card>

      <Card className="lg:col-span-2">
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

      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('react.actions')}</CardTitle>
          <CardDescription>{t('react.actionsHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-wrap gap-3">
          <Button
            variant="outline"
            onClick={() => void runAction('world.destroyVehicles', undefined, 'react.destroyVehicles')}
          >
            {t('react.destroyVehicles')}
          </Button>
          <Button variant="outline" onClick={() => void runAction('world.destroyPeds', undefined, 'react.destroyPeds')}>
            {t('react.destroyPeds')}
          </Button>
        </CardContent>
      </Card>

      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('world.gameRules')}</CardTitle>
          <CardDescription>{t('react.actionsHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-4">
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
              disabled={!isUsable(report, 'world.weatherRelease')}
              onClick={() => void runAction('world.weatherRelease', undefined, 'world.revertWeather')}
            >
              {t('world.revertWeather')}
            </Button>
            <Button
              variant="outline"
              disabled={!isUsable(report, 'world.syncClock')}
              onClick={() => void runAction('world.syncClock', undefined, 'world.syncRealTime')}
            >
              {t('world.syncRealTime')}
            </Button>
          </div>
        </CardContent>
      </Card>
      <Card className="lg:col-span-2">
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

      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('vehicle.flyingCars')}</CardTitle>
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

      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('world.randomCheats')}</CardTitle>
          <CardDescription>{t('world.randomCheatsList')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-4">
          <ParamToggles
            report={report}
            method="cheats.random"
            items={[
              { method: '', label: 'enabled' },
              { method: '', label: 'showProgress' },
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

    </div>
  )
}
