import { useState } from 'react'
import { Badge } from '@/components/ui/badge'
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

type RadarToggle = {
  key: string
  label: string
  capability: string
}

const RadarToggles: RadarToggle[] = [
  { key: 'square', label: 'visual.squareRadar', capability: 'visual.radarOptions' },
  { key: 'noRadarRot', label: 'visual.noRadarRot', capability: 'visual.radarOptions' },
  { key: 'fullscreenMap', label: 'visual.fullscreenMap', capability: 'visual.radarOptions' },
  { key: 'unfogMap', label: 'visual.unfogMap', capability: 'visual.radarOptions' },
  { key: 'hideAreaNames', label: 'visual.hideAreaNames', capability: 'visual.radarOptions' },
  { key: 'hideVehicleNames', label: 'visual.hideVehicleNames', capability: 'visual.radarOptions' },
  { key: 'nightVision', label: 'visual.nightVision', capability: 'visual.radarOptions' },
  { key: 'infrared', label: 'visual.infrared', capability: 'visual.radarOptions' },
]

export function VisualPage({ report }: PageProps) {
  const { t } = useI18n()
  const [hud, setHud] = useState(true)
  const [radar, setRadar] = useState(true)
  const [options, setOptions] = useState<Record<string, boolean>>({})
  const [filterId, setFilterId] = useState('0')
  const [strength, setStrength] = useState('1')

  const toggleOption = (key: string, checked: boolean) => {
    const next = { ...options, [key]: checked }
    setOptions(next)
    void runActionQuiet('visual.radarOptions', next)
  }

  return (
    <div className="grid gap-5 lg:grid-cols-2">
      <Card>
        <CardHeader>
          <CardTitle>{t('tab.visual')}</CardTitle>
          <CardDescription>{t('visual.squareRadarHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-3">
          <label className="flex items-center justify-between text-sm">
            <span>{t('visual.hud')}</span>
            <Switch
              checked={hud}
              disabled={!isUsable(report, 'visual.hud')}
              onCheckedChange={(checked) => {
                setHud(checked)
                void runActionQuiet('visual.hud', { enable: checked })
              }}
            />
          </label>
          <label className="flex items-center justify-between text-sm">
            <span>{t('visual.radar')}</span>
            <Switch
              checked={radar}
              disabled={!isUsable(report, 'visual.radar')}
              onCheckedChange={(checked) => {
                setRadar(checked)
                void runActionQuiet('visual.radar', { enable: checked })
              }}
            />
          </label>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>{t('visual.applyFilter')}</CardTitle>
          <CardDescription>{t('visual.filterHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-wrap items-end gap-3">
          <div className="w-28">
            <div className="mb-2 text-xs text-muted-foreground">{t('visual.filterId')}</div>
            <Input value={filterId} onChange={(event) => setFilterId(event.target.value)} inputMode="numeric" />
          </div>
          <div className="w-28">
            <div className="mb-2 text-xs text-muted-foreground">{t('visual.timecycStrength')}</div>
            <Input value={strength} onChange={(event) => setStrength(event.target.value)} inputMode="decimal" />
          </div>
          <Button
            disabled={!isUsable(report, 'visual.filter')}
            onClick={() =>
              void runAction(
                'visual.filter',
                { id: Number(filterId) || 0, strength: Number(strength) || 1 },
                'visual.applyFilter',
              )
            }
          >
            {t('react.apply')}
          </Button>
        </CardContent>
      </Card>

      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('visual.radar')}</CardTitle>
          <CardDescription>{t('visual.listHint')}</CardDescription>
        </CardHeader>
        <CardContent className="grid gap-3 md:grid-cols-2">
          {RadarToggles.map((item) => (
            <label key={item.key} className="flex items-center justify-between text-sm">
              <span>{t(item.label)}</span>
              <Switch
                checked={options[item.key] ?? false}
                disabled={!isUsable(report, item.capability)}
                onCheckedChange={(checked) => toggleOption(item.key, checked)}
              />
            </label>
          ))}
          <Badge variant="secondary" className="justify-self-start md:col-span-2">
            {report?.gameName ?? t('react.unknown')}
          </Badge>
        </CardContent>
      </Card>
    </div>
  )
}
