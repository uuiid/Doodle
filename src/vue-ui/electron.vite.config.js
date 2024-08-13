import { defineConfig, externalizeDepsPlugin, defineViteConfig } from 'electron-vite';
import { resolve, join } from 'path';
import vue from '@vitejs/plugin-vue';
import { builtinModules } from 'module';
import vuetify, { transformAssetUrls } from 'vite-plugin-vuetify';
import { checker } from 'vite-plugin-checker';
import { fileURLToPath, URL } from 'node:url';

export default defineConfig(({ command, mode }) => {
  const PACKAGE_ROOT = __dirname;
  return {
    main: {
      plugins: [externalizeDepsPlugin()],
      build: {
        lib: {
          entry: resolve(__dirname, 'electron/main/index.ts'),
          formats: ['cjs'],
        },
        rollupOptions: {
          input: {
            index: resolve(__dirname, 'electron/main/index.ts'),
          },
        },
      },
    },
    preload: {
      plugins: [externalizeDepsPlugin()],
      build: {
        sourcemap: true,
        minify: process.env.MODE !== 'development',
        lib: {
          entry: resolve(__dirname, 'electron/preload/index.ts'),
          formats: ['cjs'],
        },
        rollupOptions: {
          external: ['electron', ...builtinModules],
          input: {
            index: resolve(__dirname, 'electron/preload/index.ts'),
          },
          output: {
            entryFileNames: '[name].cjs',
          },
        },
      },
    },
    renderer: defineViteConfig(
      ({ command, mode }) => {
        return {
          server: {
            host: true,
          },
          plugins: [vue({
            template: {
              // https://github.com/vuetifyjs/vuetify-loader/tree/next/packages/vite-plugin#image-loading
              transformAssetUrls,
            },
          }), vuetify({
            autoImport: true,
            styles: { configFile: 'src/styles/settings.scss' },
          }),
            // vite-plugin-checker
            // https://github.com/fi3ework/vite-plugin-checker
            checker({
              typescript: true,
              // vueTsc: true,
              // eslint: { lintCommand: 'eslint' },
              // stylelint: { lintCommand: 'stylelint' },
            })],
          root: '.',
          build: {
            sourcemap: true,
            rollupOptions: {
              input: {
                index: resolve(__dirname, 'index.html'),
              },
            },
          },
          resolve: {
            // https://vitejs.dev/config/shared-options.html#resolve-alias
            // vue: 'vue/dist/vue.esm.js',
            extensions: ['.mjs', '.js', '.ts', '.jsx', '.tsx', '.json', '.vue'],
            alias: {
              '@': fileURLToPath(new URL('./src', import.meta.url)),
              '~': fileURLToPath(new URL('./node_modules', import.meta.url)),
            },
          },
          css: {
            preprocessorOptions: {
              scss: {
                additionalData: ``,
              },
            },
          },
        };
      },
    ),
  };
});
