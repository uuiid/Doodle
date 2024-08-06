<script lang="ts" setup>


</script>

<template>
  <v-container class="fill-height">
    <v-row class="d-flex align-center justify-center1">
      <p class="text-h5">模型库</p>
      <v-col class="d-flex align-center justify-center" cols="2">
        <v-sheet class="pa-4" rounded="lg">
          <v-treeview :items="[]"></v-treeview>
        </v-sheet>
        <v-sheet class="pa-4" rounded="lg">
          <v-treeview :items="[]"></v-treeview>
        </v-sheet>
      </v-col>
    </v-row>
  </v-container>
</template>

<style scoped>

</style>