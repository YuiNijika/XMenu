import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { call } from '@/lib/bridge'
import { useI18n } from '@/lib/i18n'
import type { MenuInfo } from '@/pages/settings'

type PageProps = {
  info: MenuInfo | null
}

export function AboutPage({ info }: PageProps) {
  const { t } = useI18n()

  return (
    <div className="grid gap-5 lg:grid-cols-2">
      <Card>
        <CardHeader>
          <CardTitle>{t('tab.about')}</CardTitle>
          <CardDescription>{t('react.aboutHint')}</CardDescription>
        </CardHeader>
        <CardContent className="grid gap-2 text-sm">
          <div className="flex items-center gap-2">
            <span className="text-muted-foreground">XMenu</span>
            <Badge variant="secondary">{info?.version ?? '--'}</Badge>
          </div>
          <div>{info?.author ?? '--'}</div>
          <div className="text-muted-foreground">{info?.url ?? ''}</div>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>{t('settings.uiMode')}</CardTitle>
          <CardDescription>{t('settings.uiMode.hint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-wrap gap-2">
          <Button
            variant="outline"
            onClick={() => {
              void call('menu.setUi', { ui: 'imgui' })
            }}
          >
            {t('settings.uiMode.imgui')}
          </Button>
          <Button variant="outline" onClick={() => void call('menu.hide')}>
            {t('react.hide')}
          </Button>
          <Button variant="outline" onClick={() => void call('menu.close')}>
            {t('react.close')}
          </Button>
        </CardContent>
      </Card>
    </div>
  )
}
