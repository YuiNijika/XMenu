import { useState } from 'react'
import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { Separator } from '@/components/ui/separator'
import { Slider } from '@/components/ui/slider'
import { call, isUsable, type CapabilityReport, type PlayerSnapshot } from '@/lib/bridge'
import { usePolling } from '@/lib/hooks'
import { useI18n } from '@/lib/i18n'

type PageProps = {
  report: CapabilityReport | null
}

export function PlayerPage({ report }: PageProps) {
  const { value } = usePolling<PlayerSnapshot>('player.snapshot', 400)
  const { t } = useI18n()
  const [amount, setAmount] = useState('1000')
  const [wanted, setWanted] = useState([0])
  const [busy, setBusy] = useState(false)

  const run = async (method: string, params?: Record<string, unknown>) => {
    setBusy(true)
    try {
      await call(method, params)
    } catch {
      // 失败时保持界面不变，错误由能力表与轮询结果体现
    } finally {
      setBusy(false)
    }
  }

  return (
    <div className="grid gap-5 lg:grid-cols-2">
      <Card>
        <CardHeader>
          <CardTitle>{t('react.status', '状态')}</CardTitle>
          <CardDescription>{t('react.statusHint', '每 0.4 秒读取一次玩家快照')}</CardDescription>
        </CardHeader>
        <CardContent className="grid grid-cols-2 gap-4 text-sm">
          <Stat label="{t('player.health')}" value={value ? value.health.toFixed(0) : '--'} />
          <Stat label="{t('player.armor')}" value={value ? value.armour.toFixed(0) : '--'} />
          <Stat label="{t('player.money')}" value={value ? String(value.money) : '--'} />
          <Stat label="通缉" value={value ? String(value.wantedLevel) : '--'} />
          <Stat
            label={t('react.position', '坐标')}
            value={
              value
                ? `${value.position.x.toFixed(0)} ${value.position.y.toFixed(0)} ${value.position.z.toFixed(0)}`
                : '--'
            }
          />
          <Stat label={t('react.valid', '可用')} value={value?.valid ? '是' : '否'} />
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>{t('react.actions', '操作')}</CardTitle>
          <CardDescription>{t('react.actionsHint', '按钮与滑条直接调用 XBase 接口')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-4">
          <div className="flex flex-wrap gap-2">
            <Button disabled={busy || !isUsable(report, 'player.heal')} onClick={() => run('player.heal')}>
              {t('player.healFully')}
            </Button>
            <Button
              variant="outline"
              disabled={busy || !isUsable(report, 'player.armour')}
              onClick={() => run('player.armour')}
            >
              {t('player.armor')}
            </Button>
            <Button
              variant="outline"
              disabled={busy || !isUsable(report, 'player.kill')}
              onClick={() => run('player.kill')}
            >
              {t('player.kill')}
            </Button>
          </div>
          <Separator />
          <div className="flex items-end gap-3">
            <div className="flex-1">
              <div className="mb-2 text-xs text-muted-foreground">{t('player.money')}</div>
              <Input value={amount} onChange={(event) => setAmount(event.target.value)} inputMode="numeric" />
            </div>
            <Button
              disabled={busy || !isUsable(report, 'player.money')}
              onClick={() => run('player.money', { amount: Number(amount) || 0 })}
            >
              {t('player.setMoney')}
            </Button>
          </div>
          <div>
            <div className="mb-2 flex items-center justify-between text-xs text-muted-foreground">
              <span>{t('player.wantedLevel')}</span>
              <Badge variant="secondary">{wanted[0]}</Badge>
            </div>
            <Slider
              value={wanted}
              min={0}
              max={6}
              step={1}
              onValueChange={(value) => {
                const next = Array.isArray(value) ? [...value] : [value]
                setWanted(next)
                void run('player.wanted', { level: next[0] })
              }}
            />
          </div>
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
