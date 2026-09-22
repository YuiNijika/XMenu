import { useState } from 'react'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { call, isUsable, type CapabilityReport } from '@/lib/bridge'
import { useI18n } from '@/lib/i18n'

type PageProps = {
  report: CapabilityReport | null
}

export function TeleportPage({ report }: PageProps) {
  const [x, setX] = useState('0')
  const { t } = useI18n()
  const [y, setY] = useState('0')
  const [z, setZ] = useState('0')
  const [distance, setDistance] = useState('50')
  const [message, setMessage] = useState<string | null>(null)

  const run = async (method: string, params?: Record<string, unknown>, label?: string) => {
    try {
      const result = await call<boolean>(method, params)
      setMessage(result ? `${label ?? method} 完成` : `${label ?? method} 未生效`)
    } catch (reason: unknown) {
      setMessage(reason instanceof Error ? reason.message : String(reason))
    }
  }

  return (
    <div className="grid gap-5 lg:grid-cols-2">
      <Card>
        <CardHeader>
          <CardTitle>{t('react.teleportHint', '坐标传送')}</CardTitle>
          <CardDescription>直接写入世界坐标</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-4">
          <div className="grid grid-cols-3 gap-3">
            <Field label="X" value={x} onChange={setX} />
            <Field label="Y" value={y} onChange={setY} />
            <Field label="Z" value={z} onChange={setZ} />
          </div>
          <Button
            disabled={!isUsable(report, 'teleport.to')}
            onClick={() => run('teleport.to', { x: Number(x) || 0, y: Number(y) || 0, z: Number(z) || 0 }, '坐标传送')}
          >
            传送
          </Button>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>{t('react.quickTeleport', '快捷传送')}</CardTitle>
          <CardDescription>{t('react.quickTeleportHint', '沿当前朝向或地图标记移动')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-4">
          <div className="flex items-end gap-3">
            <div className="flex-1">
              <div className="mb-2 text-xs text-muted-foreground">{t('teleport.forwardDistance')}</div>
              <Input value={distance} onChange={(event) => setDistance(event.target.value)} inputMode="numeric" />
            </div>
            <Button
              variant="outline"
              disabled={!isUsable(report, 'teleport.forward')}
              onClick={() => run('teleport.forward', { distance: Number(distance) || 0 }, '向前传送')}
            >
              向前
            </Button>
          </div>
          <Button
            disabled={!isUsable(report, 'teleport.marker')}
            onClick={() => run('teleport.marker', { underwater: false }, '标记传送')}
          >
            {t('teleport.toMarker')}
          </Button>
        </CardContent>
      </Card>

      {message ? (
        <Card className="lg:col-span-2">
          <CardContent className="py-4 text-sm text-muted-foreground">{message}</CardContent>
        </Card>
      ) : null}
    </div>
  )
}

function Field({ label, value, onChange }: { label: string; value: string; onChange: (value: string) => void }) {
  return (
    <div>
      <div className="mb-2 text-xs text-muted-foreground">{label}</div>
      <Input value={value} onChange={(event) => onChange(event.target.value)} />
    </div>
  )
}
