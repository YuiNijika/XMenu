import { useCallback, useEffect, useRef, useState } from 'react'
import { call, capabilities, isBridgeAvailable, type CapabilityReport } from '@/lib/bridge'

// 能力表只在挂载时取一次，之后用它决定入口是否可用
export function useCapabilities() {
  const [report, setReport] = useState<CapabilityReport | null>(null)
  const [error, setError] = useState<string | null>(null)

  useEffect(() => {
    if (!isBridgeAvailable()) {
      setError('XBase 桥不可用')
      return
    }
    let cancelled = false
    capabilities()
      .then((value) => {
        if (!cancelled) setReport(value)
      })
      .catch((reason: unknown) => {
        if (!cancelled) setError(reason instanceof Error ? reason.message : String(reason))
      })
    return () => {
      cancelled = true
    }
  }, [])

  return { report, error }
}

// 轮询式读取，面板本身是被动渲染，不需要更复杂的状态同步
export function usePolling<T>(method: string, intervalMs = 500) {
  const [value, setValue] = useState<T | null>(null)
  const [error, setError] = useState<string | null>(null)
  const busy = useRef(false)

  const refresh = useCallback(() => {
    if (busy.current) return
    busy.current = true
    call<T>(method)
      .then((result) => {
        setValue(result)
        setError(null)
      })
      .catch((reason: unknown) => {
        setError(reason instanceof Error ? reason.message : String(reason))
      })
      .finally(() => {
        busy.current = false
      })
  }, [method])

  useEffect(() => {
    refresh()
    const timer = window.setInterval(refresh, intervalMs)
    return () => window.clearInterval(timer)
  }, [refresh, intervalMs])

  return { value, error, refresh }
}
