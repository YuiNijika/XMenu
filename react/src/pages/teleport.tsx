import { useEffect, useState } from 'react'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { SchemaSection } from '@/components/menu/schema-section'
import { DataBrowser } from '@/components/menu/data-browser'
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

export function TeleportPage({ report }: PageProps) {
  const { t } = useI18n()
  const [schema, setSchema] = useState<UiSchemaPayload | null>(null)

  // 界面注册表只取一次，网页端与 ImGui 从此共用同一份控件与能力门控
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

  const [x, setX] = useState('0')
  const [y, setY] = useState('0')
  const [z, setZ] = useState('0')
  const [distance, setDistance] = useState('50')
  const [mapX, setMapX] = useState('0')
  const [mapY, setMapY] = useState('0')

  return (
    <div className="grid gap-5 lg:grid-cols-2">
      <Card>
        <CardHeader>
          <CardTitle>{t('react.teleportTo')}</CardTitle>
          <CardDescription>{t('react.teleportHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-4">
          <div className="grid grid-cols-3 gap-3">
            <Field label="X" value={x} onChange={setX} />
            <Field label="Y" value={y} onChange={setY} />
            <Field label="Z" value={z} onChange={setZ} />
          </div>
          <Button
            disabled={!isUsable(report, 'teleport.to')}
            onClick={() =>
              void runAction(
                'teleport.to',
                { x: Number(x) || 0, y: Number(y) || 0, z: Number(z) || 0 },
                'react.teleportTo',
              )
            }
          >
            {t('react.teleport')}
          </Button>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>{t('react.quickTeleport')}</CardTitle>
          <CardDescription>{t('react.quickTeleportHint')}</CardDescription>
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
              onClick={() => void runAction('teleport.forward', { distance: Number(distance) || 0 }, 'react.forward')}
            >
              {t('react.forward')}
            </Button>
          </div>
          <Button
            disabled={!isUsable(report, 'teleport.marker')}
            onClick={() => void runAction('teleport.marker', { underwater: false }, 'teleport.toMarker')}
          >
            {t('teleport.toMarker')}
          </Button>
        </CardContent>
      </Card>
      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('react.mapPosition')}</CardTitle>
          <CardDescription>{t('react.teleportHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-wrap items-end gap-3">
          <div className="w-28">
            <div className="mb-2 text-xs text-muted-foreground">X</div>
            <Input value={mapX} onChange={(event) => setMapX(event.target.value)} inputMode="numeric" />
          </div>
          <div className="w-28">
            <div className="mb-2 text-xs text-muted-foreground">Y</div>
            <Input value={mapY} onChange={(event) => setMapY(event.target.value)} inputMode="numeric" />
          </div>
          <Button
            disabled={!isUsable(report, 'teleport.mapPosition')}
            onClick={() =>
              void runAction(
                'teleport.mapPosition',
                { x: Number(mapX) || 0, y: Number(mapY) || 0 },
                'react.mapPosition',
              )
            }
          >
            {t('react.teleport')}
          </Button>
          <Button
            variant="outline"
            disabled={!isUsable(report, 'teleport.center')}
            onClick={() => void runAction('teleport.center', undefined, 'react.teleportCenter')}
          >
            {t('react.teleportCenter')}
          </Button>
        </CardContent>
      </Card>
      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('teleport.coordinates')}</CardTitle>
          <CardDescription>{t('scene.listHint')}</CardDescription>
        </CardHeader>
        <CardContent>
          <DataBrowser
            method="data.locations"
            disabled={!isUsable(report, 'teleport.to')}
            onPick={(item) =>
              void runAction('teleport.to', { x: item.x ?? 0, y: item.y ?? 0, z: item.z ?? 0 }, item.name)
            }
          />
        </CardContent>
      </Card>

      {schema ? (
        <Card className="lg:col-span-2">
          <CardHeader>
            <CardTitle>{t('teleport.sectionQuickOptions')}</CardTitle>
            <CardDescription>{t('react.uiHint')}</CardDescription>
          </CardHeader>
          <CardContent className="grid gap-4">
            <SchemaSection
              payload={schema}
              report={report}
              tabId="teleport"
              pageId="teleportMain"
              sectionId="quickOptions"
            />
            <SchemaSection payload={schema} report={report} tabId="teleport" pageId="teleportMain" sectionId="forward" />
          </CardContent>
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
