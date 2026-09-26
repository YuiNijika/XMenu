import { useCallback, useEffect, useRef, useState } from 'react'
import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { ScrollArea } from '@/components/ui/scroll-area'
import { Separator } from '@/components/ui/separator'
import { Slider } from '@/components/ui/slider'
import { toast, useToastManager } from '@/components/ui/toast'
import { SchemaSection } from '@/components/menu/schema-section'
import {
  call,
  fetchUiSchema,
  isUsable,
  type CapabilityReport,
  type UiSchemaPayload,
} from '@/lib/bridge'
import { useI18n } from '@/lib/i18n'
import { ACCENT_OPTIONS, THEME_LABEL_KEYS, THEME_MODES } from '@/lib/appearance'
import { useAppearance } from '@/lib/appearance-provider'
import { cn } from 'cn'

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
  xbaseVersion?: string
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

type SettingsTabId = 'appearance' | 'update' | 'log' | 'basic'

export function SettingsPage({ report, info }: PageProps) {
  const { t } = useI18n()
  const [ui, setUi] = useState(info?.ui ?? 'react')
  const [windowMode, setWindowMode] = useState<WindowModeState | null>(null)
  const [update, setUpdate] = useState<UpdateState | null>(null)
  const [entries, setEntries] = useState<LogEntry[]>([])
  const [total, setTotal] = useState(0)
  const [schema, setSchema] = useState<UiSchemaPayload | null>(null)
  const [tab, setTab] = useState<SettingsTabId>('appearance')

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

  useEffect(() => {
    if (info?.ui) setUi(info.ui)
  }, [info])

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
    loadWindowMode()
    loadUpdate()
    loadLog()
  }, [loadWindowMode, loadUpdate, loadLog])

  // 发现新版本时弹出提示，避免只在设置页静默显示
  const newVersionToasted = useRef(false)
  useEffect(() => {
    if (update?.status === 'remoteNewer') {
      if (!newVersionToasted.current) {
        newVersionToasted.current = true
        toast.add({
          type: 'info',
          title: t('status.remoteNewer'),
          description: update?.latestVersion ? `${t('status.remoteVersion').replace('%s', '')}${update.latestVersion}` : '',
        })
      }
    } else {
      newVersionToasted.current = false
    }
  }, [update?.status, update?.latestVersion, t])

  const tabs: { id: SettingsTabId; label: string }[] = [
    { id: 'appearance', label: t('react.appearance') },
    { id: 'update', label: t('update.availableTitle') },
    { id: 'log', label: t('log.title') },
    { id: 'basic', label: t('settings.basic') },
  ]

  return (
    <div className="flex flex-col gap-4 lg:flex-row lg:gap-8">
      <nav aria-label="设置分类" className="shrink-0 lg:sticky lg:top-6 lg:w-40 lg:self-start">
        <div className="flex gap-1.5 overflow-x-auto pb-1 lg:flex-col lg:gap-1 lg:overflow-visible lg:pb-0">
          {tabs.map((item) => {
            const active = tab === item.id
            return (
              <button
                key={item.id}
                type="button"
                aria-current={active ? 'page' : undefined}
                onClick={() => setTab(item.id)}
                className={cn(
                  'flex shrink-0 cursor-pointer items-center rounded-full px-3.5 py-2 text-sm font-medium transition-[background-color,color,transform]',
                  'active:scale-[0.98]',
                  'lg:w-full lg:rounded-xl',
                  active
                    ? 'bg-primary text-primary-foreground'
                    : 'text-muted-foreground hover:bg-[var(--surface-fill)] hover:text-foreground',
                )}
              >
                {item.label}
              </button>
            )
          })}
        </div>
      </nav>

      <div className="min-w-0 flex-1">
        <div key={tab} className="flex flex-col gap-5">
          {tab === 'appearance' ? (
            <AppearanceTab
              report={report}
              ui={ui}
              setUi={setUi}
              windowMode={windowMode}
              setWindowMode={setWindowMode}
              loadWindowMode={loadWindowMode}
              schema={schema}
            />
          ) : null}
          {tab === 'update' ? <UpdateTab update={update} loadUpdate={loadUpdate} /> : null}
          {tab === 'log' ? <LogTab entries={entries} total={total} loadLog={loadLog} /> : null}
          {tab === 'basic' ? <BasicTab schema={schema} report={report} /> : null}
        </div>
      </div>
    </div>
  )
}

