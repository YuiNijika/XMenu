import { useEffect, useMemo, useState } from 'react'
import { Alert, AlertDescription, AlertTitle } from '@/components/ui/alert'
import { Shell, type MenuPage } from '@/components/menu/shell'
import { AboutPage } from '@/pages/about'
import { PedPage } from '@/pages/ped'
import { PlayerPage } from '@/pages/player'
import { ScenePage } from '@/pages/scene'
import { SettingsPage, type MenuInfo } from '@/pages/settings'
import { TeleportPage } from '@/pages/teleport'
import { VehiclePage } from '@/pages/vehicle'
import { VisualPage } from '@/pages/visual'
import { WeaponPage } from '@/pages/weapon'
import { WorldPage } from '@/pages/world'
import { Button } from '@/components/ui/button'
import { call, isBridgeAvailable, rawCall, type CapabilityReport } from '@/lib/bridge'
import { I18nProvider, useI18n } from '@/lib/i18n'
import { useCapabilities } from '@/lib/hooks'
import './App.css'

const Pages: (MenuPage & { titleKey: string; subtitleKey: string })[] = [
  { id: 'player', label: 'tab.player', titleKey: 'tab.player', subtitleKey: 'player.health' },
  { id: 'vehicle', label: 'tab.vehicle', titleKey: 'tab.vehicle', subtitleKey: 'vehicle.health' },
  { id: 'ped', label: 'tab.ped', titleKey: 'tab.ped', subtitleKey: 'ped.hint' },
  { id: 'weapon', label: 'tab.weapon', titleKey: 'tab.weapon', subtitleKey: 'react.weaponHint' },
  { id: 'world', label: 'tab.world', titleKey: 'tab.world', subtitleKey: 'world.time' },
  { id: 'visual', label: 'tab.visual', titleKey: 'tab.visual', subtitleKey: 'visual.filterHint' },
  { id: 'scene', label: 'tab.scene', titleKey: 'tab.scene', subtitleKey: 'scene.animationListHint' },
  { id: 'teleport', label: 'tab.teleport', titleKey: 'tab.teleport', subtitleKey: 'teleport.coordinates' },
  { id: 'about', label: 'tab.about', titleKey: 'tab.about', subtitleKey: 'react.aboutHint' },
  { id: 'settings', label: 'tab.settings', titleKey: 'tab.settings', subtitleKey: 'settings.language' },
]

function App() {
  return (
    <I18nProvider>
      <MenuApp />
    </I18nProvider>
  )
}

function MenuApp() {
  const { t, lang } = useI18n()
  const [activePage, setActivePage] = useState(Pages[0].id)
  const [info, setInfo] = useState<MenuInfo | null>(null)
  const { report, error } = useCapabilities()
  const bridgeReady = useMemo(() => isBridgeAvailable(), [])

  // 面板跟着产品默认主题走深色，原生控件与滚动条才会一起变深，
  // 语言也要写到 html 上，读屏与自动翻译才认得当前语言
  useEffect(() => {
    document.documentElement.classList.add('dark')
  }, [])

  useEffect(() => {
    document.documentElement.lang = lang
  }, [lang])

  useEffect(() => {
    if (!bridgeReady) return
    call<MenuInfo>('menu.info')
      .then(setInfo)
      .catch(() => setInfo(null))
  }, [bridgeReady])

  const current = Pages.find((page) => page.id === activePage) ?? Pages[0]

  if (!bridgeReady) {
    return <BridgeHero />
  }

  return (
    <Shell
      pages={Pages.map((page) => ({ id: page.id, label: t(page.label) }))}
      activePage={current.id}
      onSelect={setActivePage}
      title={t(current.titleKey)}
      subtitle={t(current.subtitleKey)}
      game={report?.gameName ?? ''}
      version={info?.version}
    >
      {error ? (
        <Alert className="mb-5">
          <AlertTitle>{t('react.capabilityFailed')}</AlertTitle>
          <AlertDescription>{error}</AlertDescription>
        </Alert>
      ) : null}
      {renderPage(current.id, report, info)}
    </Shell>
  )
}

// 没有接入 XBase 时不给任何页面入口，只留重载与切回 ImGui 两条出路
function BridgeHero() {
  const { t } = useI18n()
  const [busy, setBusy] = useState(false)

  const toImgui = () => {
    setBusy(true)
    rawCall('menu.setUi', { ui: 'imgui' })
    window.setTimeout(() => setBusy(false), 800)
  }

  return (
    <div className="relative flex h-screen w-screen items-center justify-center overflow-hidden bg-background p-8 text-foreground">
      <div className="pointer-events-none absolute -top-40 left-1/2 h-96 w-[38rem] -translate-x-1/2 rounded-full bg-primary/20 blur-3xl" />
      <div className="relative w-full max-w-lg rounded-3xl border border-border/60 bg-card/70 p-10 text-center shadow-2xl backdrop-blur-2xl">
        <div className="text-xs font-medium tracking-[0.3em] text-muted-foreground uppercase">XMenu</div>
        <h1 className="mt-4 text-2xl font-semibold tracking-tight">{t('react.heroTitle')}</h1>
        <p className="mt-3 text-sm text-muted-foreground">{t('react.heroHint')}</p>
        <div className="mt-8 flex flex-wrap items-center justify-center gap-3">
          <Button variant="outline" onClick={() => window.location.reload()}>
            {t('react.heroReload')}
          </Button>
          <Button disabled={busy} onClick={toImgui}>
            {t('react.heroImgui')}
          </Button>
        </div>
      </div>
    </div>
  )
}

function renderPage(id: string, report: CapabilityReport | null, info: MenuInfo | null) {
  switch (id) {
    case 'vehicle':
      return <VehiclePage report={report} />
    case 'ped':
      return <PedPage report={report} />
    case 'world':
      return <WorldPage report={report} />
    case 'visual':
      return <VisualPage report={report} />
    case 'scene':
      return <ScenePage report={report} />
    case 'teleport':
      return <TeleportPage report={report} />
    case 'weapon':
      return <WeaponPage report={report} />
    case 'about':
      return <AboutPage info={info} />
    case 'settings':
      return <SettingsPage report={report} info={info} />
    case 'player':
    default:
      return <PlayerPage report={report} />
  }
}

export default App
