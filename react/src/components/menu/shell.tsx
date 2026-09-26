import { useEffect, useRef, useState } from 'react'
import { Badge } from '@/components/ui/badge'
import { Button, buttonVariants } from '@/components/ui/button'
import { Toaster, toast } from '@/components/ui/toast'
import { call } from '@/lib/bridge'
import { runAction } from '@/lib/actions'
import { useI18n } from '@/lib/i18n'
import {
  Car,
  Clapperboard,
  Crosshair,
  EyeOffIcon,
  Globe2,
  Info,
  MapPin,
  Monitor,
  MoonStar,
  Palette,
  Settings2,
  SunMedium,
  User,
  Users,
  XIcon,
  type LucideIcon,
} from 'lucide-react'
import { cn } from 'cn'
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuGroup,
  DropdownMenuLabel,
  DropdownMenuRadioGroup,
  DropdownMenuRadioItem,
  DropdownMenuSeparator,
  DropdownMenuTrigger,
} from '@/components/ui/dropdown-menu'
import { ACCENT_OPTIONS, THEME_LABEL_KEYS, THEME_MODES } from '@/lib/appearance'
import { useAppearance } from '@/lib/appearance-provider'

export type MenuPage = {
  id: string
  label: string
}

// 页面图标与分组，布局参考 MusicStorm 的侧栏：主导航留内容入口，关于与设置收尾
const PAGE_ICONS: Record<string, LucideIcon> = {
  player: User,
  vehicle: Car,
  ped: Users,
  weapon: Crosshair,
  world: Globe2,
  visual: EyeOffIcon,
  scene: Clapperboard,
  teleport: MapPin,
  about: Info,
  settings: Settings2,
}

const SECONDARY_PAGES = new Set(['about', 'settings'])

type ShellProps = {
  pages: MenuPage[]
  activePage: string
  onSelect: (id: string) => void
  title: string
  subtitle: string
  game: string
  version?: string
  children: React.ReactNode
}

type DragState = {
  x: number
  y: number
  width: number
  height: number
}

