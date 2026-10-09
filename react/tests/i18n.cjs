const assert = require('node:assert/strict')
const { loadDictionary, validEntries } = require('../src/lib/i18n-loader.ts')

const indexes = {
    zh: { files: ['common.json'], fallback: '' },
    en: { files: ['common.json'], fallback: 'zh' },
    jp: { files: ['common.json', 'missing.json'], fallback: 'ru' },
    ru: { files: ['common.json'], fallback: 'jp' },
}
const packs = {
    zh: { shared: '中文', onlyDefault: '默认', blank: '默认值', invalid: '有效值' },
    en: { shared: 'English', configured: 'Configured fallback', blank: 'Fallback value' },
    jp: { shared: '日本語', blank: '  ', invalid: 42, echo: 'echo' },
    ru: { declared: 'Declared fallback' },
}
global.fetch = async (url) => {
    const [, code, file] = String(url).match(/\/([^/]+)\/([^/]+)$/) ?? []
    const value = file === 'index.json' ? indexes[code] : file === 'common.json' ? packs[code] : null
    return { ok: Boolean(value), json: async () => value }
}

async function main() {
    const partial = await loadDictionary('jp', 'en')
    assert.equal(partial.shared, '日本語')
    assert.equal(partial.configured, 'Configured fallback')
    assert.equal(partial.declared, 'Declared fallback')
    assert.equal(partial.onlyDefault, '默认')
    assert.equal(partial.blank, 'Fallback value')
    assert.equal(partial.invalid, '有效值')
    assert.equal(partial.echo, undefined)
    const missing = await loadDictionary('missing-language', 'en')
    assert.equal(missing.shared, 'English')
    assert.equal(missing.onlyDefault, '默认')
    const changedFallback = await loadDictionary('jp', 'zh')
    assert.equal(changedFallback.blank, '默认值')
    assert.deepEqual(validEntries({ valid: 'text', empty: '', blank: ' ', number: 1, echo: 'echo' }), { valid: 'text' })
    global.fetch = async () => { throw new Error('offline') }
    assert.deepEqual(await loadDictionary('jp', 'en'), {})
    console.log('Per-key fallback, configured and declared languages, missing packs, blank entries, cycles and fetch failures passed')
}

main().catch((error) => {
    console.error(error)
    process.exitCode = 1
})
