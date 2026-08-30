<script setup lang="ts">
defineProps<{
  id?: string;
  title?: string;
  subtitle?: string;
  dense?: boolean;
}>();
</script>

<template>
  <el-card :id="id" class="glass panel" :class="{'glass--dense': dense}" shadow="never">
    <template v-if="title || $slots.title || $slots.head" #header>
      <div class="panel__head">
        <div class="panel__titles">
          <h2><slot name="title">{{ title }}</slot></h2>
          <p v-if="subtitle" class="hint panel__subtitle">{{ subtitle }}</p>
        </div>
        <slot name="head" />
      </div>
    </template>
    <slot />
  </el-card>
</template>