// 外观：强调色、界面模式（Web/ImGui 与窗口模式）与界面语言
function AppearanceTab({
  report,
  ui,
  setUi,
  windowMode,
  setWindowMode,
  loadWindowMode,
  schema,
}: {
  report: CapabilityReport | null
  ui: string
  setUi: (value: string) => void
  windowMode: WindowModeState | null
  setWindowMode: (value: WindowModeState | null) => void
  loadWindowMode: () => void
  schema: UiSchemaPayload | null
}) {
  const { t } = useI18n()
  const { prefs, theme, setTheme, setAccent, setCustomHue } = useAppearance()
  const toastManager = useToastManager()
  const hue = prefs.accent === 'custom' ? prefs.customHue : 260
  const ModeKeys = [
    'settings.displayMode.fullscreen',
    'settings.displayMode.windowed',
    'settings.displayMode.borderless',
  ]

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
    <>
      <Card>
        <CardHeader>
          <CardTitle>{t('react.appearance')}</CardTitle>
          <CardDescription>{t('react.appearanceHint')}</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-4">
          {/* 明暗：与顶部调色板同一份状态，跟随系统时按系统设置变化 */}
          <div className="flex flex-col gap-2">
            <div className="text-sm font-medium">{t('react.theme')}</div>
            <div className="flex flex-wrap gap-2">
              {THEME_MODES.map((mode) => (
                <Button
                  key={mode}
                  size="sm"
                  variant={theme === mode ? 'default' : 'outline'}
                  onClick={() => setTheme(mode)}
                >
                  {t(THEME_LABEL_KEYS[mode])}
                </Button>
              ))}
            </div>
            <div className="text-xs text-muted-foreground">{t('react.themeHint')}</div>
          </div>

          <Separator />

          <div className="flex flex-wrap items-center gap-2">
            {ACCENT_OPTIONS.map((option) => (
              <button
                key={option.id}
                type="button"
                aria-pressed={prefs.accent === option.id}
                title={t(option.labelKey)}
                onClick={() => setAccent(option.id)}
                className={cn(
                  'h-8 w-8 rounded-full border-2 transition-transform active:scale-90',
                  'focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring',
                  prefs.accent === option.id ? 'border-foreground scale-110' : 'border-transparent',
                )}
                style={{ backgroundColor: `oklch(0.7 0.15 ${option.hue})` }}
              >
                <span className="sr-only">{t(option.labelKey)}</span>
              </button>
            ))}
          </div>

          <div className="text-sm">
            <div className="flex items-center justify-between gap-3">
              <span className="min-w-0 truncate">{t('react.accentCustom')}</span>
              <span className="shrink-0 tabular-nums text-muted-foreground">
                {prefs.accent === 'custom' ? hue : '—'}
              </span>
            </div>
            <Slider
              className="mt-1.5"
              min={0}
              max={359}
              step={1}
              value={[hue]}
              onValueChange={(next) => {
                const updated = Array.isArray(next) ? next[0] : next
                if (typeof updated !== 'number') return
                setCustomHue(updated)
              }}
            />
          </div>
        </CardContent>
      </Card>

      <Card>
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
          <Button variant="outline" className="w-fit" onClick={loadWindowMode}>
            {t('update.refresh')}
          </Button>
        </CardContent>
      </Card>

      {schema ? (
        <Card>
          <CardHeader>
            <CardTitle>{t('settings.interfaceLanguage')}</CardTitle>
            <CardDescription>{t('react.uiHint')}</CardDescription>
          </CardHeader>
          <CardContent className="grid gap-4">
            <SchemaSection payload={schema} report={report} tabId="settings" pageId="settingsMain" sectionId="language" />
          </CardContent>
        </Card>
      ) : null}
    </>
  )
}

