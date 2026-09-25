import { useEffect, useState } from 'react'
import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { Slider } from '@/components/ui/slider'
import { ToggleGrid } from '@/components/menu/toggle-grid'
import { SchemaSection } from '@/components/menu/schema-section'
import { Switch } from '@/components/ui/switch'
import { Tabs, TabsContent, TabsList, TabsTrigger } from '@/components/ui/tabs'
import { runAction, runActionQuiet } from '@/lib/actions'
import {
  fetchUiSchema,
  isUsable,
  type CapabilityReport,
  type PlayerSnapshot,
  type UiSchemaPayload,
} from '@/lib/bridge'
import { usePolling } from '@/lib/hooks'
import { useI18n } from '@/lib/i18n'

const ProofFlags = [
  { key: 'bullet', label: 'proof.bullet' },
  { key: 'fire', label: 'proof.fire' },
  { key: 'explosion', label: 'proof.explosion' },
  { key: 'collision', label: 'proof.collision' },
  { key: 'melee', label: 'proof.melee' },
]

const RuntimeFlags = [
  { key: 'autoHeal', label: 'player.autoHeal' },
  { key: 'respawnAtDeathPosition', label: 'player.respawnAtDeathPosition' },
  { key: 'freezeWantedLevel', label: 'player.freezeWantedLevel' },
  { key: 'freeFlyProtection', label: 'player.autoFlight' },
]

type PageProps = {
  report: CapabilityReport | null
}

