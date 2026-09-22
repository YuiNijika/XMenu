import { useEffect, useState } from 'react'
import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Separator } from '@/components/ui/separator'
import { Switch } from '@/components/ui/switch'
import { call, isUsable, type CapabilityReport } from '@/lib/bridge'
import { useI18n } from '@/lib/i18n'

type PageProps = {
  report: CapabilityReport | null
  info: MenuInfo | null
}

export type MenuInfo = {
  version: string
  author: string
  ui: string
  url: string
}

export function SettingsPage({ report, info }: PageProps) {
  const [ui, setUi] = useState(info?.ui ?? 'react')
  const { t } = useI18n()
  const [notice, setNotice] = useState<string | null>(null)

  useEffect(() => {
    if (info?.ui) setUi(info.ui)
  }, [info])

  const switchTo = async (target: string) => {
    try {
      await call('menu.setUi', { ui: target })
      setUi(target)
      setNotice(target === 'imgui' ? '已切换到 ImGui，重新打开菜单后生效' : '已在 React 界面中')
    } catch (reason: unknown) {
      setNotice(reason instanceof Error ? reason.message : String(reason))
    }
  }

  const toggleWeapon = async (method: string, enabled: boolean) => {
    try {
      await call(method, { enable: enabled })
    } catch {
      // 能力不足时按钮已禁用
    }
  }

  const [infiniteAmmo, setInfiniteAmmo] = useState(false)

  return (
    <div className="grid gap-5 lg:grid-cols-2">
      <Card>
        <CardHeader>
          <CardTitle>{t('settings.guiStyle')}</CardTitle>
          <CardDescription>{t('react.uiHint', 'React 与 ImGui 两套界面共用同一份配置')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-4">
          <div className="flex items-center gap-2">
            <Button variant={ui === 'react' ? 'default' : 'outline'} onClick={() => switchTo('react')}>
              React
            </Button>
            <Button variant={ui === 'imgui' ? 'default' : 'outline'} onClick={() => switchTo('imgui')}>
              ImGui
            </Button>
            <Badge variant="secondary" className="ml-auto">
              {t('react.currentUi', '当前 %s').replace('%s', ui)}
            </Badge>
          </div>
          {notice ? <div className="text-xs text-muted-foreground">{notice}</div> : null}
          <Separator />
          <div className="text-xs text-muted-foreground">
            网页不可用时宿主会自动回退到 ImGui，并在日志与提示条里说明原因
          </div>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>{t('react.weapon', '武器')}</CardTitle>
          <CardDescription>{t('react.weaponHint', '与 ImGui 版共享同一套开关')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-3">
          <label className="flex items-center justify-between text-sm">
            <span>{t('react.infiniteAmmo', '无限弹药')}</span>
            <Switch
              checked={infiniteAmmo}
              disabled={!isUsable(report, 'weapon.infiniteAmmo')}
              onCheckedChange={(checked) => {
                setInfiniteAmmo(checked)
                void toggleWeapon('weapon.infiniteAmmo', checked)
              }}
            />
          </label>
        </CardContent>
      </Card>

      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('tab.about')}</CardTitle>
          <CardDescription>{t('react.aboutHint', '版本与作者')}</CardDescription>
        </CardHeader>
        <CardContent className="grid gap-2 text-sm">
          <div>版本 {info?.version ?? '--'}</div>
          <div>作者 {info?.author ?? '--'}</div>
          <div className="text-muted-foreground">{info?.url ?? ''}</div>
        </CardContent>
      </Card>
    </div>
  )
}
