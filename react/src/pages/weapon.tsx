import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Switch } from '@/components/ui/switch'
import { call, isUsable, type CapabilityReport } from '@/lib/bridge'
import { useI18n } from '@/lib/i18n'
import { useState } from 'react'

type PageProps = {
  report: CapabilityReport | null
}

export function WeaponPage({ report }: PageProps) {
  const { t } = useI18n()
  const [infiniteAmmo, setInfiniteAmmo] = useState(false)
  const [message, setMessage] = useState<string | null>(null)

  const run = async (method: string, params?: Record<string, unknown>) => {
    try {
      const result = await call<boolean>(method, params)
      setMessage(result ? `${method} ok` : `${method} false`)
    } catch (reason: unknown) {
      setMessage(reason instanceof Error ? reason.message : String(reason))
    }
  }

  return (
    <div className="grid gap-5 lg:grid-cols-2">
      <Card>
        <CardHeader>
          <CardTitle>{t('tab.weapon')}</CardTitle>
          <CardDescription>{t('react.weaponHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-4">
          <Button disabled={!isUsable(report, 'weapon.giveAll')} onClick={() => run('weapon.giveAll')}>
            {t('player.maxWeaponSkills')}
          </Button>
          <label className="flex items-center justify-between text-sm">
            <span>{t('react.infiniteAmmo')}</span>
            <Switch
              checked={infiniteAmmo}
              disabled={!isUsable(report, 'weapon.infiniteAmmo')}
              onCheckedChange={(checked) => {
                setInfiniteAmmo(checked)
                void run('weapon.infiniteAmmo', { enable: checked })
              }}
            />
          </label>
          {message ? <div className="text-xs text-muted-foreground">{message}</div> : null}
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>{t('react.actions')}</CardTitle>
          <CardDescription>{t('react.actionsHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-wrap gap-2">
          <Button variant="outline" disabled={!isUsable(report, 'weapon.give')} onClick={() => run('weapon.give', { type: 22, ammo: 999 })}>
            {t('weapon.pistol')}
          </Button>
          <Button variant="outline" disabled={!isUsable(report, 'weapon.give')} onClick={() => run('weapon.give', { type: 24, ammo: 999 })}>
            {t('weapon.smg')}
          </Button>
          <Button variant="outline" disabled={!isUsable(report, 'weapon.give')} onClick={() => run('weapon.give', { type: 30, ammo: 999 })}>
            {t('weapon.rifle')}
          </Button>
          <Badge variant="secondary" className="ml-auto">
            {t('react.requiresCapability', 'Requires %s').replace('%s', 'WeaponGive')}
          </Badge>
        </CardContent>
      </Card>
    </div>
  )
}
