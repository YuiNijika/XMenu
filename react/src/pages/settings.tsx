import { useEffect, useState } from 'react'
import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Separator } from '@/components/ui/separator'
import { Switch } from '@/components/ui/switch'
import { runAction, runActionQuiet } from '@/lib/actions'
import { isUsable, type CapabilityReport } from '@/lib/bridge'
import { useI18n } from '@/lib/i18n'

type PageProps = {
  report: CapabilityReport | null
  info: MenuInfo | null
}

export type MenuInfo = {
  version: string
  author: string
  ui: string
  lang: string
  url: string
}

export function SettingsPage({ report, info }: PageProps) {
  const { t } = useI18n()
  const [ui, setUi] = useState(info?.ui ?? 'react')
  const [infiniteAmmo, setInfiniteAmmo] = useState(false)

  useEffect(() => {
    if (info?.ui) setUi(info.ui)
  }, [info])

  return (
    <div className="grid gap-5 lg:grid-cols-2">
      <Card>
        <CardHeader>
          <CardTitle>{t('settings.uiMode')}</CardTitle>
          <CardDescription>{t('react.uiHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-4">
          <div className="flex items-center gap-2">
            <Button
              variant={ui === 'react' ? 'default' : 'outline'}
              onClick={() => {
                setUi('react')
                void runAction('menu.setUi', { ui: 'react' }, 'settings.uiMode.web')
              }}
            >
              {t('settings.uiMode.web')}
            </Button>
            <Button
              variant={ui === 'imgui' ? 'default' : 'outline'}
              onClick={() => {
                setUi('imgui')
                void runAction('menu.setUi', { ui: 'imgui' }, 'settings.uiMode.imgui')
              }}
            >
              {t('settings.uiMode.imgui')}
            </Button>
            <Badge variant="secondary" className="ml-auto">
              {t('react.currentUi').replace('%s', ui === 'react' ? t('react.reactLabel') : 'ImGui')}
            </Badge>
          </div>
          <Separator />
          <div className="text-xs text-muted-foreground">{t('react.fallbackHint')}</div>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>{t('react.weapon')}</CardTitle>
          <CardDescription>{t('react.weaponHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-3">
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
          <Separator />
          <div className="text-xs text-muted-foreground">{t('settings.uiMode.hint')}</div>
        </CardContent>
      </Card>
    </div>
  )
}
