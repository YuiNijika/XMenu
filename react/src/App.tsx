import { useEffect, useMemo, useState } from 'react'
import { Alert, AlertDescription, AlertTitle } from '@/components/ui/alert'
import { Shell, type MenuPage } from '@/components/menu/shell'
import { AboutPage } from '@/pages/about'
import { PlayerPage } from '@/pages/player'
import { SettingsPage, type MenuInfo } from '@/pages/settings'
import { TeleportPage } from '@/pages/teleport'
import { VehiclePage } from '@/pages/vehicle'
import { WeaponPage } from '@/pages/weapon'
import { WorldPage } from '@/pages/world'
import { call, isBridgeAvailable, type CapabilityReport } from '@/lib/bridge'
import { I18nProvider, useI18n } from '@/lib/i18n'
import { useCapabilities } from '@/lib/hooks'
import './App.css'

const Pages: (MenuPage & { titleKey: string; subtitleKey: string })[] = [
  { id: 'player', label: 'tab.player', titleKey: 'tab.player', subtitleKey: 'player.health' },
  { id: 'vehicle', label: 'tab.vehicle', titleKey: 'tab.vehicle', subtitleKey: 'vehicle.health' },
  { id: 'world', label: 'tab.world', titleKey: 'tab.world', subtitleKey: 'world.time' },
  { id: 'teleport', label: 'tab.teleport', titleKey: 'tab.teleport', subtitleKey: 'teleport.coordinates' },
  { id: 'weapon', label: 'tab.weapon', titleKey: 'tab.weapon', subtitleKey: 'react.weaponHint' },
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
  const { t } = useI18n()
  const [activePage, setActivePage] = useState(Pages[0].id)
  const [info, setInfo] = useState<MenuInfo | null>(null)
  const { report, error } = useCapabilities()
  const bridgeReady = useMemo(() => isBridgeAvailable(), [])

  useEffect(() => {
    if (!bridgeReady) return
    call<MenuInfo>('menu.info')
      .then(setInfo)
      .catch(() => setInfo(null))
  }, [bridgeReady])

  const current = Pages.find((page) => page.id === activePage) ?? Pages[0]

  if (!bridgeReady) {
    return (
      <div className="flex h-screen items-center justify-center bg-background p-10">
        <Alert className="max-w-xl">
          <AlertTitle>{t('react.bridgeUnavailable', '网页界面不可用')}</AlertTitle>
          <AlertDescription>
            {t('react.bridgeUnavailableHint', '当前面板没有连接到 XBase')}
          </AlertDescription>
        </Alert>
      </div>
    )
  }

  return (
    <Shell
      pages={Pages.map((page) => ({ id: page.id, label: t(page.label) }))}
      activePage={current.id}
      onSelect={setActivePage}
      title={t(current.titleKey)}
      subtitle={t(current.subtitleKey)}
    >
      {error ? (
        <Alert className="mb-5">
          <AlertTitle>{t('react.capabilityFailed', '能力表读取失败')}</AlertTitle>
          <AlertDescription>{error}</AlertDescription>
        </Alert>
      ) : null}
      {renderPage(current.id, report, info)}
    </Shell>
  )
}

function renderPage(id: string, report: CapabilityReport | null, info: MenuInfo | null) {
  switch (id) {
    case 'vehicle':
      return <VehiclePage report={report} />
    case 'world':
      return <WorldPage report={report} />
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
