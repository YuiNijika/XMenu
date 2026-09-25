import { useEffect, useState } from 'react'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { Switch } from '@/components/ui/switch'
import { SchemaSection } from '@/components/menu/schema-section'
import { DataBrowser } from '@/components/menu/data-browser'
import { Tabs, TabsContent, TabsList, TabsTrigger } from '@/components/ui/tabs'
import { runAction } from '@/lib/actions'
import {
  fetchUiSchema,
  isUsable,
  type CapabilityReport,
  type UiSchemaPayload,
} from '@/lib/bridge'
import { useI18n } from '@/lib/i18n'

type PageProps = {
  report: CapabilityReport | null
}

export function PedPage({ report }: PageProps) {
  const { t } = useI18n()
  const [model, setModel] = useState('0')
  const [atMarker, setAtMarker] = useState(false)
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

  const [pedHealth, setPedHealth] = useState('100')
  const [pedArmour, setPedArmour] = useState('0')
  const [gangId, setGangId] = useState('0')
  const [density, setDensity] = useState('0')
  const [slot, setSlot] = useState('0')
  const [memberModel, setMemberModel] = useState('0')
  const [gangWeapon, setGangWeapon] = useState('0')

  return (
    <div className="flex flex-col gap-5">
      <Tabs defaultValue="toggles" className="w-full">
        <TabsList className="mb-1">
          <TabsTrigger value="toggles">{t('common.toggles')}</TabsTrigger>
          <TabsTrigger value="spawnPed">{t('ped.spawnPed')}</TabsTrigger>
          <TabsTrigger value="gangs">{t('ped.gangs')}</TabsTrigger>
        </TabsList>

        <TabsContent value="toggles">
          {schema ? (
            <Card>
              <CardHeader>
                <CardTitle>{t('common.toggles')}</CardTitle>
                <CardDescription>{t('ped.hint')}</CardDescription>
              </CardHeader>
              <CardContent className="grid gap-4">
                {/* 开关与帮派战争动作都改由注册表绘制 */}
                <SchemaSection payload={schema} report={report} tabId="ped" pageId="pedMain" sectionId="strategies" />
                <SchemaSection payload={schema} report={report} tabId="ped" pageId="pedMain" sectionId="noFire" />
                <SchemaSection payload={schema} report={report} tabId="ped" pageId="pedMain" sectionId="spawnLimits" />
              </CardContent>
            </Card>
          ) : null}
        </TabsContent>

        <TabsContent value="spawnPed">
          <div className="grid gap-5 md:grid-cols-2">
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
                      void runAction(
                        'ped.spawn',
                        {
                          model: Number(model) || 0,
                          atMarker,
                          health: Number(pedHealth) || 100,
                          armour: Number(pedArmour) || 0,
                        },
                        'ped.spawnPed',
                      )
                    }
                  >
                    {t('react.spawn')}
                  </Button>
                </div>
                <label className="flex items-center justify-between text-sm">
                  <span>{t('ped.spawnMarker')}</span>
                  <Switch checked={atMarker} onCheckedChange={setAtMarker} />
                </label>
                <div className="grid grid-cols-2 gap-3">
                  <div>
                    <div className="mb-2 text-xs text-muted-foreground">{t('ped.health')}</div>
                    <Input value={pedHealth} onChange={(event) => setPedHealth(event.target.value)} inputMode="numeric" />
                  </div>
                  <div>
                    <div className="mb-2 text-xs text-muted-foreground">{t('ped.armour')}</div>
                    <Input value={pedArmour} onChange={(event) => setPedArmour(event.target.value)} inputMode="numeric" />
                  </div>
                </div>
                <Button
                  variant="outline"
                  disabled={!isUsable(report, 'ped.deleteLast')}
                  onClick={() => void runAction('ped.deleteLast', undefined, 'ped.deleteLast')}
                >
                  {t('ped.deleteLast')}
                </Button>
              </CardContent>
            </Card>

            {schema ? (
              <Card className="md:col-span-2">
                <CardHeader>
                  <CardTitle>{t('ped.spawnOptions')}</CardTitle>
                  <CardDescription>{t('react.uiHint')}</CardDescription>
                </CardHeader>
                <CardContent className="grid gap-4">
                  <SchemaSection payload={schema} report={report} tabId="ped" pageId="pedMain" sectionId="spawnOptions" />
                </CardContent>
              </Card>
            ) : null}

            <Card className="md:col-span-2">
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
        </TabsContent>

        <TabsContent value="gangs">
          <div className="grid gap-5 md:grid-cols-2">
            {schema ? (
              <Card className="md:col-span-2">
                <CardHeader>
                  <CardTitle>{t('ped.gangWars')}</CardTitle>
                  <CardDescription>{t('react.uiHint')}</CardDescription>
                </CardHeader>
                <CardContent className="grid gap-4">
                  <SchemaSection payload={schema} report={report} tabId="ped" pageId="pedMain" sectionId="gangWars" />
                </CardContent>
              </Card>
            ) : null}

            <Card className="md:col-span-2">
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
                  onClick={() => void runAction('ui.run', { id: 'ped.resetGangModels' }, 'ped.resetGangModels')}
                >
                  {t('ped.resetGangModels')}
                </Button>
              </CardContent>
            </Card>
          </div>
        </TabsContent>
      </Tabs>
    </div>
  )
}
