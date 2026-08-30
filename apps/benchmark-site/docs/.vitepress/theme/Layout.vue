<script setup lang="ts">
import DefaultTheme from 'vitepress/theme';
import Wallpaper from '../../components/Wallpaper.vue';
</script>

<template>
  <DefaultTheme.Layout>
    <template #layout-top>
      <Wallpaper />
    </template>
  </DefaultTheme.Layout>
</template>
