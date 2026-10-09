import { useState } from 'react'
import { Languages, LoaderCircle } from 'lucide-react'
import { buttonVariants } from '@/components/ui/button'
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuGroup,
  DropdownMenuLabel,
  DropdownMenuRadioGroup,
  DropdownMenuRadioItem,
  DropdownMenuTrigger,
} from '@/components/ui/dropdown-menu'
import { toast } from '@/components/ui/toast'
import { call } from '@/lib/bridge'
import { useI18n } from '@/lib/i18n'

interface LanguageOption {
  code: string
  name: string
}

export function LanguageMenu() {
  const { lang, t } = useI18n()
  const [languages, setLanguages] = useState<LanguageOption[]>([])
  const [loading, setLoading] = useState(false)
  const [pending, setPending] = useState(false)
  const [failed, setFailed] = useState(false)

  const load = async () => {
    setLoading(true)
    setFailed(false)
    try {
      const result = await call<{ languages?: LanguageOption[] }>('settings.appearance')
      if (!Array.isArray(result.languages) || !result.languages.length) throw new Error('No languages')
      setLanguages(result.languages)
    } catch {
      setFailed(true)
    } finally {
      setLoading(false)
    }
  }

  const select = async (code: string) => {
    if (pending || code === lang) return
    setPending(true)
    try {
      const result = await call<{ language: string }>('settings.setLanguage', { code })
      if (result.language !== code) throw new Error('Language not applied')
    } catch {
      toast.add({ type: 'error', title: t('react.failed'), description: t('settings.interfaceLanguage') })
    } finally {
      setPending(false)
    }
  }

  return (
    <DropdownMenu onOpenChange={(open) => { if (open) void load() }}>
      <DropdownMenuTrigger className={buttonVariants({ variant: 'outline', size: 'icon' })}
        title={t('settings.interfaceLanguage')} aria-label={t('settings.interfaceLanguage')}
        data-language-menu>
        {pending ? <LoaderCircle className="size-4 animate-spin motion-reduce:animate-none" aria-hidden="true" />
          : <Languages className="size-4" aria-hidden="true" />}
      </DropdownMenuTrigger>
      <DropdownMenuContent align="end" sideOffset={8} className="min-w-44 max-w-[calc(100vw-2rem)]">
        <DropdownMenuGroup>
          <DropdownMenuLabel>{t('settings.interfaceLanguage')}</DropdownMenuLabel>
          {loading || failed ? (
            <p role="status" className="px-2 py-2 text-xs text-muted-foreground">
              {t(loading ? 'react.loading' : 'react.failed')}
            </p>
          ) : (
            <DropdownMenuRadioGroup value={lang} onValueChange={(code) => void select(code)}>
              {languages.map((language) => (
                <DropdownMenuRadioItem key={language.code} value={language.code} disabled={pending}
                  data-language-choice={language.code} className="min-h-9 break-words">
                  {language.name}
                </DropdownMenuRadioItem>
              ))}
            </DropdownMenuRadioGroup>
          )}
        </DropdownMenuGroup>
      </DropdownMenuContent>
    </DropdownMenu>
  )
}
