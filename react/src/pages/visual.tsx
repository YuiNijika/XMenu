import { useEffect, useState } from 'react'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { SchemaSection } from '@/components/menu/schema-section'
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

export function VisualPage({ report }: PageProps) {
  const { t } = useI18n()
  const [filterId, setFilterId] = useState('0')
  const [strength, setStrength] = useState('1')
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

  return (
    <div className="grid gap-5 md:grid-cols-2">
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

      {schema ? (
        <Card className="md:col-span-2">
          <CardHeader>
            <CardTitle>{t('tab.visual')}</CardTitle>
            <CardDescription>{t('react.uiHint')}</CardDescription>
          </CardHeader>
          <CardContent className="grid gap-4">
            <SchemaSection payload={schema} report={report} tabId="visual" pageId="visualMain" sectionId="display" />
            <SchemaSection
              payload={schema}
              report={report}
              tabId="visual"
              pageId="visualMain"
              sectionId="radarOptions"
            />
            <SchemaSection payload={schema} report={report} tabId="visual" pageId="visualMain" sectionId="filter" />
          </CardContent>
        </Card>
      ) : null}
    </div>
  )
}