// 更新：版本状态与忽略
function UpdateTab({ update, loadUpdate }: { update: UpdateState | null; loadUpdate: () => void }) {
  const { t } = useI18n()

  return (
    <Card>
      <CardHeader>
        <CardTitle>{t('update.availableTitle')}</CardTitle>
        <CardDescription>{t('update.sourceHint')}</CardDescription>
      </CardHeader>
      <CardContent className="flex flex-col gap-3 text-sm">
          <div className="flex flex-wrap items-center gap-2">
            <span className="text-muted-foreground">{t('status.localVersion').replace('%s', '')}</span>
            <Badge variant="secondary">{update?.currentVersion ?? '--'}</Badge>
            <span className="text-muted-foreground">{t('status.remoteVersion').replace('%s', '')}</span>
            <Badge variant="secondary">{update?.latestVersion ?? t('status.remoteUnknown')}</Badge>
          </div>
          <div className="text-xs text-muted-foreground">
            {update?.checking ? t('status.checking') : t(StatusKeys[update?.status ?? 'unknown'])}
            {update?.source ? ` · ${t('update.source').replace('%s', update.source)}` : ''}
          </div>
        <div className="flex flex-wrap gap-2">
          <Button variant="outline" disabled={update?.checking} onClick={() => void call('update.check').then(loadUpdate)}>
            {update?.checking ? t('status.checking') : t('update.refresh')}
          </Button>
          <Button variant="outline" onClick={() => void call('update.open', { url: 'https://github.com/YuiNijika/XMenu' })}>
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
            <Button
              key={option.key}
              variant="outline"
              size="sm"
              onClick={() => void call('update.later', { hours: option.hours })}
            >
              {t(option.key)}
            </Button>
          ))}
        </div>
      </CardContent>
    </Card>
  )
}

// 日志：近期记录与复制
function LogTab({
  entries,
  total,
  loadLog,
}: {
  entries: LogEntry[]
  total: number
  loadLog: () => void
}) {
  const { t } = useI18n()

  return (
    <Card>
      <CardHeader>
        <CardTitle>{t('log.title')}</CardTitle>
        <CardDescription>{t('log.counts').replace('%s', String(total))}</CardDescription>
      </CardHeader>
      <CardContent className="flex flex-col gap-3">
        <ScrollArea className="h-96 rounded-lg border border-border/60 p-3">
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
  )
}

// 基础设置：Overlay、外观（注册表）、运行态、GUI 风格、网页与配置导入导出
function BasicTab({ schema, report }: { schema: UiSchemaPayload | null; report: CapabilityReport | null }) {
  const { t } = useI18n()
  const [importText, setImportText] = useState('')

  return (
    <>
      {schema ? (
        <Card>
          <CardHeader>
            <CardTitle>{t('settings.basic')}</CardTitle>
            <CardDescription>{t('react.uiHint')}</CardDescription>
          </CardHeader>
          <CardContent className="grid gap-4">
            <SchemaSection payload={schema} report={report} tabId="settings" pageId="settingsMain" sectionId="overlay" />
            <SchemaSection payload={schema} report={report} tabId="settings" pageId="settingsMain" sectionId="appearance" />
            <SchemaSection payload={schema} report={report} tabId="settings" pageId="settingsMain" sectionId="runtime" />
            <SchemaSection payload={schema} report={report} tabId="settings" pageId="settingsMain" sectionId="guiStyle" />
            <SchemaSection payload={schema} report={report} tabId="settings" pageId="settingsMain" sectionId="web" />
          </CardContent>
        </Card>
      ) : (
        <Card>
          <CardHeader>
            <CardTitle>{t('settings.basic')}</CardTitle>
          </CardHeader>
          <CardContent className="text-sm text-muted-foreground">{t('react.loading')}</CardContent>
        </Card>
      )}

      <Card>
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
          <Input value={importText} onChange={(event) => setImportText(event.target.value)} placeholder={t('settings.importTextHint')} />
        </CardContent>
      </Card>
    </>
  )
}
