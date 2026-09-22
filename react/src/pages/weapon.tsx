import { useState } from 'react'
import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Switch } from '@/components/ui/switch'
import { ParamToggles, ToggleGrid } from '@/components/menu/toggle-grid'
import { Input } from '@/components/ui/input'
import { DataBrowser } from '@/components/menu/data-browser'
import { runAction, runActionQuiet } from '@/lib/actions'
import { isUsable, type CapabilityReport } from '@/lib/bridge'
import { useI18n } from '@/lib/i18n'

// 武器编号按机型区分，只列出该机型武器表里真实存在的条目
const QuickWeapons = [
  { game: 'sa', type: 22, label: 'weapon.colt45' },
  { game: 'sa', type: 24, label: 'weapon.desert_eagle' },
  { game: 'sa', type: 30, label: 'weapon.ak47' },
  { game: 'vc', type: 17, label: 'weapon.colt45' },
  { game: 'vc', type: 18, label: 'weapon.python' },
  { game: 'vc', type: 20, label: 'weapon.mp5' },
  { game: 'iii', type: 17, label: 'weapon.colt45' },
  { game: 'iii', type: 18, label: 'weapon.uzi' },
  { game: 'iii', type: 19, label: 'weapon.m16' },
]

type PageProps = {
  report: CapabilityReport | null
}

