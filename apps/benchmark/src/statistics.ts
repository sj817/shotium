export function percentile(sortedValues, fraction) {
  if (!sortedValues.length) return null;
  const index = Math.max(0, Math.ceil(fraction * sortedValues.length) - 1);
  return sortedValues[index];
}

export function median(values) {
  if (!values.length) return null;
  const sorted = [...values].sort((left, right) => left - right);
  const middle = Math.floor(sorted.length / 2);
  return sorted.length % 2 ? sorted[middle] : (sorted[middle - 1] + sorted[middle]) / 2;
}

export function distribution(values) {
  const sorted = values.filter(Number.isFinite).sort((left, right) => left - right);
  if (!sorted.length) return null;
  const p50 = median(sorted);
  const deviations = sorted.map((value) => Math.abs(value - p50)).sort((a, b) => a - b);
  return {
    n: sorted.length,
    min: round(sorted[0]),
    p50: round(p50),
    p95: round(percentile(sorted, 0.95)),
    max: round(sorted.at(-1)),
    mean: round(sorted.reduce((sum, value) => sum + value, 0) / sorted.length),
    mad: round(median(deviations)),
  };
}

export function coefficientOfVariation(values) {
  if (values.length < 2) return Number.POSITIVE_INFINITY;
  const mean = values.reduce((sum, value) => sum + value, 0) / values.length;
  if (!mean) return 0;
  const variance = values.reduce((sum, value) => sum + ((value - mean) ** 2), 0) /
      (values.length - 1);
  return Math.sqrt(variance) / mean;
}

export function relativeDrift(values) {
  const finite = values.filter(Number.isFinite);
  if (finite.length < 2) return Number.POSITIVE_INFINITY;
  const base = Math.max(1, median(finite));
  return (Math.max(...finite) - Math.min(...finite)) / base;
}

export function round(value, digits = 3) {
  if (!Number.isFinite(value)) return null;
  const scale = 10 ** digits;
  return Math.round(value * scale) / scale;
}

export function fnv1a(value) {
  let hash = 0x811c9dc5;
  for (const byte of Buffer.from(String(value))) {
    hash ^= byte;
    hash = Math.imul(hash, 0x01000193);
  }
  return hash >>> 0;
}

export function balancedOrder(items, round, seed) {
  if (!items.length) return [];
  const base = [...items];
  let state = fnv1a(seed) || 1;
  for (let index = base.length - 1; index > 0; index -= 1) {
    state ^= state << 13;
    state ^= state >>> 17;
    state ^= state << 5;
    const swap = (state >>> 0) % (index + 1);
    [base[index], base[swap]] = [base[swap], base[index]];
  }
  const shift = round % base.length;
  return [...base.slice(shift), ...base.slice(0, shift)];
}
