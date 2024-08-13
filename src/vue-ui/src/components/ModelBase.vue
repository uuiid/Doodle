<script lang="ts" setup>

import {ref} from "vue";

const file = ref()

function open_model() {
  console.log(file.value.path)
}
</script>

<template>
  <v-container class="fill-height" fluid>
    <v-toolbar class="model base toolbar" flat>
      <v-toolbar-title class="text-h6">模型库</v-toolbar-title>
      <v-spacer></v-spacer>
      <!--      <v-btn icon="mdi-database"></v-btn>-->
      <v-file-input v-model="file" accept=".doodle_db" hide-input prepend-icon="mdi-database"
                    @change="open_model"></v-file-input>
    </v-toolbar>
    <v-row>
      <v-col class="d-flex align-center justify-center">
        <v-sheet class="pa-4" rounded="lg">
          <v-treeview :items="[]"></v-treeview>
        </v-sheet>
      </v-col>
      <v-col class="d-flex align-center justify-center1">
        <v-sheet class="pa-4" rounded="lg">
          <v-treeview :items="[]"></v-treeview>
        </v-sheet>
      </v-col>
    </v-row>
  </v-container>
</template>

<style scoped>

</style>