// 侧边栏固定在左，内容区独立滚动，右下角可拖动改面板尺寸
export function Shell({ pages, activePage, onSelect, title, subtitle, game, version, children }: ShellProps) {
  const { t } = useI18n()
  const dragRef = useRef<DragState | null>(null)
  const navRef = useRef<HTMLElement | null>(null)
  const scrollRef = useRef<HTMLDivElement | null>(null)
  const [resizing, setResizing] = useState(false)
  const [moving, setMoving] = useState(false)
  const [exclusiveFullscreen, setExclusiveFullscreen] = useState(false)
  const moveRef = useRef<{ x: number; y: number; left: number; top: number } | null>(null)
  const panelRectRef = useRef<{ x: number; y: number } | null>(null)

  // 面板矩形提前取好并定期刷新，拖动时才不用等异步回调
  useEffect(() => {
    const refresh = () => {
      call<{ x: number; y: number }>('menu.panelRect')
        .then((rect) => {
          panelRectRef.current = { x: rect.x, y: rect.y }
        })
        .catch(() => {
          panelRectRef.current = null
        })
    }
    refresh()
    const timer = window.setInterval(refresh, 2000)
    return () => window.clearInterval(timer)
  }, [])

  // 独占全屏下面板只能抓帧回显，这里判断后提示改成窗口或无边框
  useEffect(() => {
    const check = () => {
      call<{ current: number }>('settings.windowModeGet')
        .then((state) => setExclusiveFullscreen(state.current === 0))
        .catch(() => setExclusiveFullscreen(false))
    }
    check()
    const timer = window.setInterval(check, 5000)
    return () => window.clearInterval(timer)
  }, [])

  // 换页后内容区回到顶部，翻长列表时不会停在上一页的位置
  useEffect(() => {
    scrollRef.current?.scrollTo({ top: 0 })
  }, [activePage])

  // 侧边栏用上下键切页，Home 与 End 跳首尾，键盘用户不必再摸鼠标
  const onNavKeyDown = (event: React.KeyboardEvent<HTMLElement>) => {
    if (!['ArrowDown', 'ArrowUp', 'Home', 'End'].includes(event.key)) {
      return
    }
    event.preventDefault()
    const index = pages.findIndex((page) => page.id === activePage)
    let next = index
    if (event.key === 'ArrowDown') next = Math.min(pages.length - 1, index + 1)
    if (event.key === 'ArrowUp') next = Math.max(0, index - 1)
    if (event.key === 'Home') next = 0
    if (event.key === 'End') next = pages.length - 1
    if (next === index || next < 0) {
      return
    }
    onSelect(pages[next].id)
    navRef.current?.querySelectorAll<HTMLButtonElement>('button[data-page]')[next]?.focus()
  }

  // 宿主每帧只处理一次挂起动作，这里不做节流，拖动才能跟手
  const sendSize = (width: number, height: number) => {
    void call('menu.setPanelSize', { width, height })
  }

  const sendPosition = (x: number, y: number) => {
    void call('menu.setPanelPos', { x, y })
  }

  // 拖动标题栏整体移动面板，位置以缓存的面板矩形为基准
  const beginMove = (event: React.PointerEvent<HTMLDivElement>) => {
    const start = panelRectRef.current
    if (!start) {
      // 缓存还没就绪，先补一次再让用户重拖，避免异步里丢指针捕获
      void call<{ x: number; y: number }>('menu.panelRect').then((rect) => {
        panelRectRef.current = { x: rect.x, y: rect.y }
      })
      return
    }
    event.currentTarget.setPointerCapture(event.pointerId)
    moveRef.current = { x: event.clientX, y: event.clientY, left: start.x, top: start.y }
    setMoving(true)
  }

  const moveMove = (event: React.PointerEvent<HTMLDivElement>) => {
    const start = moveRef.current
    if (!start) {
      return
    }
    sendPosition(start.left + (event.clientX - start.x), start.top + (event.clientY - start.y))
  }

  const endMove = (event: React.PointerEvent<HTMLDivElement>) => {
    const start = moveRef.current
    moveRef.current = null
    setMoving(false)
    if (!start) {
      return
    }
    sendPosition(start.left + (event.clientX - start.x), start.top + (event.clientY - start.y))
  }

  const beginResize = (event: React.PointerEvent<HTMLDivElement>) => {
    event.preventDefault()
    event.currentTarget.setPointerCapture(event.pointerId)
    dragRef.current = {
      x: event.clientX,
      y: event.clientY,
      width: window.innerWidth,
      height: window.innerHeight,
    }
    setResizing(true)
  }

  const moveResize = (event: React.PointerEvent<HTMLDivElement>) => {
    const start = dragRef.current
    if (!start) {
      return
    }
    sendSize(start.width + (event.clientX - start.x), start.height + (event.clientY - start.y))
  }

  // 拖拽需要键盘等价物，聚焦手柄后用方向键调尺寸
  const resizeWithKeyboard = (event: React.KeyboardEvent<HTMLDivElement>) => {
    const step = 40.0
    const width = window.innerWidth + (event.key === 'ArrowRight' ? step : event.key === 'ArrowLeft' ? -step : 0)
    const height = window.innerHeight + (event.key === 'ArrowUp' ? step : event.key === 'ArrowDown' ? -step : 0)
    if (width === window.innerWidth && height === window.innerHeight) {
      return
    }
    event.preventDefault()
    sendSize(width, height)
  }

  const endResize = (event: React.PointerEvent<HTMLDivElement>) => {
    const start = dragRef.current
    dragRef.current = null
    setResizing(false)
    if (!start) {
      return
    }
    sendSize(start.width + (event.clientX - start.x), start.height + (event.clientY - start.y))
  }

  return (
    <Toaster>
      <div className={cn('relative flex h-screen w-screen overflow-hidden bg-background text-foreground', resizing && 'select-none')}>
        <aside className="flex w-[76px] shrink-0 flex-col border-r border-border/60 bg-sidebar/80 backdrop-blur-xl">
          <div className="flex h-14 items-center justify-center border-b border-border/60 px-2">
            <span className="truncate text-sm font-semibold tracking-tight">XMenu</span>
          </div>
          <nav
            ref={navRef}
            aria-label={t('react.navigation')}
            onKeyDown={onNavKeyDown}
            className="mt-2 flex w-full min-h-0 flex-1 flex-col items-center gap-1 overflow-y-auto px-1.5 pb-2"
          >
            {pages.map((page, index) => {
              const Icon = PAGE_ICONS[page.id]
              const secondary = SECONDARY_PAGES.has(page.id)
              const showGroupGap = secondary && !SECONDARY_PAGES.has(pages[index - 1]?.id ?? '')
              const isActive = page.id === activePage
              return (
                <div key={page.id} className="w-full">
                  {showGroupGap ? <div className="mb-2 mt-1 border-t border-border/60" /> : null}
                  <button
                    type="button"
                    data-page={page.id}
                    aria-current={isActive ? 'page' : undefined}
                    title={page.label}
                    onClick={() => onSelect(page.id)}
                    className={cn(
                      'flex w-full flex-col items-center justify-center gap-[3px] rounded-xl py-2',
                      'transition-[color,background-color,transform] duration-150',
                      'focus-visible:ring-2 focus-visible:ring-ring focus-visible:outline-none',
                      'active:scale-[0.97]',
                      isActive
                        ? 'bg-primary text-primary-foreground'
                        : 'text-muted-foreground hover:bg-muted hover:text-foreground',
                    )}
                  >
                    {Icon ? (
                      <Icon
                        className="size-[22px] shrink-0"
                        strokeWidth={isActive ? 2.3 : 1.9}
                        aria-hidden="true"
                      />
                    ) : null}
                    <span
                      className={cn(
                        'text-[11px] leading-none',
                        isActive ? 'font-semibold' : 'font-medium',
                      )}
                    >
                      {page.label}
                    </span>
                  </button>
                </div>
              )
            })}
          </nav>
          <div className="border-t border-border/60 px-1 py-2 text-center text-[10px] leading-tight text-muted-foreground">
            <div className="truncate opacity-70" title={version ?? ''}>
              XMenu {version ?? ''}
            </div>
          </div>
        </aside>

        <main className="flex min-h-0 min-w-0 flex-1 flex-col">
          {exclusiveFullscreen ? (
            <div className="flex flex-wrap items-center gap-3 border-b border-amber-500/40 bg-amber-500/10 px-7 py-3 text-xs text-amber-200">
              <span className="min-w-0 flex-1">{t('react.fullscreenBanner')}</span>
              <Button
                variant="outline"
                size="sm"
                onClick={() => {
                  call<{ restartRequired: boolean }>('settings.windowMode', { value: 2 })
                    .then((state) => {
                      toast.add({
                        type: 'success',
                        title: t(state.restartRequired
                          ? 'settings.displayModeRestartNotice'
                          : 'react.done'),
                        description: t('settings.displayMode.borderless'),
                      })
                    })
                    .catch(() => {
                      toast.add({
                        type: 'error',
                        title: t('react.failed'),
                        description: t('settings.displayMode.borderless'),
                      })
                    })
                }}
              >
                {t('settings.displayMode.borderless')}
              </Button>
            </div>
          ) : null}
          <header className="flex items-center justify-between border-b border-border/60 px-7 py-5">
            <div
              className={cn(
                'min-w-0 flex-1 select-none',
                moving ? 'cursor-grabbing' : 'cursor-move',
              )}
              title={t('react.moveHint')}
              onPointerDown={beginMove}
              onPointerMove={moveMove}
              onPointerUp={endMove}
              onPointerCancel={endMove}
            >
              <h1 className="text-xl font-semibold tracking-tight">{title}</h1>
              <p className="mt-1 text-sm text-muted-foreground">{subtitle}</p>
            </div>
            <div className="flex items-center gap-1.5">
              {game ? <Badge variant="secondary">{game}</Badge> : null}
              <PaletteMenu />
              <Button
                variant="outline"
                size="icon"
                title={t('about.projectPage')}
                aria-label={t('about.projectPage')}
                onClick={() => void call('update.open', { url: 'https://github.com/YuiNijika/XMenu' })}
              >
                <GitHubMark className="size-4" aria-hidden="true" />
              </Button>
              <Button
                variant="outline"
                size="icon"
                title={t('react.close')}
                aria-label={t('react.close')}
                onClick={() => void runAction('menu.hide', undefined, 'react.close')}
              >
                <XIcon aria-hidden="true" />
                <span className="sr-only">{t('react.close')}</span>
              </Button>
            </div>
          </header>
          <div ref={scrollRef} className="min-h-0 flex-1 overflow-y-auto">
            <div className="px-7 py-6">{children}</div>
          </div>
        </main>

        <div
          role="separator"
          aria-label={t('react.resizeHint')}
          title={t('react.resizeHint')}
          onPointerDown={beginResize}
          onPointerMove={moveResize}
          onPointerUp={endResize}
          onPointerCancel={endResize}
          tabIndex={0}
          aria-valuetext={t('react.resizeHint')}
          onKeyDown={resizeWithKeyboard}
          className={cn(
            'group absolute right-0 bottom-0 z-40 flex h-8 w-8 cursor-nwse-resize items-end justify-end rounded-tl-lg p-1.5',
            'transition-[background-color,color] duration-150',
            'focus-visible:outline-none focus-visible:bg-primary/15 focus-visible:text-foreground',
            resizing ? 'bg-primary/15 text-foreground' : 'hover:bg-muted/60',
          )}
        >
          <span
            className={cn(
              'pointer-events-none block h-3 w-3 border-r-2 border-b-2',
              resizing ? 'border-primary' : 'border-muted-foreground/60 group-hover:border-foreground',
            )}
          />
        </div>
      </div>
    </Toaster>
  )
}

