import { useEffect, useRef, useState } from 'react'
import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'
import { Separator } from '@/components/ui/separator'
import { Toaster, toast } from '@/components/ui/toast'
import { call } from '@/lib/bridge'
import { runAction } from '@/lib/actions'
import { useI18n } from '@/lib/i18n'
import { EyeOffIcon, XIcon } from 'lucide-react'
import { cn } from 'cn'

export type MenuPage = {
  id: string
  label: string
}

type ShellProps = {
  pages: MenuPage[]
  activePage: string
  onSelect: (id: string) => void
  title: string
  subtitle: string
  game: string
  children: React.ReactNode
}

type DragState = {
  x: number
  y: number
  width: number
  height: number
}

// 侧边栏固定在左，内容区独立滚动，右下角可拖动改面板尺寸
export function Shell({ pages, activePage, onSelect, title, subtitle, game, children }: ShellProps) {
  const { t } = useI18n()
  const dragRef = useRef<DragState | null>(null)
  const [resizing, setResizing] = useState(false)
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
        <aside className="flex w-56 shrink-0 flex-col border-r border-border/60 bg-sidebar/80 backdrop-blur-xl">
          <div className="px-5 py-5">
            <div className="text-lg font-semibold tracking-tight">XMenu</div>
            <div className="mt-1 text-xs text-muted-foreground">{t('react.reactLabel')}</div>
          </div>
          <Separator className="opacity-60" />
          <div className="min-h-0 flex-1 overflow-y-auto px-3 py-3">
            <nav className="flex flex-col gap-1">
              {pages.map((page) => (
                <button
                  key={page.id}
                  type="button"
                  onClick={() => onSelect(page.id)}
                  className={cn(
                    'rounded-lg px-3 py-2 text-left text-sm transition-colors',
                    page.id === activePage
                      ? 'bg-primary text-primary-foreground'
                      : 'text-muted-foreground hover:bg-muted hover:text-foreground',
                  )}
                >
                  {page.label}
                </button>
              ))}
            </nav>
          </div>
          <Separator className="opacity-60" />
          <div className="px-5 py-4 text-xs text-muted-foreground">
            <div className="opacity-70">{t('react.authorLine')}</div>
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
              className="min-w-0 flex-1 cursor-move select-none"
              title={t('react.moveHint')}
              onPointerDown={beginMove}
              onPointerMove={moveMove}
              onPointerUp={endMove}
              onPointerCancel={endMove}
            >
              <h1 className="text-xl font-semibold tracking-tight">{title}</h1>
              <p className="mt-1 text-sm text-muted-foreground">{subtitle}</p>
            </div>
            <div className="flex items-center gap-2">
              <Badge variant="secondary">{game || 'XBase'}</Badge>
              <Button
                variant="outline"
                size="sm"
                title={t('react.hide')}
                onClick={() => void runAction('menu.hide', undefined, 'react.hide')}
              >
                <EyeOffIcon aria-hidden="true" />
                <span className="sr-only">{t('react.hide')}</span>
              </Button>
              <Button
                variant="outline"
                size="sm"
                title={t('react.close')}
                onClick={() => void runAction('menu.close', undefined, 'react.close')}
              >
                <XIcon aria-hidden="true" />
                <span className="sr-only">{t('react.close')}</span>
              </Button>
            </div>
          </header>
          <div className="min-h-0 flex-1 overflow-y-auto">
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
          className={cn(
            'group absolute right-0 bottom-0 z-40 flex h-6 w-6 cursor-nwse-resize items-end justify-end p-1',
            resizing && 'text-foreground',
          )}
        >
          <span className="pointer-events-none block h-3 w-3 border-r-2 border-b-2 border-muted-foreground/60 group-hover:border-foreground" />
        </div>
      </div>
    </Toaster>
  )
}
