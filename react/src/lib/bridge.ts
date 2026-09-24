import { translateKey } from '@/lib/i18n'

export type CapabilityState = 'supported' | 'partial' | 'unsupported'

export type CapabilityReport = {
  protocol: number
  game: string
  gameName: string
  methods: Record<string, CapabilityState>
}

type XBaseBridge = {
  call: <T = unknown>(method: string, params?: Record<string, unknown>) => Promise<T>
  on: (event: string, callback: (payload: unknown) => void) => void
  capabilities: () => Promise<CapabilityReport>
}

type RawTransport = {
  postMessage: (message: unknown) => void
}

declare global {
  interface Window {
    xbase?: XBaseBridge
    chrome?: {
      webview?: RawTransport
    }
  }
}

export function isBridgeAvailable(): boolean {
  return typeof window.xbase !== 'undefined'
}

// 客户端封装缺失时仍能直接投递消息，宿主侧的消息处理器与它无关
export function hasRawTransport(): boolean {
  return typeof window.chrome?.webview?.postMessage === 'function'
}

export function rawCall(method: string, params?: Record<string, unknown>): boolean {
  if (!hasRawTransport()) {
    return false
  }
  window.chrome?.webview?.postMessage({ id: Date.now(), method, params: params ?? {} })
  return true
}

export async function call<T = unknown>(method: string, params?: Record<string, unknown>): Promise<T> {
  if (!window.xbase) {
    throw new Error(translateKey('react.bridgeOffline'))
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

// 界面特性注册表，与 ImGui 共用同一份配置
export type UiControl = {
  id: string
  kind: 'toggle' | 'float' | 'int' | 'action' | 'select'
  labelKey: string
  state?: string
  capability?: string
  games?: string[]
  min?: number
  max?: number
  step?: number
  format?: string
  visibleWhen?: string
  onChange?: string
  // 下拉选择的选项来自宿主的哪张表
  source?: string
}

// 下拉选项：label 是要翻译的词条键还是现成的名字
export type UiSelectOption = {
  value: string
  label: string
  translated: boolean
}

export type UiSection = {
  id: string
  labelKey: string
  hintKey?: string
  capability?: string
  games?: string[]
  columns?: number
  // 内联分区按固定宽度排布并自动换行，适合一排同类型的动作按钮
  inline?: boolean
  controls: UiControl[]
}

export type UiPage = {
  id: string
  labelKey: string
  sections?: UiSection[]
}

export type UiTab = {
  id: string
  labelKey: string
  pages?: UiPage[]
}

export type UiSchema = {
  version?: number
  tabs: UiTab[]
}

// 宿主把能力表一起返回，网页端按 FeatureCapability 名门控
export type UiSchemaPayload = {
  schema: UiSchema
  capabilities: Record<string, string>
}

export async function fetchUiSchema(): Promise<UiSchemaPayload> {
  return call<UiSchemaPayload>('ui.schema')
}
