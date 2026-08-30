<script setup lang="ts">
import {onBeforeUnmount, onMounted} from 'vue';

// Colour-field wallpaper behind every glass surface. Pure CSS: three large
// radial blobs drifting on the compositor; see theme/glass.css.
// Stop the compositor when this tab/window is not being viewed. The next
// focus resumes from the same animation time, so the visible effect is
// unchanged while background GPU usage drops to zero.
const PAUSED_CLASS = 'bench-wall-paused';

function syncMotion() {
  document.documentElement.classList.toggle(PAUSED_CLASS, document.hidden || !document.hasFocus());
}

onMounted(() => {
  syncMotion();
  document.addEventListener('visibilitychange', syncMotion);
  window.addEventListener('focus', syncMotion);
  window.addEventListener('blur', syncMotion);
});

onBeforeUnmount(() => {
  document.removeEventListener('visibilitychange', syncMotion);
  window.removeEventListener('focus', syncMotion);
  window.removeEventListener('blur', syncMotion);
  document.documentElement.classList.remove(PAUSED_CLASS);
});
</script>

<template>
  <div class="bench-wall" aria-hidden="true">
    <div class="bench-wall__blob bench-wall__blob--a" />
    <div class="bench-wall__blob bench-wall__blob--b" />
    <div class="bench-wall__blob bench-wall__blob--c" />
  </div>
</template>
