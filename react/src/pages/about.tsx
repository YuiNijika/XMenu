import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'
import { Separator } from '@/components/ui/separator'
import { runAction } from '@/lib/actions'
import { call } from '@/lib/bridge'
import { useI18n } from '@/lib/i18n'
import type { MenuInfo } from '@/pages/settings'

type PageProps = {
  info: MenuInfo | null
}

export function AboutPage({ info }: PageProps) {
  const { t } = useI18n()

  return (
    <div className="flex flex-col gap-5">
      <div className="relative overflow-hidden rounded-2xl border border-border/60 bg-gradient-to-br from-primary/15 via-card to-card p-8 lg:p-10">
        <div className="pointer-events-none absolute -right-16 -top-16 h-56 w-56 rounded-full bg-primary/20 blur-3xl" />
        <div className="relative flex flex-col gap-6">
          <div>
            <div className="text-xs font-semibold tracking-[0.3em] text-muted-foreground uppercase">XMenu</div>
            <h1 className="mt-2 text-3xl font-bold tracking-tight">{t('tab.about')}</h1>
            <p className="mt-2 max-w-prose text-sm text-muted-foreground">{t('react.aboutHint')}</p>
          </div>

          <div className="flex flex-wrap items-center gap-3 text-sm">
            <Badge variant="secondary" className="tabular-nums">
              XMenu {info?.version ?? '--'}
            </Badge>
            <Badge variant="secondary" className="tabular-nums">
              XBase {info?.xbaseVersion ?? '--'}
            </Badge>
            <span className="text-muted-foreground">{info?.author ?? '--'}</span>
          </div>

          <div className="flex flex-wrap gap-2">
            <Button variant="outline" onClick={() => void call('update.open', { url: info?.url })}>
              {t('about.projectPage')}
            </Button>
            <Button
              variant="outline"
              onClick={() => void call('update.open', { url: 'https://github.com/YuiNijika/XMenu' })}
            >
              {t('update.openGitHub')}
            </Button>
          </div>

          <Separator />

          <div className="flex flex-wrap gap-2">
            <Button
              variant="outline"
              onClick={() => void runAction('menu.setUi', { ui: 'imgui' }, 'settings.uiMode.imgui')}
            >
              {t('settings.uiMode.imgui')}
            </Button>
            <Button variant="outline" onClick={() => void runAction('menu.hide', undefined, 'react.hide')}>
              {t('react.hide')}
            </Button>
            <Button variant="outline" onClick={() => void runAction('menu.close', undefined, 'react.close')}>
              {t('react.close')}
            </Button>
          </div>
        </div>
      </div>
    </div>
  )
}
