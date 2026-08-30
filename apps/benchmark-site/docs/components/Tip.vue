<script setup lang="ts">
import {InfoFilled} from '@element-plus/icons-vue';

/*
 * Element Plus tooltip that opens on hover and on keyboard focus. The
 * trigger is the default slot; without one an info icon is drawn. Rich
 * content goes in the `content` slot.
 */
defineProps<{text?: string}>();
const slots = defineSlots<{default?: () => unknown; content?: () => unknown}>();
</script>

<template>
  <el-tooltip
    :content="text"
    :disabled="!text && !slots.content"
    placement="top"
    popper-class="bench-pop"
    :trigger="['hover', 'focus']"
    :show-after="100"
  >
    <template v-if="slots.content" #content>
      <slot name="content" />
    </template>
    <span class="tip" tabindex="0">
      <slot>
        <el-icon class="tip-icon"><InfoFilled /></el-icon>
        <span class="sr-only">{{ text }}</span>
      </slot>
    </span>
  </el-tooltip>
</template>
