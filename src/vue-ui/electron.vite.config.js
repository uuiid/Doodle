import { defineConfig, externalizeDepsPlugin } from 'electron-vite'
import { resolve, join } from 'path'
import vue from '@vitejs/plugin-vue2'
import { builtinModules } from 'module'

export default defineConfig(({ command, mode }) => {
  const PACKAGE_ROOT = __dirname
  return {
    main: {
      plugins: [externalizeDepsPlugin()],
      build: {
        lib: {
          entry: resolve(__dirname, 'electron/main/index.ts'),
          formats: ['cjs']
        },
        rollupOptions: {
          input: {
            index: resolve(__dirname, 'electron/main/index.ts')
          }
        }
      }
    },
    preload: {
      plugins: [externalizeDepsPlugin()],
      build: {
        sourcemap: true,
        minify: process.env.MODE !== 'development',
        lib: {
          entry: resolve(__dirname, 'electron/preload/index.ts'),
          formats: ['cjs']
        },
        rollupOptions: {
          external: ['electron', ...builtinModules],
          input: {
            index: resolve(__dirname, 'electron/preload/index.ts')
          },
          output: {
            entryFileNames: '[name].cjs'
          }
        }
      }
    },
    renderer: {
      plugins: [vue()],
      root: '.',
      build: {
        sourcemap: true,
        rollupOptions: {
          input: {
            index: resolve(__dirname, 'index.html')
          }
        }
      },
      resolve: {
        vue: 'vue/dist/vue.esm.js',
        extensions: ['.mjs', '.js', '.ts', '.jsx', '.tsx', '.json', '.vue'],
        alias: {
          '@': join(PACKAGE_ROOT, 'src') + '/'
        }
      },
      css: {
        preprocessorOptions: {
          scss: {
            additionalData: `@import "@/variables.scss";`
          }
        }
      }
    }
  }
})