export function WeaponPage({ report }: PageProps) {
  const { t } = useI18n()
  const [infiniteAmmo, setInfiniteAmmo] = useState(false)
  const [fastReload, setFastReload] = useState(false)
  const [fireRate, setFireRate] = useState('1')
  const [lockRange, setLockRange] = useState('100')
  const [maxTargets, setMaxTargets] = useState('4')
  const [aimPart, setAimPart] = useState('1')

  return (
    <div className="grid gap-5 lg:grid-cols-2">
      <Card>
        <CardHeader>
          <CardTitle>{t('tab.weapon')}</CardTitle>
          <CardDescription>{t('react.weaponHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-4">
          <Button
            disabled={!isUsable(report, 'weapon.giveAll')}
            onClick={() => void runAction('weapon.giveAll', undefined, 'weapon.getAll')}
          >
            {t('weapon.getAll')}
          </Button>
          <label className="flex items-center justify-between text-sm">
            <span>{t('react.infiniteAmmo')}</span>
            <Switch
              checked={infiniteAmmo}
              disabled={!isUsable(report, 'weapon.infiniteAmmo')}
              onCheckedChange={(checked) => {
                setInfiniteAmmo(checked)
                void runActionQuiet('weapon.infiniteAmmo', { enable: checked })
              }}
            />
          </label>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>{t('react.actions')}</CardTitle>
          <CardDescription>{t('react.actionsHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-wrap gap-2">
          {QuickWeapons.filter((item) => item.game === report?.game).map((item) => (
            <Button
              key={item.label}
              variant="outline"
              disabled={!isUsable(report, 'weapon.give')}
              onClick={() => void runAction('weapon.give', { type: item.type, ammo: 999 }, item.label)}
            >
              {t(item.label)}
            </Button>
          ))}
          <Badge variant="secondary" className="ml-auto">
            {t('react.requiresCapability').replace('%s', 'WeaponGive')}
          </Badge>
        </CardContent>
      </Card>

      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('react.actions')}</CardTitle>
          <CardDescription>{t('react.actionsHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-wrap items-center gap-3">
          <Button
            variant="outline"
            disabled={!isUsable(report, 'weapon.maxSkills')}
            onClick={() => void runAction('weapon.maxSkills', undefined, 'player.maxWeaponSkills')}
          >
            {t('player.maxWeaponSkills')}
          </Button>
          <Button
            variant="outline"
            disabled={!isUsable(report, 'weapon.clearAll')}
            onClick={() => void runAction('weapon.clearAll', undefined, 'weapon.clearWeapons')}
          >
            {t('weapon.clearWeapons')}
          </Button>
          <Button
            variant="outline"
            disabled={!isUsable(report, 'weapon.drop')}
            onClick={() => void runAction('weapon.drop', undefined, 'weapon.dropWeapon')}
          >
            {t('weapon.dropWeapon')}
          </Button>
          <label className="flex items-center gap-2 text-sm">
            <span>{t('weapon.fastReload')}</span>
            <Switch
              checked={fastReload}
              disabled={!isUsable(report, 'weapon.fastReload')}
              onCheckedChange={(checked) => {
                setFastReload(checked)
                void runActionQuiet('weapon.fastReload', { enable: checked })
              }}
            />
          </label>
        </CardContent>
      </Card>

      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('weapon.fireRate')}</CardTitle>
          <CardDescription>{t('react.actionsHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-4">
          <ToggleGrid
            report={report}
            items={[
              { method: 'weapon.statOverrides', label: 'weapon.highDamage' },
              { method: 'weapon.statOverrides', label: 'weapon.longRange' },
              { method: 'weapon.statOverrides', label: 'weapon.rapidFire' },
              { method: 'weapon.statOverrides', label: 'weapon.dualWield' },
              { method: 'weapon.statOverrides', label: 'weapon.moveWhileAiming' },
              { method: 'weapon.statOverrides', label: 'weapon.moveWhileFiring' },
              { method: 'weapon.statOverrides', label: 'weapon.noSpread' },
              { method: 'weapon.statOverrides', label: 'weapon.autoAim' },
            ]}
          />
          <div className="flex items-end gap-3">
            <div className="w-32">
              <div className="mb-2 text-xs text-muted-foreground">{t('weapon.fireRateValue')}</div>
              <Input value={fireRate} onChange={(event) => setFireRate(event.target.value)} inputMode="decimal" />
            </div>
            <Button
              variant="outline"
              disabled={!isUsable(report, 'weapon.statOverrides')}
              onClick={() =>
                void runAction(
                  'weapon.statOverrides',
                  { customFireRate: true, fireRate: Number(fireRate) || 1 },
                  'weapon.fireRate',
                )
              }
            >
              {t('react.apply')}
            </Button>
            <Button
              variant="outline"
              disabled={!isUsable(report, 'weapon.resetStats')}
              onClick={() => void runAction('weapon.resetStats', undefined, 'react.resetStats')}
            >
              {t('react.resetStats')}
            </Button>
          </div>
        </CardContent>
      </Card>
      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('weapon.bulletTrack')}</CardTitle>
          <CardDescription>{t('react.actionsHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-4">
          <ParamToggles
            report={report}
            method="bulletAssist.config"
            items={[
              { method: '', label: 'tracking' },
              { method: '', label: 'throughWalls' },
              { method: '', label: 'hardLock' },
              { method: '', label: 'trackCivilian' },
              { method: '', label: 'trackFriend' },
              { method: '', label: 'trackHostile' },
              { method: '', label: 'trackNeutral' },
              { method: '', label: 'drawPedBounds' },
              { method: '', label: 'drawPedCollision' },
              { method: '', label: 'drawPedSkeleton' },
              { method: '', label: 'drawVehicleBounds' },
              { method: '', label: 'drawVehicleCollision' },
            ]}
          />
          <div className="flex flex-wrap items-end gap-3">
            <div className="w-32">
              <div className="mb-2 text-xs text-muted-foreground">{t('weapon.bulletLockRange')}</div>
              <Input value={lockRange} onChange={(event) => setLockRange(event.target.value)} inputMode="decimal" />
            </div>
            <div className="w-32">
              <div className="mb-2 text-xs text-muted-foreground">{t('weapon.bulletMaxTargets')}</div>
              <Input value={maxTargets} onChange={(event) => setMaxTargets(event.target.value)} inputMode="numeric" />
            </div>
            <div className="w-28">
              <div className="mb-2 text-xs text-muted-foreground">{t('weapon.aimPart')}</div>
              <Input value={aimPart} onChange={(event) => setAimPart(event.target.value)} inputMode="numeric" />
            </div>
            <Button
              variant="outline"
              disabled={!isUsable(report, 'bulletAssist.config')}
              onClick={() =>
                void runAction(
                  'bulletAssist.config',
                  {
                    lockRange: Number(lockRange) || 100,
                    maxTargets: Number(maxTargets) || 4,
                    aimPart: Number(aimPart) || 1,
                  },
                  'weapon.bulletTrack',
                )
              }
            >
              {t('react.apply')}
            </Button>
          </div>
        </CardContent>
      </Card>

      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('weapon.getById')}</CardTitle>
          <CardDescription>{t('react.actionsHint')}</CardDescription>
        </CardHeader>
        <CardContent>
          <DataBrowser
            method="data.weapons"
            disabled={!isUsable(report, 'weapon.give')}
            onPick={(item) =>
              void runAction(
                'weapon.give',
                item.isModel ? { model: item.modelId ?? 0, ammo: 999 } : { type: item.id ?? 0, ammo: 999 },
                item.name,
              )
            }
          />
        </CardContent>
      </Card>

    </div>
  )
}
