import { useEffect, useState } from 'react'
import { Button } from '@/components/ui/button'
import { Separator } from '@/components/ui/separator'
import { ScrollArea } from '@/components/ui/scroll-area'
import { Badge } from '@/components/ui/badge'
import { call } from '@/lib/bridge'
import { useI18n } from '@/lib/i18n'
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
  children: React.ReactNode
}

// 侧边栏固定在左，内容区独立滚动，与 ImGui 版的分区保持一致
export function Shell({ pages, activePage, onSelect, title, subtitle, children }: ShellProps) {
  const { t } = useI18n()
  const [clock, setClock] = useState(() => new Date())

  useEffect(() => {
    const timer = window.setInterval(() => setClock(new Date()), 1000)
    return () => window.clearInterval(timer)
  }, [])

  return (
    <div className="flex h-screen w-screen overflow-hidden bg-background text-foreground">
      <aside className="flex w-56 shrink-0 flex-col border-r border-border/60 bg-sidebar/80 backdrop-blur-xl">
        <div className="px-5 py-5">
          <div className="text-lg font-semibold tracking-tight">XMenu</div>
          <div className="mt-1 text-xs text-muted-foreground">{t('react.reactLabel', 'React 界面')}</div>
        </div>
        <Separator className="opacity-60" />
        <ScrollArea className="flex-1 px-3 py-3">
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
        </ScrollArea>
        <Separator className="opacity-60" />
        <div className="px-5 py-4 text-xs text-muted-foreground">
          <div>{clock.toLocaleTimeString('zh-CN', { hour12: false })}</div>
          <div className="mt-1 opacity-70">{t('react.authorLine', '作者 鼠子 YuiNijika')}</div>
        </div>
      </aside>

      <main className="flex min-w-0 flex-1 flex-col">
        <header className="flex items-center justify-between border-b border-border/60 px-7 py-5">
          <div>
            <h1 className="text-xl font-semibold tracking-tight">{title}</h1>
            <p className="mt-1 text-sm text-muted-foreground">{subtitle}</p>
          </div>
          <div className="flex items-center gap-2">
            <Badge variant="secondary">XBase</Badge>
            <Button variant="outline" size="sm" onClick={() => void call('menu.hide')}>
              ??
            </Button>
            <Button variant="outline" size="sm" onClick={() => void call('menu.close')}>
              ??
            </Button>
          </div>
        </header>
        <ScrollArea className="flex-1">
          <div className="px-7 py-6">{children}</div>
        </ScrollArea>
      </main>
    </div>
  )
}
