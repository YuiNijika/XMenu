import { toast } from '@/components/ui/toast'
import { call } from '@/lib/bridge'
import { translateKey } from '@/lib/i18n'

export type ActionParams = Record<string, unknown> | undefined

// 所有交互统一走这里，结果用提示条回传，成功与失败各一种样式
export async function runAction(method: string, params?: ActionParams, labelKey?: string): Promise<boolean> {
  const label = labelKey ? translateKey(labelKey) : method
  try {
    await call(method, params)
    toast.add({
      type: 'success',
      title: translateKey('react.done'),
      description: label,
    })
    return true
  } catch (reason: unknown) {
    const message = reason instanceof Error ? reason.message : String(reason)
    toast.add({
      type: 'error',
      title: translateKey('react.failed'),
      description: message ? `${label} · ${message}` : label,
    })
    return false
  }
}

// 只提示不显示动作名的场景，例如切换开关
export async function runActionQuiet(method: string, params?: ActionParams): Promise<boolean> {
  try {
    await call(method, params)
    return true
  } catch (reason: unknown) {
    const message = reason instanceof Error ? reason.message : String(reason)
    toast.add({
      type: 'error',
      title: translateKey('react.failed'),
      description: message,
    })
    return false
  }
}
