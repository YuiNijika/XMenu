import { useState } from 'react'
import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { Separator } from '@/components/ui/separator'
import { Switch } from '@/components/ui/switch'
import { call, isUsable, type CapabilityReport, type VehicleSnapshot } from '@/lib/bridge'
import { usePolling } from '@/lib/hooks'
import { useI18n } from '@/lib/i18n'

type PageProps = {
  report: CapabilityReport | null
}

export function VehiclePage({ report }: PageProps) {
  const { value } = usePolling<VehicleSnapshot>('vehicle.snapshot', 500)
  const { t } = useI18n()
  const [model, setModel] = useState('411')
  const [asDriver, setAsDriver] = useState(true)
  const [cleanup, setCleanup] = useState(true)
  const [primary, setPrimary] = useState('0')
  const [secondary, setSecondary] = useState('0')

  const run = async (method: string, params?: Record<string, unknown>) => {
    try {
      await call(method, params)
    } catch {
      // 忽略单次失败，状态由快照轮询反映
    }
  }

  return (
    <div className="grid gap-5 lg:grid-cols-2">
      <Card>
        <CardHeader>
          <CardTitle>{t('react.currentVehicle', '当前载具')}</CardTitle>
          <CardDescription>{t('react.noVehicle', '无载具时显示为空')}</CardDescription>
        </CardHeader>
        <CardContent className="grid grid-cols-2 gap-4 text-sm">
          <Stat label={t('react.model', '模型')} value={value?.valid ? String(value.modelId) : '--'} />
          <Stat label={t('react.durability', '耐久')} value={value?.valid ? value.health.toFixed(0) : '--'} />
          <Stat label={t('react.colors', '颜色')} value={value?.valid ? `${value.colors.primary} / ${value.colors.secondary}` : '--'} />
          <Stat label={t('react.lights', '车灯')} value={value?.valid ? (value.lights ? '开' : '关') : '--'} />
          <div className="col-span-2 flex flex-wrap gap-2 pt-2">
            <Button disabled={!isUsable(report, 'vehicle.repair')} onClick={() => run('vehicle.repair')}>
              {t('vehicle.repair')}
            </Button>
            <Button variant="outline" disabled={!isUsable(report, 'vehicle.unflip')} onClick={() => run('vehicle.unflip')}>
              {t('vehicle.unflip')}
            </Button>
          </div>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>生成载具</CardTitle>
          <CardDescription>{t('react.spawnHint', '模型 ID 参考游戏车辆表')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-4">
          <div className="flex items-end gap-3">
            <div className="flex-1">
              <div className="mb-2 text-xs text-muted-foreground">{t('react.modelId', '模型 ID')}</div>
              <Input value={model} onChange={(event) => setModel(event.target.value)} inputMode="numeric" />
            </div>
            <Button
              disabled={!isUsable(report, 'vehicle.spawn')}
              onClick={() =>
                run('vehicle.spawn', {
                  model: Number(model) || 0,
                  asDriver,
                  cleanupPrevious: cleanup,
                })
              }
            >
              生成
            </Button>
          </div>
          <Separator />
          <label className="flex items-center justify-between text-sm">
            <span>{t('vehicle.spawnAsDriver')}</span>
            <Switch checked={asDriver} onCheckedChange={setAsDriver} />
          </label>
          <label className="flex items-center justify-between text-sm">
            <span>{t('vehicle.cleanupAfterSpawn')}</span>
            <Switch checked={cleanup} onCheckedChange={setCleanup} />
          </label>
        </CardContent>
      </Card>

      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('react.bodyColors', '车身颜色')}</CardTitle>
          <CardDescription>{t('react.bodyColorsHint', '仅对当前载具生效')}</CardDescription>
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
            onClick={() => run('vehicle.colors', { primary: Number(primary) || 0, secondary: Number(secondary) || 0 })}
          >
            应用
          </Button>
          <Badge variant="secondary" className="ml-auto">
            需要能力 VehicleColors
          </Badge>
        </CardContent>
      </Card>
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
