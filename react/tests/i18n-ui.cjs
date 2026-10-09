const assert = require('node:assert/strict')
const fs = require('node:fs')
const path = require('node:path')
const { chromium } = require(process.env.PLAYWRIGHT_MODULE || 'playwright')

const root = path.resolve(__dirname, '../../src/data/i18n')
const packs = Object.fromEntries(['zh', 'en', 'jp', 'ru'].map((code) => [
    code,
    {
        ...JSON.parse(fs.readFileSync(path.join(root, code, 'common.json'), 'utf8')),
        ...JSON.parse(fs.readFileSync(path.join(root, code, 'settings.json'), 'utf8')),
    },
]))
const output = path.resolve(__dirname, '../../build/language-ui-tests')
fs.mkdirSync(output, { recursive: true })
delete packs.jp['tab.targeting']
packs.jp['tab.weapon'] = ' '

async function main() {
    const browser = await chromium.launch({ headless: true, channel: process.env.PLAYWRIGHT_CHANNEL || 'msedge' })
    try {
        const page = await browser.newPage()
        const errors = []
        page.on('pageerror', (error) => errors.push(error.message))
        let delayedJapanese = false
        await page.route('**/data/i18n/**', async (route) => {
            const url = new URL(route.request().url())
            const [, code, file] = url.pathname.match(/\/i18n\/([^/]+)\/([^/]+)$/) ?? []
            const data = file === 'index.json' && packs[code]
                ? { files: ['common.json'], fallback: code === 'zh' ? '' : 'zh' }
                : file === 'common.json' ? packs[code] : null
            if (delayedJapanese && code === 'jp') await new Promise((resolve) => setTimeout(resolve, 450))
            await route.fulfill({ status: data ? 200 : 404, contentType: 'application/json', body: JSON.stringify(data ?? {}) })
        })
        await page.addInitScript(() => {
            window.testLanguage = 'jp'
            window.testFallback = 'en'
            window.testEntries = {}
            window.testDictionaryCalls = 0
            window.testFailDictionary = false
            window.testFailLanguage = false
            window.testLanguageRequests = []
            window.testLanguageEvents = []
            window.xbase = {
                on: (event, callback) => { if (event === 'i18n.changed') window.testLanguageEvents.push(callback) },
                call: async (method, params = {}) => {
                    if (method === 'menu.info') return { lang: window.testLanguage, fallbackLanguage: window.testFallback }
                    if (method === 'i18n.dictionary') {
                        window.testDictionaryCalls += 1
                        if (window.testFailDictionary) throw new Error('Dictionary unavailable')
                        return { lang: window.testLanguage, entries: window.testEntries }
                    }
                    if (method === 'bridge.capabilities') return { game: 'vc', gameName: 'Vice City', methods: {} }
                    if (method === 'ui.schema') return { schema: { tabs: [] }, capabilities: {} }
                    if (method === 'player.snapshot') return { valid: true, health: 100, armour: 0, money: 0, wantedLevel: 0, position: { x: 0, y: 0, z: 0 } }
                    if (method === 'settings.appearance') return { languages: [
                        { code: 'zh', name: '简体中文' }, { code: 'en', name: 'English' },
                        { code: 'jp', name: '日本語' }, { code: 'ru', name: 'Русский' },
                    ] }
                    if (method === 'settings.setLanguage') {
                        if (window.testFailLanguage) throw new Error('Language failed')
                        window.testLanguageRequests.push(params.code)
                        window.testLanguage = params.code
                        window.testLanguageEvents.forEach((callback) => callback({ lang: params.code }))
                        return { language: params.code }
                    }
                    return {}
                },
            }
        })
        await page.goto(process.env.TARGETING_TEST_URL || 'http://127.0.0.1:5174/ui.html')
        const label = (id) => page.locator(`button[data-page="${id}"]`)
        const waitLabel = async (id, text) => {
            try {
                await page.waitForFunction(({ id, text }) =>
                    document.querySelector(`button[data-page="${id}"]`)?.textContent === text, { id, text }, { timeout: 10000 })
            } catch (error) {
                console.error({ id, expected: text, errors, body: await page.locator('body').innerText() })
                throw error
            }
        }
        await waitLabel('targeting', packs.en['tab.targeting'])
        await waitLabel('weapon', packs.en['tab.weapon'])
        assert.equal(await label('player').textContent(), packs.jp['tab.player'])
        assert(await page.evaluate(() => window.testDictionaryCalls > 0))
        const languageButton = page.locator('[data-language-menu]')
        const palette = await page.getByRole('button', { name: packs.jp['react.appearance'], exact: true }).boundingBox()
        const language = await languageButton.boundingBox()
        assert(palette && language && language.x >= palette.x + palette.width)
        await languageButton.click()
        await page.locator('[data-language-choice="jp"]').waitFor()
        assert.equal(await page.locator('[data-language-choice="jp"]').getAttribute('aria-checked'), 'true')
        await page.locator('[data-language-choice="en"]').click()
        await waitLabel('player', packs.en['tab.player'])
        assert.deepEqual(await page.evaluate(() => window.testLanguageRequests), ['en'])
        await page.keyboard.press('Escape')
        await page.evaluate(() => { window.testFailLanguage = true })
        await languageButton.click()
        await page.locator('[data-language-choice="ru"]').click()
        await page.getByText(packs.en['react.failed'], { exact: true }).waitFor()
        assert.equal(await page.locator('html').getAttribute('lang'), 'en')
        await page.keyboard.press('Escape')
        await page.evaluate(() => { window.testFailLanguage = false })
        await languageButton.click()
        await page.locator('[data-language-choice="jp"]').click()
        await waitLabel('player', packs.jp['tab.player'])
        await page.keyboard.press('Escape')
        for (const [name, width, height] of [['desktop', 1280, 900], ['mobile', 390, 844]]) {
            await page.setViewportSize({ width, height })
            await languageButton.click()
            await page.locator('[data-language-choice="ru"]').waitFor()
            await page.waitForTimeout(200)
            await page.screenshot({ path: path.join(output, `${name}.png`) })
            const boxes = await page.locator('header button').evaluateAll((buttons) => buttons.map((button) => {
                const rect = button.getBoundingClientRect()
                return { left: rect.left, right: rect.right }
            }))
            assert(boxes.every((box) => box.left >= 0 && box.right <= width))
            await page.keyboard.press('Escape')
        }

        await page.evaluate(() => {
            window.testEntries = { 'tab.targeting': 'HOST TARGET' }
            window.testLanguageEvents.forEach((callback) => callback({ lang: 'jp' }))
        })
        await waitLabel('targeting', 'HOST TARGET')

        await page.evaluate(() => {
            window.testEntries = {}
            window.testFallback = 'zh'
            window.testLanguageEvents.forEach((callback) => callback({ lang: 'jp' }))
        })
        await waitLabel('targeting', packs.zh['tab.targeting'])
        await waitLabel('weapon', packs.zh['tab.weapon'])

        await page.evaluate(() => {
            window.testLanguage = 'missing-pack'
            window.testFallback = 'en'
            window.testFailDictionary = true
            window.testLanguageEvents.forEach((callback) => callback({ lang: 'missing-pack' }))
        })
        await waitLabel('player', packs.en['tab.player'])
        await waitLabel('targeting', packs.en['tab.targeting'])

        delayedJapanese = true
        await page.evaluate(() => {
            window.testLanguage = 'jp'
            window.testLanguageEvents.forEach((callback) => callback({ lang: 'jp' }))
        })
        await page.waitForTimeout(40)
        await page.evaluate(() => {
            window.testLanguage = 'ru'
            window.testLanguageEvents.forEach((callback) => callback({ lang: 'ru' }))
        })
        await waitLabel('targeting', packs.ru['tab.targeting'])
        await page.waitForTimeout(600)
        assert.equal(await label('player').textContent(), packs.ru['tab.player'])
        assert.equal(await page.locator('html').getAttribute('lang'), 'ru')
        assert.deepEqual(errors, [])
        console.log('Toolbar language menu, active choice, language changes, failed changes, responsive layouts, fallback and rapid language switches passed')
    } finally {
        await browser.close()
    }
}

main().catch((error) => {
    console.error(error)
    process.exitCode = 1
})
