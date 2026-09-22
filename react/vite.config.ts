import path from 'node:path'
import tailwindcss from '@tailwindcss/vite'
import react from '@vitejs/plugin-react'
import { defineConfig, type Plugin } from 'vite'

// 产物是 iife 经典脚本，但 Vite 仍按模块标签输出，file 协议下模块与 crossorigin 会被拦下
function classicBundleHtml(): Plugin {
  return {
    name: 'xmenu-classic-bundle-html',
    apply: 'build',
    enforce: 'post',
    transformIndexHtml(html) {
      return html
        .replace(/\s+type="module"/g, '')
        .replace(/\s+crossorigin/g, '')
        .replace(/<script(?![^>]*\bdefer\b)([^>]*)>/g, '<script defer$1>')
    },
  }
}

// 面板用 file 协议加载，资源路径必须相对；脚本输出经典 iife，样式与脚本统一放进 data 目录
export default defineConfig({
  base: './',
  plugins: [react(), tailwindcss(), classicBundleHtml()],
  resolve: {
    alias: {
      '@': path.resolve(import.meta.dirname, 'src'),
    },
  },
  build: {
    outDir: 'dist',
    emptyOutDir: true,
    cssCodeSplit: false,
    assetsDir: 'data',
    rollupOptions: {
      input: path.resolve(import.meta.dirname, 'ui.html'),
      output: {
        format: 'iife',
        inlineDynamicImports: true,
        entryFileNames: 'data/app.js',
        assetFileNames: (info) => {
          const name = info.names?.[0] ?? info.name ?? ''
          return name.endsWith('.css') ? 'data/app.css' : 'data/[name][extname]'
        },
      },
    },
  },
})