// 标题栏调色板：明暗 + 强调色预设 + 自定义色相，和 MusicStorm 的调色板一致，只发 --accent-hue
const THEME_ICONS = {
  system: Monitor,
  light: SunMedium,
  dark: MoonStar,
} as const

function PaletteMenu() {
  const { t } = useI18n()
  const { prefs, theme, setTheme, setAccent, setCustomHue } = useAppearance()
  const hue = prefs.accent === 'custom' ? prefs.customHue : 260

  return (
    <DropdownMenu>
      <DropdownMenuTrigger
        className={cn(buttonVariants({ variant: 'outline', size: 'icon' }))}
        title={t('react.appearance')}
        aria-label={t('react.appearance')}
      >
        <Palette className="size-4" aria-hidden="true" />
        <span className="sr-only">{t('react.appearance')}</span>
      </DropdownMenuTrigger>
      <DropdownMenuContent align="end" sideOffset={8} className="w-64 p-3">
        <DropdownMenuGroup>
          <DropdownMenuLabel className="px-0 text-[11px] text-muted-foreground">
            {t('react.theme')}
          </DropdownMenuLabel>
          <DropdownMenuRadioGroup
            value={theme}
            onValueChange={(value) => {
              if (value === 'light' || value === 'dark' || value === 'system') {
                setTheme(value)
              }
            }}
          >
            {THEME_MODES.map((mode) => {
              const Icon = THEME_ICONS[mode]
              return (
                <DropdownMenuRadioItem key={mode} value={mode} className="cursor-pointer">
                  <Icon className="mr-1.5 size-3.5" aria-hidden="true" />
                  {t(THEME_LABEL_KEYS[mode])}
                </DropdownMenuRadioItem>
              )
            })}
          </DropdownMenuRadioGroup>
        </DropdownMenuGroup>

        <DropdownMenuSeparator className="my-3" />

        <DropdownMenuGroup>
          <DropdownMenuLabel className="px-0 text-[11px] text-muted-foreground">
            {t('react.accent')}
          </DropdownMenuLabel>
          <div className="mt-2 flex flex-wrap gap-1.5">
            {ACCENT_OPTIONS.map((option) => {
              const selected = prefs.accent === option.id
              return (
                <button
                  key={option.id}
                  type="button"
                  title={t(option.labelKey)}
                  aria-label={t(option.labelKey)}
                  aria-pressed={selected}
                  onClick={() => setAccent(option.id)}
                  className={cn(
                    'h-7 w-7 cursor-pointer rounded-full transition-transform active:scale-90',
                    'ring-offset-2 ring-offset-popover',
                    selected ? 'ring-2 ring-foreground/80' : 'ring-1 ring-black/10 dark:ring-white/15',
                  )}
                  style={{ backgroundColor: `oklch(0.7 0.15 ${option.hue})` }}
                />
              )
            })}
          </div>
        </DropdownMenuGroup>
        <DropdownMenuSeparator className="my-3" />
        <div className="text-sm">
          <div className="flex items-center justify-between gap-3">
            <span className="min-w-0 truncate text-[11px] text-muted-foreground">
              {t('react.accentCustom')}
            </span>
            <span className="shrink-0 tabular-nums text-[11px] text-muted-foreground">
              {prefs.accent === 'custom' ? hue : '—'}
            </span>
          </div>
          <input
            type="range"
            min={0}
            max={359}
            step={1}
            value={hue}
            aria-label={t('react.accentCustom')}
            onChange={(event) => setCustomHue(Number(event.currentTarget.value))}
            className="mt-1.5 w-full cursor-pointer"
            style={{ accentColor: 'oklch(0.72 0.14 var(--accent-hue))' }}
          />
        </div>
      </DropdownMenuContent>
    </DropdownMenu>
  )
}

