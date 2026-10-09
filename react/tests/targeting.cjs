const assert = require('node:assert/strict')
const fs = require('node:fs')
const path = require('node:path')
const { chromium } = require(process.env.PLAYWRIGHT_MODULE || 'playwright')

const dictionary = JSON.parse(fs.readFileSync(path.resolve(__dirname, '../../src/data/i18n/zh/common.json'), 'utf8'))
const output = path.resolve(__dirname, '../../build/targeting-ui-tests')
fs.mkdirSync(output, { recursive: true })

async function main() {
    const browser = await chromium.launch({ headless: true, channel: process.env.PLAYWRIGHT_CHANNEL || 'msedge' })
    const page = await browser.newPage({ viewport: { width: 1280, height: 900 } })
    const errors = []
    page.on('pageerror', (error) => errors.push(error.message))
    page.on('console', (message) => {
        if (message.type() === 'error') console.error(message.text())
    })
    await page.route('**/data/i18n/**', (route) => route.fulfill({ status: 404, body: '' }))
    await page.addInitScript(({ dictionary }) => {
        const binding = (action, value = 0) => ({ action, value, secondary: 0, tertiary: 0, quaternary: 0, enabled: true })
        const defaults = {
            pedMenu: [binding(1), binding(4, 100), binding(5), binding(2), binding(8), binding(9)],
            vehicleMenu: [binding(1), binding(6), binding(7), binding(3), binding(8), binding(9)],
        }
        const state = {
            enabled: true, drawLinks: true, mouseSelect: true, includePeds: true, includeVehicles: true,
            radius: 80, hitRadius: 160, maxTargets: 16, ...structuredClone(defaults), items: [],
        }
        const action = (action, labelKey, parameter = 0, min = 0, max = 0, defaultValue = 0, supported = true) =>
            ({ action, labelKey, parameter, min, max, defaultValue, supported })
        const shared = [
            action(0, 'targeting.action.none'), action(1, 'targeting.restore'),
            action(8, 'targeting.bring'), action(9, 'targeting.teleport'),
            action(10, 'targeting.action.health', 1, 0, 100000, 100),
            action(11, 'targeting.action.maxHealth', 1, 1, 100000, 100),
            action(19, 'targeting.action.visible', 2),
        ]
        const actions = {
            colorChannels: 2,
            ped: [...shared, action(2, 'targeting.action.kill'), action(4, 'targeting.armour', 1, 0, 100000, 100),
                action(5, 'targeting.disarm'), action(26, 'targeting.action.weapon', 4, 1, 33, 1)],
            vehicle: [...shared, action(3, 'targeting.action.ignite'), action(6, 'targeting.upright'),
                action(45, 'targeting.action.enterVehicle'),
                action(7, 'targeting.unlock'), action(12, 'targeting.action.colors', 3, 0, 255),
                action(13, 'targeting.action.explode'), action(15, 'targeting.action.engine', 2),
                action(27, 'targeting.action.openDoor', 5, 0, 5, 0, false)],
        }
        window.testState = state
        window.testRequests = []
        window.testFail = false
        window.xbase = {
            on: () => {},
            call: async (method, params = {}) => {
                if (method === 'menu.info') return { lang: 'zh', version: 'test' }
                if (method === 'i18n.dictionary') return { lang: 'zh', entries: dictionary }
                if (method === 'bridge.capabilities') return { game: 'vc', gameName: 'Vice City', methods: {} }
                if (method === 'menu.panelRect') return { x: 0, y: 0 }
                if (method === 'settings.windowModeGet') return { current: 1 }
                if (method === 'player.snapshot') return { health: 100, armour: 0, money: 0, wantedLevel: 0, position: { x: 0, y: 0, z: 0 }, valid: true }
                if (method === 'targeting.actions') return structuredClone(actions)
                if (method === 'targeting.snapshot') return structuredClone(state)
                if (method === 'targeting.config') {
                    if (window.testFail) throw new Error('Test bridge failure')
                    window.testRequests.push(structuredClone(params))
                    if (params.reset) Object.assign(state, structuredClone(defaults))
                    else if (params.binding) Object.assign(state[`${params.kind}Menu`][params.slot], params.binding)
                    else Object.assign(state, params)
                    return structuredClone(state)
                }
                if (method === 'ui.schema') return { schema: { tabs: [] }, capabilities: {} }
                return {}
            },
        }
    }, { dictionary })
    await page.goto(process.env.TARGETING_TEST_URL || 'http://127.0.0.1:5174/ui.html')
    await page.waitForTimeout(1500)
    if (errors.length || !await page.locator('button[data-page="targeting"]').count()) {
        await page.screenshot({ path: path.join(output, 'initialization.png') })
        throw new Error(JSON.stringify({ errors, content: await page.locator('body').innerText() }))
    }
    await page.locator('button[data-page="targeting"]').click()
    await page.getByRole('heading', { name: '自定义菜单', exact: true }).waitFor()
    const ped = page.getByRole('region', { name: '人物菜单', exact: true })
    const vehicle = page.getByRole('region', { name: '载具菜单', exact: true })
    await ped.locator('[data-target-slot]').first().waitFor()
    assert.equal(await ped.locator('select,input').count(), 0)
    assert.equal(await vehicle.isVisible(), false)
    const pedWheel = ped.locator('[data-target-slot]')
    await pedWheel.nth(0).dragTo(pedWheel.nth(3))
    await page.waitForFunction(() => window.testState.pedMenu[0].action === 2 && window.testState.pedMenu[3].action === 1)

    const edit = async (region, slot) => {
        await region.locator(`[data-target-slot="${slot}"]`).click({ button: 'right' })
        await page.getByRole('menuitem', { name: '自定义', exact: true }).click()
        await page.getByRole('dialog').waitFor()
    }
    const close = async () => {
        await page.getByRole('dialog').getByRole('button', { name: 'Close', exact: true }).click()
        await page.getByRole('dialog').waitFor({ state: 'hidden' })
    }
    await page.getByRole('tab', { name: '载具菜单', exact: true }).click()
    assert.equal(await vehicle.locator('[data-target-slot]').count(), 6)
    assert.equal(await vehicle.locator('select,input').count(), 0)
    await edit(vehicle, 0)
    assert.equal(await page.getByRole('dialog').getByRole('button', { name: /打开车门/ }).isDisabled(), true)
    await page.getByRole('dialog').getByRole('button', { name: '调色', exact: true }).click()
    const primary = page.getByLabel('主色编号', { exact: true })
    await primary.waitFor()
    await primary.fill('99')
    await page.waitForTimeout(700)
    assert.equal(await primary.inputValue(), '99')
    await primary.press('Enter')
    await page.waitForFunction(() => window.testState.vehicleMenu[0].value === 99)
    const secondary = page.getByLabel('副色编号', { exact: true })
    await secondary.fill('999')
    await secondary.press('Enter')
    await page.waitForFunction(() => window.testState.vehicleMenu[0].secondary === 255)
    assert.equal(await page.getByLabel('第三色编号', { exact: true }).count(), 0)
    await close()
    await edit(vehicle, 1)
    await page.getByRole('dialog').getByRole('button', { name: '直接上车', exact: true }).click()
    await page.waitForFunction(() => window.testState.vehicleMenu[1].action === 45)
    await close()
    await page.getByRole('tab', { name: '人物菜单', exact: true }).click()
    await edit(ped, 0)

    await page.getByRole('dialog').getByRole('button', { name: '给予武器', exact: true }).click()
    const ammo = page.getByLabel('弹药数量', { exact: true })
    await ammo.waitFor()
    await ammo.fill('750')
    await ammo.press('Enter')
    await page.waitForFunction(() => window.testState.pedMenu[0].secondary === 750)
    await close()
    await page.getByRole('tab', { name: '载具菜单', exact: true }).click()
    await edit(vehicle, 1)
    await page.getByRole('dialog').getByRole('button', { name: '引擎', exact: true }).click()
    const engineSwitch = page.getByRole('dialog').getByRole('switch')
    await engineSwitch.waitFor()
    await engineSwitch.click()
    await page.waitForFunction(() => window.testState.vehicleMenu[1].enabled === false)
    await close()
    await vehicle.locator('[data-target-slot="2"]').click({ button: 'right' })
    await page.getByRole('menuitem', { name: '删除动作', exact: true }).click()
    await page.waitForFunction(() => window.testState.vehicleMenu[2].action === 0)
    await vehicle.locator('[data-target-slot="2"]').getByText('空槽位').waitFor()

    await page.evaluate(() => { window.testState.vehicleMenu[2].action = 13 })
    await vehicle.locator('[data-target-slot="2"]').getByText('爆炸', { exact: true }).waitFor()
    const requests = await page.evaluate(() => window.testRequests)
    assert(requests.every((request) => request.reset || request.pedMenu || request.vehicleMenu || Object.keys(request).join(',') === 'kind,slot,binding'))

    await edit(vehicle, 2)
    await page.evaluate(() => { window.testFail = true })
    await page.getByRole('dialog').getByRole('button', { name: '调色', exact: true }).click()
    await page.getByRole('dialog').getByRole('alert').waitFor()
    assert.equal(await page.getByRole('dialog').getByRole('button', { name: '爆炸', exact: true }).getAttribute('aria-pressed'), 'true')
    await page.evaluate(() => { window.testFail = false })
    await close()
    const beforeFailedSwap = await page.evaluate(() => structuredClone(window.testState.vehicleMenu))
    await page.evaluate(() => { window.testFail = true })
    await vehicle.locator('[data-target-slot="0"]').dragTo(vehicle.locator('[data-target-slot="3"]'))
    await vehicle.getByRole('alert').waitFor()
    assert.deepEqual(await page.evaluate(() => window.testState.vehicleMenu), beforeFailedSwap)
    await page.evaluate(() => { window.testFail = false })

    for (const [name, width, height] of [['desktop', 1280, 900], ['compact', 800, 600], ['mobile', 390, 844]]) {
        await page.setViewportSize({ width, height })
        await page.getByRole('heading', { name: '自定义菜单', exact: true }).scrollIntoViewIfNeeded()
        await page.screenshot({ path: path.join(output, `${name}.png`) })
        const overflow = await page.evaluate(() => {
            const controls = [...document.querySelectorAll('[data-target-slot]')].filter((element) => element.getBoundingClientRect().width > 0)
            return controls.filter((element) => {
                const rect = element.getBoundingClientRect()
                return rect.width < 30 || rect.right > innerWidth + 1 || rect.left < -1
            }).map((element) => element.dataset.targetSlot)
        })
        assert.deepEqual(overflow, [], `${name} controls overflow`)
        await edit(vehicle, 0)
        await page.waitForFunction(() => {
            const dialog = document.querySelector('[role="dialog"]')
            return dialog && getComputedStyle(dialog).opacity === '1'
        })
        await page.waitForTimeout(150)
        await page.screenshot({ path: path.join(output, `${name}-editor.png`) })
        const dialog = await page.getByRole('dialog').boundingBox()
        assert(dialog && dialog.x >= 0 && dialog.y >= 0 && dialog.x + dialog.width <= width + 1 && dialog.y + dialog.height <= height + 1)
        await close()
    }
    await page.getByRole('button', { name: '恢复默认菜单', exact: true }).click()
    await page.waitForFunction(() => window.testState.vehicleMenu[0].action === 1 && window.testState.pedMenu[0].action === 1)
    assert.deepEqual(errors, [])
    await browser.close()
    console.log('Tabs, wheel drag, context editing, deletion, parameters, enter vehicle, synchronization, errors and responsive layouts passed')
}

main().catch((error) => {
    console.error(error)
    process.exit(1)
})
