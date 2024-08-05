<script lang="ts" setup>
import { version } from 'vuetify';

import Meta from '@/Meta';

const title = import.meta.env.VITE_APP_TITLE;

/** Props */
defineProps({
  msg: {
    type: String,
    default: 'doodle 工具包',
  },
});
</script>

<template>
  <v-container class="fill-height">
    <v-responsive class="align-center text-center fill-height">
      <v-img
        :width="256"
        alt="vuetify"
        class="logo mx-auto"
        src="@/assets/logo.svg"
      />
      <h1 class="text-h2 font-weight-bold mb-3">
        doodle 工具包
      </h1>
      <p>{{ msg }}</p>
      <p class="text-medium-emphasis">
        核心版本
        <v-code tag="code">{{ 'v' + version }}</v-code>
        Build:
        <time
          :datetime="Meta.date"
          v-text="new Date(Meta.date).toLocaleString()"
        />
      </p>
      <v-alert
        v-if="!title"
        class="my-3 w-75 mx-auto"
        density="compact"
        type="warning"
        variant="tonal"
      >
        Please define
        <v-code tag="code">VITE_APP_TITLE</v-code>
        variable in to
        <v-code tag="code">.env</v-code>
        file.
      </v-alert>
      <div class="py-10" />
    </v-responsive>
  </v-container>
</template>

<style scoped>
.logo:hover {
  will-change: filter;
  filter: drop-shadow(0 0 1em #2196f3aa);
}
</style>