// lucide 新版本已移除品牌图标，本地内联 GitHub mark
function GitHubMark({ className }: { className?: string }) {
  return (
    <svg viewBox="0 0 24 24" fill="currentColor" aria-hidden="true" className={className}>
      <path d="M12 2C6.477 2 2 6.584 2 12.253c0 4.526 2.865 8.363 6.839 9.718.5.093.682-.222.682-.493 0-.243-.009-.888-.014-1.743-2.782.618-3.369-1.372-3.369-1.372-.455-1.18-1.11-1.494-1.11-1.494-.908-.635.069-.622.069-.622 1.004.072 1.532 1.057 1.532 1.057.892 1.564 2.341 1.112 2.91.85.091-.662.35-1.112.636-1.367-2.22-.259-4.555-1.138-4.555-5.066 0-1.119.39-2.033 1.03-2.75-.104-.26-.447-1.302.098-2.713 0 0 .84-.275 2.75 1.05A9.35 9.35 0 0 1 12 7.098c.85.004 1.705.117 2.504.343 1.909-1.325 2.747-1.05 2.747-1.05.547 1.411.204 2.453.1 2.713.64.717 1.028 1.631 1.028 2.75 0 3.939-2.339 4.804-4.566 5.058.36.317.679.942.679 1.9 0 1.371-.012 2.477-.012 2.814 0 .274.18.591.688.491C19.138 20.613 22 16.777 22 12.253 22 6.584 17.523 2 12 2Z" />
    </svg>
  )
}