export function PlayerPage({ report }: PageProps) {
  const { value } = usePolling<PlayerSnapshot>('player.snapshot', 400)
  const { t } = useI18n()
  const [amount, setAmount] = useState('1000')
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

  const [wanted, setWanted] = useState([0])
  const [skin, setSkin] = useState('0')
  const [health, setHealth] = useState('100')
  const [proofs, setProofs] = useState<Record<string, boolean>>({})
  const [runtime, setRuntime] = useState<Record<string, boolean>>({})
  const [texture, setTexture] = useState('0')
  const [clothesModel, setClothesModel] = useState('0')
  const [bodyPart, setBodyPart] = useState('0')
  const [skinName, setSkinName] = useState('')
  const [statId, setStatId] = useState('0')
  const [statValue, setStatValue] = useState('0')

  return (
    <div className="flex flex-col gap-5">
      <Card>
        <CardHeader>
          <CardTitle>{t('react.status')}</CardTitle>
          <CardDescription>{t('react.statusHint')}</CardDescription>
        </CardHeader>
        <CardContent className="grid grid-cols-2 gap-4 text-sm">
          <Stat label={t('player.health')} value={value ? value.health.toFixed(0) : '--'} />
          <Stat label={t('player.armor')} value={value ? value.armour.toFixed(0) : '--'} />
          <Stat label={t('player.money')} value={value ? String(value.money) : '--'} />
          <Stat label={t('player.wantedLevel')} value={value ? String(value.wantedLevel) : '--'} />
          <Stat
            label={t('react.position')}
            value={
              value
                ? `${value.position.x.toFixed(0)} ${value.position.y.toFixed(0)} ${value.position.z.toFixed(0)}`
                : '--'
            }
          />
          <Stat label={t('react.valid')} value={value?.valid ? t('react.yes') : t('react.no')} />
        </CardContent>
      </Card>

      <Tabs defaultValue="toggles" className="w-full">
        <TabsList className="mb-1">
          <TabsTrigger value="toggles">{t('common.toggles')}</TabsTrigger>
          <TabsTrigger value="values">{t('player.valueAdjustments')}</TabsTrigger>
          <TabsTrigger value="appearance">{t('player.appearance')}</TabsTrigger>
          <TabsTrigger value="skins">{t('player.customSkins')}</TabsTrigger>
        </TabsList>

        <TabsContent value="toggles">
          <div className="grid gap-5 md:grid-cols-2">
            {schema ? (
              <Card className="md:col-span-2">
                <CardHeader>
                  <CardTitle>{t('player.sectionActions')}</CardTitle>
                  <CardDescription>{t('react.uiHint')}</CardDescription>
                </CardHeader>
                <CardContent className="grid gap-4">
                  <SchemaSection payload={schema} report={report} tabId="player" pageId="playerMain" sectionId="actions" />
                  <SchemaSection
                    payload={schema}
                    report={report}
                    tabId="player"
                    pageId="playerMain"
                    sectionId="statusToggles"
                  />
                  <SchemaSection payload={schema} report={report} tabId="player" pageId="playerMain" sectionId="flight" />
                </CardContent>
              </Card>
            ) : null}

            <Card className="md:col-span-2">
              <CardHeader>
                <CardTitle>{t('common.toggles')}</CardTitle>
                <CardDescription>{t('react.actionsHint')}</CardDescription>
              </CardHeader>
              <CardContent className="flex flex-col gap-4">
                {/* 已登记的开关改由注册表绘制，这里只留没进注册表的两项 */}
                <ToggleGrid
                  report={report}
                  items={[
                    { method: 'player.freeHealthcare', label: 'player.freeHospital' },
                    { method: 'player.freeJail', label: 'player.freeJail' },
                  ]}
                />
                <div className="flex flex-wrap gap-2">
                  <Button
                    variant="outline"
                    disabled={!isUsable(report, 'player.clearWanted')}
                    onClick={() => void runAction('player.clearWanted', undefined, 'player.clearWanted')}
                  >
                    {t('player.clearWanted')}
                  </Button>
                  <Button
                    variant="outline"
                    disabled={!isUsable(report, 'player.aimSkin')}
                    onClick={() => void runAction('player.aimSkin', undefined, 'player.aimSkinChanger')}
                  >
                    {t('player.aimSkinChanger')}
                  </Button>
                </div>
              </CardContent>
            </Card>

            <Card className="md:col-span-2">
              <CardHeader>
                <CardTitle>{t('player.proofFlags')}</CardTitle>
                <CardDescription>{t('react.actionsHint')}</CardDescription>
              </CardHeader>
              <CardContent className="grid gap-3 md:grid-cols-3">
                {ProofFlags.map((flag) => (
                  <label key={flag.key} className="flex items-center justify-between text-sm">
                    <span>{t(flag.label)}</span>
                    <Switch
                      checked={proofs[flag.key] ?? false}
                      disabled={!isUsable(report, 'player.proofs')}
                      onCheckedChange={(checked) => {
                        setProofs((previous) => ({ ...previous, [flag.key]: checked }))
                        void runActionQuiet('player.proofs', { [flag.key]: checked })
                      }}
                    />
                  </label>
                ))}
              </CardContent>
            </Card>

            <Card className="md:col-span-2">
              <CardHeader>
                <CardTitle>{t('settings.runtime')}</CardTitle>
                <CardDescription>{t('react.actionsHint')}</CardDescription>
              </CardHeader>
              <CardContent className="grid gap-3 md:grid-cols-2">
                {RuntimeFlags.map((flag) => (
                  <label key={flag.key} className="flex items-center justify-between text-sm">
                    <span>{t(flag.label)}</span>
                    <Switch
                      checked={runtime[flag.key] ?? false}
                      disabled={!isUsable(report, 'player.runtimeOptions')}
                      onCheckedChange={(checked) => {
                        setRuntime((previous) => ({ ...previous, [flag.key]: checked }))
                        void runActionQuiet('player.runtimeOptions', { [flag.key]: checked })
                      }}
                    />
                  </label>
                ))}
              </CardContent>
            </Card>
          </div>
        </TabsContent>

        <TabsContent value="values">
          <div className="grid gap-5 md:grid-cols-2">
            <Card className="md:col-span-2">
              <CardHeader>
                <CardTitle>{t('react.actions')}</CardTitle>
                <CardDescription>{t('react.actionsHint')}</CardDescription>
              </CardHeader>
              <CardContent className="flex flex-col gap-4">
                <div className="flex items-end gap-3">
                  <div className="flex-1">
                    <div className="mb-2 text-xs text-muted-foreground">{t('player.money')}</div>
                    <Input value={amount} onChange={(event) => setAmount(event.target.value)} inputMode="numeric" />
                  </div>
                  <Button
                    disabled={!isUsable(report, 'player.money')}
                    onClick={() => void runAction('player.money', { amount: Number(amount) || 0 }, 'player.setMoney')}
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
                    disabled={!isUsable(report, 'player.wanted')}
                    onValueChange={(next) => {
                      const value = Array.isArray(next) ? [...next] : [next]
                      setWanted(value)
                      void runActionQuiet('player.wanted', { level: value[0] })
                    }}
                  />
                </div>
              </CardContent>
            </Card>

            <Card>
              <CardHeader>
                <CardTitle>{t('player.health')}</CardTitle>
                <CardDescription>{t('react.actionsHint')}</CardDescription>
              </CardHeader>
              <CardContent className="flex items-end gap-3">
                <div className="w-32">
                  <div className="mb-2 text-xs text-muted-foreground">{t('player.health')}</div>
                  <Input value={health} onChange={(event) => setHealth(event.target.value)} inputMode="numeric" />
                </div>
                <Button
                  variant="outline"
                  disabled={!isUsable(report, 'player.setHealth')}
                  onClick={() => void runAction('player.setHealth', { value: Number(health) || 0 }, 'player.health')}
                >
                  {t('react.set')}
                </Button>
              </CardContent>
            </Card>

            <Card>
              <CardHeader>
                <CardTitle>{t('player.clearWanted')}</CardTitle>
                <CardDescription>{t('react.actionsHint')}</CardDescription>
              </CardHeader>
              <CardContent className="flex flex-wrap gap-2">
                <Button
                  variant="outline"
                  disabled={!isUsable(report, 'player.clearWanted')}
                  onClick={() => void runAction('player.clearWanted', undefined, 'player.clearWanted')}
                >
                  {t('player.clearWanted')}
                </Button>
              </CardContent>
            </Card>
          </div>
        </TabsContent>

        <TabsContent value="appearance">
          <div className="grid gap-5 md:grid-cols-2">
            <Card className="md:col-span-2">
              <CardHeader>
                <CardTitle>{t('player.appearance')}</CardTitle>
                <CardDescription>{t('react.actionsHint')}</CardDescription>
              </CardHeader>
              <CardContent className="flex flex-wrap items-end gap-3">
                <div className="w-28">
                  <div className="mb-2 text-xs text-muted-foreground">{t('ped.modelId')}</div>
                  <Input value={skin} onChange={(event) => setSkin(event.target.value)} inputMode="numeric" />
                </div>
                <Button
                  variant="outline"
                  disabled={!isUsable(report, 'player.skin')}
                  onClick={() => void runAction('player.skin', { model: Number(skin) || 0 }, 'player.applySkin')}
                >
                  {t('player.applySkin')}
                </Button>
              </CardContent>
            </Card>

            <Card className="md:col-span-2">
              <CardHeader>
                <CardTitle>{t('player.applyClothes')}</CardTitle>
                <CardDescription>{t('react.actionsHint')}</CardDescription>
              </CardHeader>
              <CardContent className="flex flex-wrap items-end gap-3">
                <div className="w-28">
                  <div className="mb-2 text-xs text-muted-foreground">{t('player.clothesTexture')}</div>
                  <Input value={texture} onChange={(event) => setTexture(event.target.value)} inputMode="numeric" />
                </div>
                <div className="w-28">
                  <div className="mb-2 text-xs text-muted-foreground">{t('player.clothesModel')}</div>
                  <Input value={clothesModel} onChange={(event) => setClothesModel(event.target.value)} inputMode="numeric" />
                </div>
                <div className="w-28">
                  <div className="mb-2 text-xs text-muted-foreground">{t('player.clothesBodyPart')}</div>
                  <Input value={bodyPart} onChange={(event) => setBodyPart(event.target.value)} inputMode="numeric" />
                </div>
                <Button
                  variant="outline"
                  disabled={!isUsable(report, 'player.clothes')}
                  onClick={() =>
                    void runAction(
                      'player.clothes',
                      {
                        textureId: Number(texture) || 0,
                        modelId: Number(clothesModel) || 0,
                        bodyPart: Number(bodyPart) || 0,
                      },
                      'player.applyClothes',
                    )
                  }
                >
                  {t('react.apply')}
                </Button>
              </CardContent>
            </Card>

            <Card className="md:col-span-2">
              <CardHeader>
                <CardTitle>{t('player.setStat')}</CardTitle>
                <CardDescription>{t('react.actionsHint')}</CardDescription>
              </CardHeader>
              <CardContent className="flex flex-wrap items-end gap-3">
                <div className="w-24">
                  <div className="mb-2 text-xs text-muted-foreground">{t('player.statId')}</div>
                  <Input value={statId} onChange={(event) => setStatId(event.target.value)} inputMode="numeric" />
                </div>
                <div className="w-28">
                  <div className="mb-2 text-xs text-muted-foreground">{t('player.statValue')}</div>
                  <Input value={statValue} onChange={(event) => setStatValue(event.target.value)} inputMode="decimal" />
                </div>
                <Button
                  variant="outline"
                  disabled={!isUsable(report, 'player.stat')}
                  onClick={() =>
                    void runAction(
                      'player.stat',
                      { id: Number(statId) || 0, value: Number(statValue) || 0 },
                      'player.setStat',
                    )
                  }
                >
                  {t('react.apply')}
                </Button>
                <Button
                  variant="outline"
                  disabled={!isUsable(report, 'player.maxWeaponSkills')}
                  onClick={() => void runAction('player.maxWeaponSkills', undefined, 'player.maxWeaponSkills')}
                >
                  {t('player.maxWeaponSkills')}
                </Button>
                <Button
                  variant="outline"
                  disabled={!isUsable(report, 'player.maxVehicleSkills')}
                  onClick={() => void runAction('player.maxVehicleSkills', undefined, 'player.maxVehicleSkills')}
                >
                  {t('player.maxVehicleSkills')}
                </Button>
              </CardContent>
            </Card>
          </div>
        </TabsContent>

        <TabsContent value="skins">
          <div className="grid gap-5 md:grid-cols-2">
            <Card className="md:col-span-2">
              <CardHeader>
                <CardTitle>{t('player.customSkins')}</CardTitle>
                <CardDescription>{t('player.customSkinsTip')}</CardDescription>
              </CardHeader>
              <CardContent className="flex flex-wrap items-end gap-3">
                <div className="w-40">
                  <div className="mb-2 text-xs text-muted-foreground">{t('player.customSkins')}</div>
                  <Input value={skinName} onChange={(event) => setSkinName(event.target.value)} />
                </div>
                <Button
                  variant="outline"
                  disabled={!isUsable(report, 'player.customSkin')}
                  onClick={() => void runAction('player.customSkin', { name: skinName }, 'player.customSkins')}
                >
                  {t('react.apply')}
                </Button>
              </CardContent>
            </Card>
          </div>
        </TabsContent>
      </Tabs>
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
