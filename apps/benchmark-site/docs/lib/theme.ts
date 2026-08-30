import {onBeforeUnmount, onMounted, ref, type Ref} from 'vue';

/** Read a design token (theme/tokens.css) from the document root. */
export function cssVar(name: string, fallback = ''): string {
  if (typeof document === 'undefined') return fallback;
  return getComputedStyle(document.documentElement).getPropertyValue(name).trim() || fallback;
}

/**
 * A counter that increments whenever VitePress toggles `html.dark`, so
 * anything that bakes token values into JavaScript (chart options) can be a
 * computed that depends on it and recolours itself.
 */
export function useThemeTick(): Ref<number> {
  const tick = ref(0);
  let observer: MutationObserver | null = null;
  onMounted(() => {
    observer = new MutationObserver(() => {
      tick.value += 1;
    });
    observer.observe(document.documentElement, {attributes: true, attributeFilter: ['class']});
  });
  onBeforeUnmount(() => observer?.disconnect());
  return tick;
}

/** True while the viewport matches the query (default: phones). */
export function useNarrow(query = '(max-width: 767px)'): Ref<boolean> {
  const narrow = ref(false);
  let media: MediaQueryList | null = null;
  const update = () => {
    narrow.value = media?.matches ?? false;
  };
  onMounted(() => {
    media = window.matchMedia(query);
    update();
    media.addEventListener('change', update);
  });
  onBeforeUnmount(() => media?.removeEventListener('change', update));
  return narrow;
}

/** Colours a chart needs, read from the tokens at call time. */
export function chartPalette() {
  return {
    ink1: cssVar('--ink-1', '#1b2229'),
    ink2: cssVar('--ink-2', '#4b5560'),
    ink3: cssVar('--ink-3', '#57626e'),
    line1: cssVar('--line-1', 'rgba(27,34,41,.14)'),
    line2: cssVar('--line-2', 'rgba(27,34,41,.08)'),
    pop: cssVar('--solid-fill-pop', '#ffffff'),
    edge: cssVar('--glass-edge', 'rgba(255,255,255,.7)'),
    font: cssVar('--bench-font', 'Inter, system-ui, sans-serif'),
  };
}
