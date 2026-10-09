const assert = require('node:assert/strict')
const fs = require('node:fs')
const path = require('node:path')
const { chromium } = require(process.env.PLAYWRIGHT_MODULE || 'playwright')

async function main() {
    const dictionary = JSON.parse(fs.readFileSync(path.resolve(__dirname, '../../src/data/i18n/zh/common.json'), 'utf8'))
    const output = path.resolve(__dirname, '../../build/mods-ui-tests')
    fs.mkdirSync(output, { recursive: true })
    const browser = await chromium.launch({ headless: true, channel: 'msedge' })
    try {
        const page = await browser.newPage({ viewport: { width: 1280, height: 900 } })
        const errors = []
        page.on('pageerror', (error) => errors.push(error.message))
        await page.route('**/data/i18n/**', (route) => route.fulfill({ status: 404, body: '' }))
        await page.addInitScript(({ dictionary }) => {
            const item = {
                id: 'one', short_id: 'short-one', slug: 'test-mod', title: '测试 MOD',
                summary: '适用于罪恶都市的插件。', cover_image: 'https://cos.gtamodx.com/modimages/ymjew503t0m000ddz12m81j3hhypqs1nDIYvBIUyDwJ1DGxPAIJ0.webp',
                author: { name: '测试作者' }, category: { name: '插件', slug: 'plugins' },
                game: '罪恶都市', reposted: false, downloads: 100, views: 200, like_count: 5, comments_count: 3,
            }
            window.catalog = {
                requested: false, loading: false, error: '', gameSlug: 'Grand-Theft-Auto-Vice-City',
                items: [], pagination: { page: 1, limit: 12, total: 24, total_pages: 2 },
                requestedPage: 1, requestedLimit: 12, type: '',
            }
            window.requests = []
            window.opened = []
            window.xbase = {
                on: () => {},
                call: async (method, params = {}) => {
                    if (method === 'i18n.dictionary') return { lang: 'zh', entries: dictionary }
                    if (method === 'menu.info') return { lang: 'zh', version: 'test' }
                    if (method === 'bridge.capabilities') return { game: 'vc', gameName: 'Vice City', methods: {} }
                    if (method === 'ui.schema') return { schema: { tabs: [] }, capabilities: {} }
                    if (method === 'settings.windowModeGet') return { current: 1 }
                    if (method === 'menu.panelRect') return { x: 0, y: 0 }
                    if (method === 'player.snapshot') return { valid: true, health: 100, armour: 0, money: 0, wantedLevel: 0, position: { x: 0, y: 0, z: 0 } }
                    if (method === 'mods.snapshot') return structuredClone(window.catalog)
                    if (method === 'mods.request') {
                        window.requests.push(params)
                        Object.assign(window.catalog, { requested: true, loading: false,
                            requestedPage: params.page, requestedLimit: params.limit, type: params.type || '',
                            items: [item, { ...item, id: 'two', slug: null, short_id: 'short-two', cover_image: 'https://invalid.example/cover.webp', title: 'LongTitle'.repeat(12), reposted: true, original_author: '原作者' }],
                            pagination: { page: params.page, limit: params.limit, total: 24, total_pages: 2 } })
                        return structuredClone(window.catalog)
                    }
                    if (method === 'mods.open' || method === 'mods.game') { window.opened.push({ method, params }); return { ok: true } }
                    return {}
                },
            }
        }, { dictionary })
        await page.goto(process.env.MODS_TEST_URL || 'http://127.0.0.1:5175/ui.html')
        await page.locator('button[data-page="mods"]').click()
        await page.locator('[data-mod-card]').first().waitFor()
        await page.locator('[data-mod-card] img').first().evaluate((image) => image.decode())
        assert.equal(await page.locator('[data-mod-card]').count(), 2)
        await page.getByRole('button', { name: '下一页', exact: true }).click()
        await page.waitForFunction(() => window.catalog.requestedPage === 2)
        assert(await page.getByRole('button', { name: '下一页', exact: true }).isDisabled())
        await page.getByRole('combobox', { name: '分类' }).selectOption('plugins & scripts')
        await page.getByRole('button', { name: '筛选', exact: true }).click()
        await page.waitForFunction(() => window.catalog.type === 'plugins & scripts' && window.catalog.requestedPage === 1)
        await page.getByRole('combobox', { name: '每页数量' }).selectOption('20')
        await page.waitForFunction(() => window.catalog.requestedLimit === 20)
        await page.getByRole('button', { name: '前往下载', exact: true }).nth(1).click()
        await page.getByRole('button', { name: '游戏分类', exact: true }).click()
        assert.deepEqual(await page.evaluate(() => window.opened), [
            { method: 'mods.open', params: { id: 'two' } }, { method: 'mods.game', params: {} },
        ])
        for (const [name, width, height] of [['desktop', 1280, 900], ['mobile', 390, 844]]) {
            await page.setViewportSize({ width, height })
            await page.screenshot({ path: path.join(output, `${name}.png`) })
            assert(await page.locator('[data-mod-card]').evaluateAll((cards) =>
                cards.every((card) => card.getBoundingClientRect().right <= innerWidth + 1)))
        }
        await page.evaluate(() => { window.catalog.loading = true })
        await page.getByText('正在加载作品…', { exact: true }).waitFor()
        assert(await page.getByRole('button', { name: '筛选', exact: true }).isDisabled())
        await page.evaluate(() => { window.catalog.loading = false; window.catalog.error = 'mods.failed' })
        await page.getByRole('alert').waitFor()
        await page.getByRole('button', { name: '重试', exact: true }).click()
        await page.evaluate(() => { window.catalog.error = ''; window.catalog.items = []; window.catalog.pagination.total = 0; window.catalog.pagination.total_pages = 0 })
        await page.getByText('当前筛选下没有作品。', { exact: true }).waitFor()
        assert.deepEqual(errors, [])
        console.log('MOD cards, category filtering, paging, page size, browser actions, image fallback, loading, errors, empty state and responsive layouts passed')
    } finally {
        await browser.close()
    }
}
main().catch((error) => { console.error(error); process.exitCode = 1 })
