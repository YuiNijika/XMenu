import { useState } from 'react'
import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { Switch } from '@/components/ui/switch'
import { ToggleGrid } from '@/components/menu/toggle-grid'
import { DataBrowser } from '@/components/menu/data-browser'
import { runAction, runActionQuiet } from '@/lib/actions'
import { isUsable, type CapabilityReport } from '@/lib/bridge'
import { useI18n } from '@/lib/i18n'

type PageProps = {
  report: CapabilityReport | null
}

export function PedPage({ report }: PageProps) {
  const { t } = useI18n()
  const [model, setModel] = useState('0')
  const [atMarker, setAtMarker] = useState(false)
  const [noFire, setNoFire] = useState(false)
  const [gangId, setGangId] = useState('0')
  const [density, setDensity] = useState('0')
  const [slot, setSlot] = useState('0')
  const [memberModel, setMemberModel] = useState('0')
  const [gangWeapon, setGangWeapon] = useState('0')

  return (
    <div className="grid gap-5 lg:grid-cols-2">
      <Card>
        <CardHeader>
          <CardTitle>{t('ped.spawnPed')}</CardTitle>
          <CardDescription>{t('ped.hint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-4">
          <div className="flex items-end gap-3">
            <div className="flex-1">
              <div className="mb-2 text-xs text-muted-foreground">{t('ped.modelId')}</div>
              <Input value={model} onChange={(event) => setModel(event.target.value)} inputMode="numeric" />
            </div>
            <Button
              disabled={!isUsable(report, 'ped.spawn')}
              onClick={() =>
                void runAction('ped.spawn', { model: Number(model) || 0, atMarker }, 'ped.spawnPed')
              }
            >
              {t('react.spawn')}
            </Button>
          </div>
          <label className="flex items-center justify-between text-sm">
            <span>{t('ped.spawnMarker')}</span>
            <Switch checked={atMarker} onCheckedChange={setAtMarker} />
          </label>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>{t('react.actions')}</CardTitle>
          <CardDescription>{t('react.actionsHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-4">
          <Button
            variant="outline"
            disabled={!isUsable(report, 'ped.deleteLast')}
            onClick={() => void runAction('ped.deleteLast', undefined, 'ped.deleteLast')}
          >
            {t('ped.deleteLast')}
          </Button>
          <label className="flex items-center justify-between text-sm">
            <span>{t('ped.pedsNoFire')}</span>
            <Switch
              checked={noFire}
              disabled={!isUsable(report, 'ped.noFire')}
              onCheckedChange={(checked) => {
                setNoFire(checked)
                void runActionQuiet('ped.noFire', { enable: checked })
              }}
            />
          </label>
          <Badge variant="secondary" className="self-start">
            {report?.gameName ?? t('react.unknown')}
          </Badge>
        </CardContent>
      </Card>

      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('common.toggles')}</CardTitle>
          <CardDescription>{t('ped.hint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-4">
          <ToggleGrid
            report={report}
            items={[
              { method: 'ped.bigHead', label: 'ped.bigHeadMode' },
              { method: 'ped.thinBody', label: 'ped.thinBodyMode' },
              { method: 'ped.flies', label: 'ped.flies' },
              { method: 'ped.smoking', label: 'ped.smoking' },
              { method: 'ped.everyoneArmed', label: 'ped.everyoneArmed' },
              { method: 'ped.mayhem', label: 'ped.pedsMayhem' },
              { method: 'ped.riot', label: 'ped.pedsRiot' },
              { method: 'ped.atkRocket', label: 'ped.pedsAtkRocket' },
              { method: 'ped.elvis', label: 'ped.elvisEverywhere' },
              { method: 'ped.slutMagnet', label: 'ped.slutMagnet' },
              { method: 'ped.nastyLimbs', label: 'ped.nastyLimbs' },
              { method: 'ped.noProstitutes', label: 'ped.noProstitutes' },
              { method: 'ped.gangsEverywhere', label: 'ped.gangsEverywhere' },
              { method: 'ped.gangsControl', label: 'ped.gangsControl' },
              { method: 'ped.gangWars', label: 'ped.gangWarsActive' },
            ]}
          />
          <div className="flex flex-wrap gap-2">
            <Button
              variant="outline"
              disabled={!isUsable(report, 'ped.gangWarStart')}
              onClick={() => void runAction('ped.gangWarStart', { offensive: true }, 'ped.startGangWar')}
            >
              {t('ped.startGangWar')}
            </Button>
            <Button
              variant="outline"
              disabled={!isUsable(report, 'ped.gangWarEnd')}
              onClick={() => void runAction('ped.gangWarEnd', undefined, 'ped.endGangWar')}
            >
              {t('ped.endGangWar')}
            </Button>
          </div>
        </CardContent>
      </Card>
      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('ped.gangs')}</CardTitle>
          <CardDescription>{t('ped.listHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-wrap items-end gap-3">
          <div className="w-24">
            <div className="mb-2 text-xs text-muted-foreground">{t('ped.gangType')}</div>
            <Input value={gangId} onChange={(event) => setGangId(event.target.value)} inputMode="numeric" />
          </div>
          <div className="w-24">
            <div className="mb-2 text-xs text-muted-foreground">{t('ped.gangDensity')}</div>
            <Input value={density} onChange={(event) => setDensity(event.target.value)} inputMode="numeric" />
          </div>
          <Button
            variant="outline"
            disabled={!isUsable(report, 'ped.gangDensity')}
            onClick={() =>
              void runAction(
                'ped.gangDensity',
                { gangId: Number(gangId) || 0, density: Number(density) || 0 },
                'ped.gangDensity',
              )
            }
          >
            {t('react.apply')}
          </Button>
          <div className="w-24">
            <div className="mb-2 text-xs text-muted-foreground">{t('ped.gangMemberIndex')}</div>
            <Input value={slot} onChange={(event) => setSlot(event.target.value)} inputMode="numeric" />
          </div>
          <div className="w-28">
            <div className="mb-2 text-xs text-muted-foreground">{t('ped.gangMemberModel')}</div>
            <Input value={memberModel} onChange={(event) => setMemberModel(event.target.value)} inputMode="numeric" />
          </div>
          <Button
            variant="outline"
            disabled={!isUsable(report, 'ped.gangMemberModel')}
            onClick={() =>
              void runAction(
                'ped.gangMemberModel',
                { gangId: Number(gangId) || 0, slot: Number(slot) || 0, modelId: Number(memberModel) || 0 },
                'ped.gangMemberModel',
              )
            }
          >
            {t('react.apply')}
          </Button>
          <div className="w-28">
            <div className="mb-2 text-xs text-muted-foreground">{t('ped.weaponModel')}</div>
            <Input value={gangWeapon} onChange={(event) => setGangWeapon(event.target.value)} inputMode="numeric" />
          </div>
          <Button
            variant="outline"
            disabled={!isUsable(report, 'ped.gangWeapons')}
            onClick={() =>
              void runAction(
                'ped.gangWeapons',
                { gangId: Number(gangId) || 0, weapon1: Number(gangWeapon) || 0 },
                'ped.applyGangWeapons',
              )
            }
          >
            {t('react.apply')}
          </Button>
          <Button
            variant="outline"
            disabled={!isUsable(report, 'ped.resetGangModels')}
            onClick={() => void runAction('ped.resetGangModels', undefined, 'ped.resetGangModels')}
          >
            {t('ped.resetGangModels')}
          </Button>
        </CardContent>
      </Card>

      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('ped.listHint')}</CardTitle>
          <CardDescription>{t('ped.spawnPed')}</CardDescription>
        </CardHeader>
        <CardContent>
          <DataBrowser
            method="data.peds"
            disabled={!isUsable(report, 'ped.spawn')}
            onPick={(item) => void runAction('ped.spawn', { model: item.id ?? 0, atMarker: false }, item.name)}
          />
        </CardContent>
      </Card>

    </div>
  )
}
