import { useEffect, useState } from 'react'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
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

export function WeaponPage({ report }: PageProps) {
  const { t } = useI18n()
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
    <div className="flex flex-col gap-5">
      <Card>
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
            disabled={!isUsable(report, 'weapon.removePickups')}
            onClick={() => void runAction('weapon.removePickups', undefined, 'weapon.removePickups')}
          >
            {t('weapon.removePickups')}
          </Button>
          <Button
            variant="outline"
            disabled={!isUsable(report, 'weapon.drop')}
            onClick={() => void runAction('weapon.drop', undefined, 'weapon.dropWeapon')}
          >
            {t('weapon.dropWeapon')}
          </Button>
        </CardContent>
      </Card>

      <Tabs defaultValue="toggles" className="w-full">
        <TabsList className="mb-1">
          <TabsTrigger value="toggles">{t('common.toggles')}</TabsTrigger>
          <TabsTrigger value="getWeapon">{t('weapon.getWeapon')}</TabsTrigger>
        </TabsList>

        <TabsContent value="toggles">
          <div className="grid gap-5 md:grid-cols-2">
            {schema ? (
              <Card className="md:col-span-2">
                <CardHeader>
                  <CardTitle>{t('common.toggles')}</CardTitle>
                  <CardDescription>{t('react.uiHint')}</CardDescription>
                </CardHeader>
                <CardContent className="grid gap-4">
                  <SchemaSection payload={schema} report={report} tabId="weapon" pageId="weaponMain" sectionId="runtime" />
                  <SchemaSection
                    payload={schema}
                    report={report}
                    tabId="weapon"
                    pageId="weaponMain"
                    sectionId="statOverrides"
                  />
                  <SchemaSection
                    payload={schema}
                    report={report}
                    tabId="weapon"
                    pageId="weaponMain"
                    sectionId="bulletAssist"
                  />
                </CardContent>
              </Card>
            ) : null}

          </div>
        </TabsContent>

        <TabsContent value="getWeapon">
          <div className="grid gap-5 md:grid-cols-2">
            {schema ? (
              <Card className="md:col-span-2">
                <CardHeader>
                  <CardTitle>{t('weapon.giveOptions')}</CardTitle>
                  <CardDescription>{t('react.uiHint')}</CardDescription>
                </CardHeader>
                <CardContent className="grid gap-4">
                  <SchemaSection
                    payload={schema}
                    report={report}
                    tabId="weapon"
                    pageId="weaponMain"
                    sectionId="giveOptions"
                  />
                </CardContent>
              </Card>
            ) : null}

            <Card className="md:col-span-2">
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
        </TabsContent>
      </Tabs>
    </div>
  )
}
