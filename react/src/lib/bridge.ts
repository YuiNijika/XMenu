export type CapabilityState = 'supported' | 'partial' | 'unsupported'

export type CapabilityReport = {
  protocol: number
  methods: Record<string, CapabilityState>
}

type XBaseBridge = {
  call: <T = unknown>(method: string, params?: Record<string, unknown>) => Promise<T>
  on: (event: string, callback: (payload: unknown) => void) => void
  capabilities: () => Promise<CapabilityReport>
}

declare global {
  interface Window {
    xbase?: XBaseBridge
  }
}

export function isBridgeAvailable(): boolean {
  return typeof window.xbase !== 'undefined'
}

export async function call<T = unknown>(method: string, params?: Record<string, unknown>): Promise<T> {
  if (!window.xbase) {
    throw new Error('XBase 桥不可用，当前面板没有连接到游戏')
  }
  return window.xbase.call<T>(method, params)
}

export function on(event: string, callback: (payload: unknown) => void): void {
  window.xbase?.on(event, callback)
}

export async function capabilities(): Promise<CapabilityReport> {
  return call<CapabilityReport>('bridge.capabilities')
}

export function isUsable(report: CapabilityReport | null, method: string): boolean {
  const state = report?.methods[method]
  return state === 'supported' || state === 'partial'
}

export type PlayerSnapshot = {
  valid: boolean
  position: { x: number; y: number; z: number }
  health: number
  armour: number
  money: number
  wantedLevel: number
}

export type VehicleSnapshot = {
  valid: boolean
  modelId: number
  health: number
  colors: { primary: number; secondary: number }
  lights: boolean
  locked: boolean
}

export type WorldTime = {
  hour: number
  minute: number
}
