<script lang="ts" setup>
import {computer, task} from '@/store';
import {ref} from 'vue';

/** Props */
// defineProps({
//   msg: {
//     type: String,
//     default: 'Welcome to Vuetify.',
//   },
// });

// defineProps({
//   computer: {
//     name: String,
//     ip: String
//   }
// })
const computers = ref([
  {
    name: 'Computer 1',
    ip: '244.178.44.111',
    status: 'Online',
  },
]);
const fetchComputers = function () {
  computer.get().then((res) => {
    computers.value = res.body;
    // console.log(computers)
  }).catch(e => {
    console.log(e);
  });
};
fetchComputers();
// setInterval(fetchComputers, 50000)

const tasks = ref([]);

const tasks_per_page = ref(10);
const tasks_length = ref(0);
const tasks_handle = [
  {
    title: 'id',
    key: 'id',
  },
  {
    title: '状态',
    key: 'status',
  },
  {
    title: '提交计算机',
    key: 'source_computer',
  },
  {
    title: '提交人',
    key: 'submitter',
  },
  {
    title: '运行计算机',
    key: 'run_computer',
  },
  {
    title: '提交时间',
    key: 'submit_time',
  },
  {
    title: '运行时间',
    key: 'run_time',
  },
  {
    title: '结束时间',
    key: 'end_time',
  },
];

function fetchTasks({
                      page,
                      itemsPerPage,
                    }: {
  page: number;
  itemsPerPage: number;
}) {
  task.get(page, itemsPerPage).then(res => {
    tasks.value = res.body.tasks;
    tasks_length.value = res.body.size;
  });
}

fetchTasks({page: 0, itemsPerPage: tasks_per_page.value});
</script>

<template>
  <v-container class="fill-height">
    <v-data-table :items="computers"></v-data-table>
    <v-data-table-server
        v-model:items-per-page="tasks_per_page"
        :headers="tasks_handle"
        :items="tasks"
        :items-length="tasks_length"
        @update:options="fetchTasks"
    ></v-data-table-server>
  </v-container>
</template>

<style scoped>
.logo:hover {
  will-change: filter;
  filter: drop-shadow(0 0 1em #2196f3aa);
}
</style>
