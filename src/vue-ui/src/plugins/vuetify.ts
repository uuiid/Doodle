/**
 * Vuetify3 Plugin
 */
import {createVuetify, type VuetifyOptions} from 'vuetify';
import * as components from 'vuetify/components';
import * as directives from 'vuetify/directives';
// import { aliases, mdi } from 'vuetify/iconsets/mdi-svg';
import * as labsComponents from 'vuetify/labs/components';
import {VTreeview} from 'vuetify/labs/VTreeview'
// Translations provided by Vuetify
import {zhHans} from 'vuetify/locale';

// Misc
// import { loadFonts } from '@/plugins/webfontloader';

// Styles
import 'vuetify/styles';
import '@mdi/font/css/materialdesignicons.css';

// await loadFonts();

/**
 * Vuetify Components
 *
 * @see {@link https://vuetifyjs.com/en/features/treeshaking/}
 */
let vuetifyConfig: VuetifyOptions = {
    // Global configuration
    // https://vuetifyjs.com/en/features/global-configuration/
    /*
    defaults: {
      global: {
        ripple: false,
      },
      VSheet: {
        elevation: 4,
      },
    },
    */
    /*
    // Icon Fonts
    // https://vuetifyjs.com/en/features/icon-fonts/
    icons: {
      defaultSet: 'mdi',
      aliases,
      sets: {
        mdi,
      },
    },
    */
    // Internationalization (i18n)
    // https://vuetifyjs.com/en/features/internationalization/#internationalization-i18n
    locale: {
        locale: 'zhHans',
        fallback: 'zhHans',
        messages: {zhHans},
    },
    // Theme
    // https://vuetifyjs.com/en/features/theme/
    theme: {
        defaultTheme: 'light',
    },
};

if (import.meta.env.DEV) {
    // Disable treeshaking for DEV mode.
    vuetifyConfig = {
        components: {components, labsComponents, VTreeview},
        directives,
        ...vuetifyConfig,
    };
}
export default createVuetify(vuetifyConfig);

// Export for test.
export {components, directives};
