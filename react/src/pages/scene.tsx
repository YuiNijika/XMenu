import { useState } from 'react'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { Switch } from '@/components/ui/switch'
import { runAction, runActionQuiet } from '@/lib/actions'
import { isUsable, type CapabilityReport } from '@/lib/bridge'
import { useI18n } from '@/lib/i18n'

type PageProps = {
  report: CapabilityReport | null
}

const StyleKeys = ['scene.fightStyle', 'scene.walkStyle']

export function ScenePage({ report }: PageProps) {
  const { t } = useI18n()
  const [animGroup, setAnimGroup] = useState('')
  const [animName, setAnimName] = useState('')
  const [loop, setLoop] = useState(false)
  const [secondary, setSecondary] = useState(false)
  const [onTargetPed, setOnTargetPed] = useState(false)
  const [particle, setParticle] = useState('')
  const [cutscene, setCutscene] = useState('')
  const [interior, setInterior] = useState('-1')
  const [mission, setMission] = useState('0')
  const [fightStyle, setFightStyle] = useState('0')
  const [walkStyle, setWalkStyle] = useState('0')
  const [freecam, setFreecam] = useState(false)
  const [topDown, setTopDown] = useState(false)
  const [fov, setFov] = useState('70')
  const [freecamSpeed, setFreecamSpeed] = useState('1')
  const [topDownZoom, setTopDownZoom] = useState('40')

  return (
    <div className="grid gap-5 lg:grid-cols-2">
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
          <div className="grid gap-3 md:grid-cols-3">
            <label className="flex items-center justify-between text-sm">
              <span>{t('scene.loop')}</span>
              <Switch checked={loop} onCheckedChange={setLoop} />
            </label>
            <label className="flex items-center justify-between text-sm">
              <span>{t('scene.secondary')}</span>
              <Switch checked={secondary} onCheckedChange={setSecondary} />
            </label>
            <label className="flex items-center justify-between text-sm">
              <span>{t('scene.onTargetPed')}</span>
              <Switch checked={onTargetPed} onCheckedChange={setOnTargetPed} />
            </label>
          </div>
          <div className="flex gap-2">
            <Button
              disabled={!isUsable(report, 'scene.animation')}
              onClick={() =>
                void runAction(
                  'scene.animation',
                  { group: animGroup, name: animName, loop, secondary, onTargetPed },
                  'scene.playAnim',
                )
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

      <Card>
        <CardHeader>
          <CardTitle>{t('scene.missions')}</CardTitle>
          <CardDescription>{t('scene.missionIndex')}</CardDescription>
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
          <div className="grid grid-cols-2 gap-3">
            <div className="flex items-end gap-2">
              <div className="flex-1">
                <div className="mb-2 text-xs text-muted-foreground">{t(StyleKeys[0])}</div>
                <Input value={fightStyle} onChange={(event) => setFightStyle(event.target.value)} inputMode="numeric" />
              </div>
              <Button
                variant="outline"
                disabled={!isUsable(report, 'scene.style')}
                onClick={() => void runAction('scene.style', { fight: Number(fightStyle) || 0 }, 'scene.applyFightStyle')}
              >
                {t('react.apply')}
              </Button>
            </div>
            <div className="flex items-end gap-2">
              <div className="flex-1">
                <div className="mb-2 text-xs text-muted-foreground">{t(StyleKeys[1])}</div>
                <Input value={walkStyle} onChange={(event) => setWalkStyle(event.target.value)} inputMode="numeric" />
              </div>
              <Button
                variant="outline"
                disabled={!isUsable(report, 'scene.style')}
                onClick={() => void runAction('scene.style', { walk: Number(walkStyle) || 0 }, 'scene.applyWalkStyle')}
              >
                {t('react.apply')}
              </Button>
            </div>
          </div>
        </CardContent>
      </Card>

      <Card className="lg:col-span-2">
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
      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('world.freecamControls')}</CardTitle>
          <CardDescription>{t('react.actionsHint')}</CardDescription>
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
  )
}
