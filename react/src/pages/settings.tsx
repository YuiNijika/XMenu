import { useCallback, useEffect, useState } from 'react'
import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { Separator } from '@/components/ui/separator'
import { ScrollArea } from '@/components/ui/scroll-area'
import { useToastManager } from '@/components/ui/toast'
import { SchemaSection } from '@/components/menu/schema-section'
import {
  call,
  fetchUiSchema,
  isUsable,
  type CapabilityReport,
  type UiSchemaPayload,
} from '@/lib/bridge'
import { useI18n } from '@/lib/i18n'

const ModeKeys = [
  'settings.displayMode.fullscreen',
  'settings.displayMode.windowed',
  'settings.displayMode.borderless',
]

type WindowModeState = {
  current: number
  pending: number
  restartRequired: boolean
}

type UpdateState = {
  checking: boolean
  available: boolean
  currentVersion: string
  latestVersion: string
  releaseUrl: string
  source: string
  status: string
}

type LogEntry = {
  level: string
  line: string
}

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

const StatusKeys: Record<string, string> = {
  equal: 'status.upToDate',
  localNewer: 'status.localNewer',
  remoteNewer: 'status.remoteNewer',
  unknown: 'status.remoteUnknown',
}

export function SettingsPage({ report, info }: PageProps) {
  const { t } = useI18n()
  const [ui, setUi] = useState(info?.ui ?? 'react')
  const [windowMode, setWindowMode] = useState<WindowModeState | null>(null)
  const [update, setUpdate] = useState<UpdateState | null>(null)
  const [entries, setEntries] = useState<LogEntry[]>([])
  const [total, setTotal] = useState(0)
  const [importText, setImportText] = useState('')
  const [schema, setSchema] = useState<UiSchemaPayload | null>(null)
  const toastManager = useToastManager()

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

  const loadWindowMode = useCallback(() => {
    call<WindowModeState>('settings.windowModeGet').then(setWindowMode).catch(() => setWindowMode(null))
  }, [])

  const loadUpdate = useCallback(() => {
    call<UpdateState>('update.status').then(setUpdate).catch(() => setUpdate(null))
  }, [])

  const loadLog = useCallback(() => {
    call<{ total: number; items: LogEntry[] }>('log.recent', { count: 80 })
      .then((payload: { total: number; items: LogEntry[] }) => {
        setEntries(payload.items ?? [])
        setTotal(payload.total)
      })
      .catch(() => setEntries([]))
  }, [])

  useEffect(() => {
    if (info?.ui) setUi(info.ui)
  }, [info])

  useEffect(() => {
    loadWindowMode()
    loadUpdate()
    loadLog()
  }, [loadWindowMode, loadUpdate, loadLog])

  const pickMode = async (value: number) => {
    try {
      const result = await call<WindowModeState>('settings.windowMode', { value })
      setWindowMode(result)
      toastManager.add({
        type: 'success',
        title: t(result.restartRequired ? 'settings.displayModeRestartNotice' : 'react.done'),
        description: t('settings.displayMode'),
      })
    } catch {
      toastManager.add({ type: 'error', title: t('react.failed'), description: t('settings.displayMode') })
    }
  }

  return (
    <div className="grid gap-5 lg:grid-cols-2">
      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('settings.uiMode')}</CardTitle>
          <CardDescription>{t('settings.uiMode.hint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-4">
          <div className="flex flex-wrap items-center gap-2">
            <Button
              variant={ui === 'react' ? 'default' : 'outline'}
              onClick={() => {
                setUi('react')
                void call('menu.setUi', { ui: 'react' })
              }}
            >
              {t('settings.uiMode.web')}
            </Button>
            <Button
              variant={ui === 'imgui' ? 'default' : 'outline'}
              onClick={() => {
                setUi('imgui')
                void call('menu.setUi', { ui: 'imgui' })
              }}
            >
              {t('settings.uiMode.imgui')}
            </Button>
            <Badge variant="secondary" className="ml-auto">
              {t('react.currentUi').replace('%s', ui === 'react' ? t('react.reactLabel') : 'ImGui')}
            </Badge>
          </div>
          <div className="text-xs text-muted-foreground">{t('react.uiHint')}</div>

          <Separator />

          <div className="flex flex-wrap items-center gap-2">
            {ModeKeys.map((key, mode) => (
              <Button
                key={key}
                variant={(windowMode?.pending ?? 0) === mode ? 'default' : 'outline'}
                disabled={!isUsable(report, 'settings.windowMode') && windowMode === null}
                onClick={() => void pickMode(mode)}
              >
                {t(key)}
              </Button>
            ))}
            <Badge variant="secondary" className="ml-auto">
              {t('settings.displayModeCurrent').replace('%s', t(ModeKeys[windowMode?.current ?? 0]))}
            </Badge>
          </div>
          <div className="text-xs text-muted-foreground">{t('settings.displayModeHint')}</div>
          {windowMode?.restartRequired ? (
            <div className="text-xs text-amber-400">{t('settings.displayModePending')}</div>
          ) : null}
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>{t('update.availableTitle')}</CardTitle>
          <CardDescription>{t('update.sourceHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-3 text-sm">
          <div className="flex flex-wrap items-center gap-2">
            <span className="text-muted-foreground">{t('status.localVersion')}</span>
            <Badge variant="secondary">{update?.currentVersion ?? '--'}</Badge>
            <span className="text-muted-foreground">{t('status.remoteVersion')}</span>
            <Badge variant="secondary">{update?.latestVersion ?? t('status.remoteUnknown')}</Badge>
          </div>
          <div className="text-xs text-muted-foreground">
            {update?.checking ? t('status.checking') : t(StatusKeys[update?.status ?? 'unknown'])}
            {update?.source ? ` · ${t('update.source')} ${update.source}` : ''}
          </div>
          <div className="flex flex-wrap gap-2">
            <Button variant="outline" disabled={update?.checking} onClick={() => void call('update.check').then(loadUpdate)}>
              {update?.checking ? t('status.checking') : t('update.refresh')}
            </Button>
            <Button variant="outline" onClick={() => void call('update.open')}>
              {t('update.openGitHub')}
            </Button>
            <Button variant="outline" onClick={() => void call('update.skip').then(loadUpdate)}>
              {t('update.skipVersion')}
            </Button>
          </div>
          <Separator />
          <div className="flex flex-wrap items-center gap-2">
            <span className="text-xs text-muted-foreground">{t('update.laterTitle')}</span>
            {[
              { hours: 24, key: 'update.later1d' },
              { hours: 72, key: 'update.later3d' },
              { hours: 168, key: 'update.later7d' },
            ].map((option) => (
              <Button key={option.key} variant="outline" size="sm" onClick={() => void call('update.later', { hours: option.hours })}>
                {t(option.key)}
              </Button>
            ))}
          </div>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>{t('log.title')}</CardTitle>
          <CardDescription>{t('log.counts').replace('%s', String(total))}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-3">
          <ScrollArea className="h-48 rounded-lg border border-border/60 p-3">
            {entries.length === 0 ? (
              <div className="text-xs text-muted-foreground">{t('log.empty')}</div>
            ) : (
              <div className="flex flex-col gap-1 font-mono text-[11px] leading-relaxed">
                {entries.map((entry, index) => (
                  <div key={`${entry.line}-${index}`} className={entry.level === 'ERROR' ? 'text-destructive' : ''}>
                    {entry.line}
                  </div>
                ))}
              </div>
            )}
          </ScrollArea>
          <div className="flex flex-wrap gap-2">
            <Button variant="outline" onClick={() => void call('log.copy')}>
              {t('log.copy')}
            </Button>
            <Button variant="outline" onClick={loadLog}>
              {t('update.refresh')}
            </Button>
          </div>
        </CardContent>
      </Card>

      <Card className="lg:col-span-2">
        <CardHeader>
          <CardTitle>{t('settings.config')}</CardTitle>
          <CardDescription>{t('settings.exportTextHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-3">
          <div className="flex flex-wrap gap-2">
            <Button variant="outline" onClick={() => void call('config.export')}>
              {t('settings.exportConfig')}
            </Button>
            <Button variant="outline" onClick={() => void call('config.import', { text: importText })}>
              {t('settings.importConfig')}
            </Button>
          </div>
          <Input
            value={importText}
            onChange={(event) => setImportText(event.target.value)}
            placeholder={t('settings.importTextHint')}
          />
        </CardContent>
      </Card>

      {schema ? (
        <Card className="lg:col-span-2">
          <CardHeader>
            <CardTitle>{t('settings.overlay')}</CardTitle>
            <CardDescription>{t('react.uiHint')}</CardDescription>
          </CardHeader>
          <CardContent className="grid gap-4">
            <SchemaSection payload={schema} report={report} tabId="settings" pageId="settingsMain" sectionId="language" />
            <SchemaSection payload={schema} report={report} tabId="settings" pageId="settingsMain" sectionId="appearance" />
            <SchemaSection payload={schema} report={report} tabId="settings" pageId="settingsMain" sectionId="runtime" />
            <SchemaSection payload={schema} report={report} tabId="settings" pageId="settingsMain" sectionId="overlay" />
            <SchemaSection payload={schema} report={report} tabId="settings" pageId="settingsMain" sectionId="guiStyle" />
            <SchemaSection payload={schema} report={report} tabId="settings" pageId="settingsMain" sectionId="web" />
          </CardContent>
        </Card>
      ) : null}
    </div>
  )
}
