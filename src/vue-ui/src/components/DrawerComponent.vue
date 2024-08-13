<script lang="ts" setup>
import type DrawerMenuItem from '@/interfaces/DrawerMenuItemInterface';

/** Drawer menu items */
const items: DrawerMenuItem[] = [
  {
    title: 'Home',
    icon: 'mdi-home',
    to: { name: 'Home' },
  },
  {
    title: '模型库',
    icon: 'mdi-file-document',
    to: { name: 'ModelView' },
  },
  {
    title: '批量导出',
    icon: 'mdi-lan-connect',
    to: { name: 'BatchExportView' },
  },
  {
    title: '-', // Divider
  },
  {
    title: 'About',
    icon: 'mdi-information',
    to: { name: 'About' },
  },
  {
    title: 'Disabled Item',
    icon: 'mdi-cancel',
    // empty `to` value becomes to disabled item
  },
];
</script>

<template>
  <v-list nav>
    <template v-for="item in items" :key="item.title">
      <v-divider v-if="item.title === '-'" />
      <template v-else>
        <!-- Menu Item -->
        <v-list-item
          v-if="!item.items"
          :disabled="!item.to"
          :prepend-icon="item.icon"
          :title="item.title"
          :to="item.to"
          link
        />
        <!-- Sub menu -->
        <v-list-group v-else-if="item.items" v-model="item.active">
          <template #activator="{ props }">
            <v-list-item
              :prepend-icon="item.icon"
              :title="item.title"
              v-bind="props"
            />
          </template>
          <!-- Sub menu item -->
          <template v-for="subItem in item.items" :key="subItem.title">
            <v-divider v-if="subItem.title === '-'" />
            <v-list-item
              v-else
              :disabled="!subItem.to"
              :prepend-icon="subItem.icon"
              :title="subItem.title"
              :to="subItem.to"
              link
            />
          </template>
        </v-list-group>
      </template>
    </template>
  </v-list>
</template>
