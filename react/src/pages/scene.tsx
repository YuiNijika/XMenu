import { useEffect, useState } from 'react'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { Switch } from '@/components/ui/switch'
import { SchemaSection } from '@/components/menu/schema-section'
import { DataBrowser } from '@/components/menu/data-browser'
import { KeyBrowser } from '@/components/menu/key-browser'
import { Tabs, TabsContent, TabsList, TabsTrigger } from '@/components/ui/tabs'
import { runAction, runActionQuiet } from '@/lib/actions'
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

export function ScenePage({ report }: PageProps) {
  const { t } = useI18n()
  const [animGroup, setAnimGroup] = useState('')
  const [animName, setAnimName] = useState('')
  const [particle, setParticle] = useState('')
  const [cutscene, setCutscene] = useState('')
  const [interior, setInterior] = useState('-1')
  const [mission, setMission] = useState('0')
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

  const [freecam, setFreecam] = useState(false)
  const [topDown, setTopDown] = useState(false)
  const [fov, setFov] = useState('70')
  const [freecamSpeed, setFreecamSpeed] = useState('1')
  const [topDownZoom, setTopDownZoom] = useState('40')

  return (
    <div className="flex flex-col gap-5">
      <Tabs defaultValue="animation" className="w-full">
        <TabsList className="mb-1 flex-wrap">
          <TabsTrigger value="animation">{t('scene.animation')}</TabsTrigger>
          <TabsTrigger value="styles">{t('scene.styles')}</TabsTrigger>
          <TabsTrigger value="particle">{t('scene.particle')}</TabsTrigger>
          <TabsTrigger value="cutscene">{t('scene.cutscene')}</TabsTrigger>
          <TabsTrigger value="missions">{t('scene.missions')}</TabsTrigger>
          <TabsTrigger value="camera">{t('world.freecam')}</TabsTrigger>
        </TabsList>

        <TabsContent value="animation">
          <div className={schema ? 'grid gap-5 md:grid-cols-2' : 'flex flex-col gap-5'}>
            <Card>
              <CardHeader>
                <CardTitle>{t('scene.animation')}</CardTitle>
                <CardDescription>{t('scene.animationListHint')}</CardDescription>
              </CardHeader>
              <CardContent className="flex flex-col gap-4">
                <div className="grid grid-cols-2 gap-3">
                  <div>
                    <div className="mb-2 text-xs text-muted-foreground">{t('scene.animGroup')}</div>
                    <Input value={animGroup} onChange={(event) => setAnimGroup(event.target.value)} />
                  </div>
                  <div>
                    <div className="mb-2 text-xs text-muted-foreground">{t('scene.animName')}</div>
                    <Input value={animName} onChange={(event) => setAnimName(event.target.value)} />
                  </div>
                </div>
                {/* 循环等三个开关改由注册表绘制，播放时宿主从 MenuState 读，这里不再传 */}
                <KeyBrowser
                  prefix="scene.animation"
                  game={report?.game}
                  disabled={!isUsable(report, 'scene.animation')}
                  onPick={(segments) => {
                    setAnimGroup(segments[3] ?? '')
                    setAnimName(segments[4] ?? '')
                  }}
                />
                <div className="flex gap-2">
                  <Button
                    disabled={!isUsable(report, 'scene.animation')}
                    onClick={() =>
                      void runAction('scene.animation', { group: animGroup, name: animName }, 'scene.playAnim')
                    }
                  >
                    {t('scene.playAnim')}
                  </Button>
                  <Button
                    variant="outline"
                    disabled={!isUsable(report, 'scene.animation')}
                    onClick={() => void runAction('scene.animation', { stop: true }, 'scene.stopAnim')}
                  >
                    {t('scene.stopAnim')}
                  </Button>
                </div>
              </CardContent>
            </Card>

            {schema ? (
              <Card className="md:col-span-2">
                <CardHeader>
                  <CardTitle>{t('scene.animation')}</CardTitle>
                  <CardDescription>{t('react.uiHint')}</CardDescription>
                </CardHeader>
                <CardContent className="grid gap-4">
                  <SchemaSection payload={schema} report={report} tabId="scene" pageId="sceneMain" sectionId="animation" />
                </CardContent>
              </Card>
            ) : null}
          </div>
        </TabsContent>

        <TabsContent value="styles">
          <Card>
            <CardHeader>
              <CardTitle>{t('scene.styles')}</CardTitle>
              <CardDescription>{t('react.uiHint')}</CardDescription>
            </CardHeader>
            <CardContent className="grid gap-4">
              {report?.game === 'sa' && schema ? (
                <SchemaSection payload={schema} report={report} tabId="scene" pageId="sceneMain" sectionId="styles" />
              ) : (
                <p className="text-xs text-muted-foreground">{t('react.unsupportedGame')}</p>
              )}
            </CardContent>
          </Card>
        </TabsContent>

        <TabsContent value="particle">
          <Card>
            <CardHeader>
              <CardTitle>{t('scene.particle')}</CardTitle>
              <CardDescription>{t('scene.particleListHint')}</CardDescription>
            </CardHeader>
            <CardContent className="flex flex-col gap-4">
              <div className="flex items-end gap-3">
                <div className="flex-1">
                  <div className="mb-2 text-xs text-muted-foreground">{t('scene.particleName')}</div>
                  <Input value={particle} onChange={(event) => setParticle(event.target.value)} />
                </div>
                <Button
                  disabled={!isUsable(report, 'scene.particle')}
                  onClick={() => void runAction('scene.particle', { name: particle }, 'scene.spawnParticle')}
                >
                  {t('scene.spawnParticle')}
                </Button>
              </div>
              <KeyBrowser
                prefix="scene.particle"
                game={report?.game}
                disabled={!isUsable(report, 'scene.particle')}
                onPick={(segments) => setParticle(segments[4] ?? '')}
              />
              <div className="flex flex-wrap gap-2">
                <Button
                  variant="outline"
                  disabled={!isUsable(report, 'scene.particle')}
                  onClick={() => void runAction('scene.particle', { removeLast: true }, 'scene.removeLastParticle')}
                >
                  {t('scene.removeLastParticle')}
                </Button>
                <Button
                  variant="outline"
                  disabled={!isUsable(report, 'scene.particle')}
                  onClick={() => void runAction('scene.particle', { removeAll: true }, 'scene.removeAllParticles')}
                >
                  {t('scene.removeAllParticles')}
                </Button>
              </div>
            </CardContent>
          </Card>
        </TabsContent>

        <TabsContent value="cutscene">
          <Card>
            <CardHeader>
              <CardTitle>{t('scene.cutscene')}</CardTitle>
              <CardDescription>{t('scene.cutsceneListHint')}</CardDescription>
            </CardHeader>
            <CardContent className="flex flex-col gap-4">
              <div className="grid grid-cols-3 gap-3">
                <div className="col-span-2">
                  <div className="mb-2 text-xs text-muted-foreground">{t('scene.cutsceneName')}</div>
                  <Input value={cutscene} onChange={(event) => setCutscene(event.target.value)} />
                </div>
                <div>
                  <div className="mb-2 text-xs text-muted-foreground">{t('scene.interiorId')}</div>
                  <Input value={interior} onChange={(event) => setInterior(event.target.value)} inputMode="numeric" />
                </div>
              </div>
              <KeyBrowser
                prefix="scene.cutscene"
                game={report?.game}
                disabled={!isUsable(report, 'scene.cutscene')}
                onPick={(segments) => setCutscene(segments[4] ?? '')}
              />
              <div className="flex flex-wrap gap-2">
                <Button
                  disabled={!isUsable(report, 'scene.cutscene')}
                  onClick={() =>
                    void runAction(
                      'scene.cutscene',
                      { name: cutscene, interior: Number(interior) },
                      'scene.startCutscene',
                    )
                  }
                >
                  {t('scene.startCutscene')}
                </Button>
                <Button
                  variant="outline"
                  disabled={!isUsable(report, 'scene.cutscene')}
                  onClick={() => void runAction('scene.cutscene', { stop: true }, 'scene.stopCutscene')}
                >
                  {t('scene.stopCutscene')}
                </Button>
              </div>
            </CardContent>
          </Card>
        </TabsContent>

        <TabsContent value="missions">
          <Card>
            <CardHeader>
              <CardTitle>{t('scene.missions')}</CardTitle>
              <CardDescription>{t('scene.missionListHint')}</CardDescription>
            </CardHeader>
            <CardContent className="flex flex-col gap-4">
              <div className="flex items-end gap-3">
                <div className="w-28">
                  <div className="mb-2 text-xs text-muted-foreground">{t('scene.missionIndex')}</div>
                  <Input value={mission} onChange={(event) => setMission(event.target.value)} inputMode="numeric" />
                </div>
                <Button
                  disabled={!isUsable(report, 'scene.mission')}
                  onClick={() => void runAction('scene.mission', { id: Number(mission) || 0 }, 'scene.startMission')}
                >
                  {t('scene.startMission')}
                </Button>
                <Button
                  variant="outline"
                  disabled={!isUsable(report, 'scene.mission')}
                  onClick={() => void runAction('scene.mission', { fail: true }, 'scene.failMission')}
                >
                  {t('scene.failMission')}
                </Button>
              </div>
              <DataBrowser
                method="data.missions"
                disabled={!isUsable(report, 'scene.mission')}
                onPick={(item) => void runAction('scene.mission', { id: item.id ?? 0 }, 'scene.startMission')}
              />
            </CardContent>
          </Card>
        </TabsContent>

        <TabsContent value="camera">
          <div className="grid gap-5 md:grid-cols-2">
            <Card className="md:col-span-2">
              <CardHeader>
                <CardTitle>{t('world.freecam')}</CardTitle>
                <CardDescription>{t('world.freecamControls')}</CardDescription>
              </CardHeader>
              <CardContent className="grid gap-3 md:grid-cols-2">
                <label className="flex items-center justify-between text-sm">
                  <span>{t('world.freecam')}</span>
                  <Switch
                    checked={freecam}
                    disabled={!isUsable(report, 'camera.freecam')}
                    onCheckedChange={(checked) => {
                      setFreecam(checked)
                      void runActionQuiet('camera.freecam', { enable: checked })
                    }}
                  />
                </label>
                <label className="flex items-center justify-between text-sm">
                  <span>{t('world.topDownCam')}</span>
                  <Switch
                    checked={topDown}
                    disabled={!isUsable(report, 'camera.topDown')}
                    onCheckedChange={(checked) => {
                      setTopDown(checked)
                      void runActionQuiet('camera.topDown', { enable: checked })
                    }}
                  />
                </label>
              </CardContent>
            </Card>
            <Card className="md:col-span-2">
              <CardHeader>
                <CardTitle>{t('settings.runtime')}</CardTitle>
                <CardDescription>{t('world.freecamControls')}</CardDescription>
              </CardHeader>
              <CardContent className="flex flex-wrap items-end gap-3">
                <div className="w-28">
                  <div className="mb-2 text-xs text-muted-foreground">{t('world.freecamFov')}</div>
                  <Input value={fov} onChange={(event) => setFov(event.target.value)} inputMode="decimal" />
                </div>
                <div className="w-28">
                  <div className="mb-2 text-xs text-muted-foreground">{t('world.freecamSpeedMul')}</div>
                  <Input value={freecamSpeed} onChange={(event) => setFreecamSpeed(event.target.value)} inputMode="numeric" />
                </div>
                <div className="w-28">
                  <div className="mb-2 text-xs text-muted-foreground">{t('world.topDownCamZoom')}</div>
                  <Input value={topDownZoom} onChange={(event) => setTopDownZoom(event.target.value)} inputMode="numeric" />
                </div>
                <Button
                  variant="outline"
                  disabled={!isUsable(report, 'camera.settings')}
                  onClick={() =>
                    void runAction(
                      'camera.settings',
                      {
                        freecamFov: Number(fov) || 70,
                        freecamSpeed: Number(freecamSpeed) || 1,
                        topDownZoom: Number(topDownZoom) || 40,
                      },
                      'world.freecamControls',
                    )
                  }
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
