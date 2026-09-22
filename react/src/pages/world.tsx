import { useEffect, useState } from 'react'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { Slider } from '@/components/ui/slider'
import { Switch } from '@/components/ui/switch'
import { call, isUsable, type CapabilityReport, type WorldTime } from '@/lib/bridge'
import { usePolling } from '@/lib/hooks'
import { useI18n } from '@/lib/i18n'

type PageProps = {
  report: CapabilityReport | null
}

const WeatherNames = ['world.weather.sunny', 'world.weather.cloudy', 'world.weather.rain', 'world.weather.fog', 'world.weather.sandstorm', 'world.weather.overcast', 'world.weather.partly', 'world.weather.storm']

export function WorldPage({ report }: PageProps) {
  const { value } = usePolling<WorldTime>('world.getTime', 1000)
  const { t } = useI18n()
  const [hour, setHour] = useState('12')
  const [minute, setMinute] = useState('0')
  const [weather, setWeather] = useState(0)
  const [speed, setSpeed] = useState([1])
  const [gravity, setGravity] = useState('0.008')
  const [freeze, setFreeze] = useState(false)

  useEffect(() => {
    if (!value) return
    setHour(String(value.hour))
    setMinute(String(value.minute))
  }, [value])

  const run = async (method: string, params?: Record<string, unknown>) => {
    try {
      await call(method, params)
    } catch {
      // 失败不阻断界面
    }
  }

  return (
    <div className="grid gap-5 lg:grid-cols-2">
      <Card>
        <CardHeader>
          <CardTitle>{t('world.time')}</CardTitle>
          <CardDescription>{value ? `${value.hour} / ${value.minute}` : t('react.unknown', '未知')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-4">
          <div className="flex items-end gap-3">
            <div className="w-24">
              <div className="mb-2 text-xs text-muted-foreground">{t('react.hour', '小时')}</div>
              <Input value={hour} onChange={(event) => setHour(event.target.value)} inputMode="numeric" />
            </div>
            <div className="w-24">
              <div className="mb-2 text-xs text-muted-foreground">{t('react.minute', '分钟')}</div>
              <Input value={minute} onChange={(event) => setMinute(event.target.value)} inputMode="numeric" />
            </div>
            <Button
              disabled={!isUsable(report, 'world.setTime')}
              onClick={() => run('world.setTime', { hour: Number(hour) || 0, minute: Number(minute) || 0 })}
            >
              设置
            </Button>
          </div>
          <label className="flex items-center justify-between text-sm">
            <span>{t('world.freezeTime')}</span>
            <Switch
              checked={freeze}
              disabled={!isUsable(report, 'world.freezeTime')}
              onCheckedChange={(checked) => {
                setFreeze(checked)
                void run('world.freezeTime', { enable: checked })
              }}
            />
          </label>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>{t('world.weather')}</CardTitle>
          <CardDescription>{t('react.weatherHint', '选中即生效并锁定')}</CardDescription>
        </CardHeader>
        <CardContent className="grid grid-cols-2 gap-2">
          {WeatherNames.map((name, index) => (
            <Button
              key={t(name, name)}
              variant={weather === index ? 'default' : 'outline'}
              disabled={!isUsable(report, 'world.weather')}
              onClick={() => {
                setWeather(index)
                void run('world.weather', { id: index, lock: true })
              }}
            >
              {t(name, name)}
            </Button>
          ))}
        </CardContent>
      </Card>

      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('react.environment', '环境')}</CardTitle>
          <CardDescription>{t('react.environmentHint', '速度与重力影响整个世界')}</CardDescription>
        </CardHeader>
        <CardContent className="grid gap-6 md:grid-cols-2">
          <div>
            <div className="mb-2 text-xs text-muted-foreground">{t('world.gameSpeed')} {speed[0].toFixed(1)}x</div>
            <Slider
              value={speed}
              min={0.2}
              max={3}
              step={0.1}
              disabled={!isUsable(report, 'world.gameSpeed')}
              onValueChange={(value) => {
                const next = Array.isArray(value) ? [...value] : [value]
                setSpeed(next)
                void run('world.gameSpeed', { value: next[0] })
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
              onClick={() => run('world.gravity', { value: Number(gravity) || 0 })}
            >
              应用
            </Button>
          </div>
        </CardContent>
      </Card>
    </div>
  )
}
