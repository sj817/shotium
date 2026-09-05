// The statistics behind the performance gate: paired bootstrap intervals of
// candidate/baseline ratios, the verdict each case gets, and the equivalence
// bands an A/A calibration reads off a machine.
//
// The percentile is the numpy one (linear interpolation on (n - 1) * p), kept
// as four lines rather than a library's because every recorded result and
// every verdict in benchmark-results/ was produced by exactly this
// definition, and a library that interpolates differently would move them.

export interface Distribution {
  p50: number;
  p95: number;
  mean: number;
}

export interface Interval {
  lo: number;
  hi: number;
}

export type Bands = {primary: number; tail: number};
export type Tolerance = number | Bands;
export type MetricVerdict = 'faster' | 'equivalent' | 'slower' | 'uncertain';
export type Status = 'faster' | 'equivalent' | 'slower' | 'unproven' | 'insufficient-samples';

export interface Comparison {
  summary: {baseline: Distribution; candidate: Distribution};
  ratios: Record<Metric, number>;
  intervals: Record<Metric, Interval>;
  verdicts: Record<Metric, MetricVerdict>;
  upper99: number;
  tolerance: Bands;
  status: Status;
  samples: number;
  throughput: {baseline: number; candidate: number};
}

export function percentile(values: number[], p: number): number {
  const sorted = [...values].sort((a, b) => a - b);
  const index = (sorted.length - 1) * p / 100;
  return sorted[Math.floor(index)] + (sorted[Math.ceil(index)] - sorted[Math.floor(index)]) * (index % 1);
}

export function distribution(values: number[]): Distribution {
  if (!values.length || values.some((v) => !Number.isFinite(v) || v <= 0)) {
    throw new Error('Timing samples must be finite and positive');
  }
  return {p50: percentile(values, 50), p95: percentile(values, 95), mean: values.reduce((sum, v) => sum + v, 0) / values.length};
}

// The three statistics the gate reads, in the order it reports them. p50 and
// mean are the two it can require an improvement on; p95 is only ever a tail
// guard, because a run's slowest few samples are the noisiest thing measured
// and holding a win to needing them lower as well rejects real wins for noise.
export const METRICS = ['p50', 'mean', 'p95'] as const;
export const PRIMARY = ['p50', 'mean'] as const;
export type Metric = typeof METRICS[number];

// A deterministic paired resample of (baseline, candidate). The same index is
// drawn for both, so a pair stays together, which is what makes the ratio a
// paired statistic. Seeded from the data length so two callers comparing the
// same-sized runs shuffle identically and a result reproduces.
function bootstrapRatios(baseline: number[], candidate: number[], resamples: number) {
  let seed = 0x817 ^ baseline.length;
  const random = () => {
    seed ^= seed << 13;
    seed ^= seed >>> 17;
    seed ^= seed << 5;
    return (seed >>> 0) / 0x100000000;
  };
  const perMetric: Record<Metric, number[]> = {p50: [], mean: [], p95: []};
  const joint: number[] = [];
  for (let iteration = 0; iteration < resamples; iteration++) {
    const a: number[] = [], b: number[] = [];
    for (let i = 0; i < baseline.length; i++) {
      const index = Math.floor(random() * baseline.length);
      a.push(baseline[index]);
      b.push(candidate[index]);
    }
    const da = distribution(a), db = distribution(b);
    let worst = 0;
    for (const metric of METRICS) {
      const ratio = db[metric] / da[metric];
      perMetric[metric].push(ratio);
      worst = Math.max(worst, ratio);
    }
    joint.push(worst);
  }
  return {perMetric, joint};
}

// The equivalence bands: one for the body of the distribution (p50 and the
// mean) and one for its tail (p95). A single number means the same band for
// both, which is what a test or a caller without a calibration passes.
//
// Two bands rather than one because the two statistics have nothing like the
// same noise. On a 3 ms case a binary timed against itself puts its p50 and
// mean ratios within a few percent of 1; its p95 ratio wanders by a third,
// because the slowest five samples in a hundred are whichever ones the
// scheduler interrupted. One band wide enough for that tail called a 30%
// slowdown of the median "equivalent", which is the thing a band is not for.
export function bandsOf(tolerance: Tolerance): Bands {
  const bands = typeof tolerance === 'number' ? {primary: tolerance, tail: tolerance} : {...tolerance};
  for (const key of ['primary', 'tail'] as const) {
    if (!(bands[key] >= 0)) throw new Error('tolerance must be a non-negative fraction');
  }
  return bands;
}

// One metric's verdict against the equivalence band [1, 1 + tolerance].
//
// `tolerance` is how much slower than the baseline still counts as the same
// speed on this machine -- the measurement noise, established by comparing a
// binary against itself (see `calibrate`), not a slowdown anyone is allowing.
// A ratio is candidate / baseline, so below 1 is faster.
//
//   faster      the whole interval is under 1: even the unlucky end is quicker
//   equivalent  the whole interval is inside the band: indistinguishable, not
//               provably faster -- what a binary scores against itself
//   slower      the whole interval is past the band: a real regression
//   uncertain   the interval straddles the band's far edge: not yet decided
function classifyMetric(interval: Interval, tolerance: number): MetricVerdict {
  if (interval.hi <= 1) return 'faster';
  if (interval.lo > 1 + tolerance) return 'slower';
  if (interval.hi <= 1 + tolerance) return 'equivalent';
  return 'uncertain';
}

// Compares one case's paired samples and returns a calibrated verdict.
//
// `tolerance` is the equivalence band from A/A calibration -- `{primary,
// tail}`, or one number for both; 0 makes the gate strict (only a measured
// improvement is not a regression), which is the floor the calibration is
// never allowed to fall below. The verdict is one of:
//
//   faster       the mean is quicker, or the median is quicker and the mean
//                within its band -- and no statistic is slower
//   equivalent   within the noise band on p50 and the mean, quicker on neither
//   slower       past the noise band on some statistic
//   unproven     otherwise: the mean, or the median, is still undecided
//
// The mean is the throughput -- captures per second is its reciprocal -- so
// a mean whose whole interval is under 1 settles the case on its own, even
// when the median's interval still straddles the band. A quicker median
// needs the mean at least within the band, so that a median win is not
// bought with a heavier tail. p95 can only make a case `slower`, and only
// when its whole interval is past the tail band.
//
// An engine case -- one whose cost is raster, encode, layout or file IO --
// must reach `faster`. A case pinned to an external wait (a fixed server
// delay, a network round trip) can only be asked not to be `slower`.
export function compare(
    baseline: number[], candidate: number[],
    {minimumSamples = 100, resamples = 2000, tolerance = 0 as Tolerance, interval = 99} = {}): Comparison {
  if (baseline.length !== candidate.length) throw new Error('Unpaired samples');
  const bands = bandsOf(tolerance);
  const summary = {baseline: distribution(baseline), candidate: distribution(candidate)};
  const ratios = Object.fromEntries(METRICS.map((metric) => [metric, summary.candidate[metric] / summary.baseline[metric]])) as Record<Metric, number>;
  const {perMetric, joint} = bootstrapRatios(baseline, candidate, resamples);
  const lowerPercentile = (100 - interval) / 2;
  const intervals = Object.fromEntries(METRICS.map((metric) => [
    metric, {lo: percentile(perMetric[metric], lowerPercentile), hi: percentile(perMetric[metric], 100 - lowerPercentile)},
  ])) as Record<Metric, Interval>;
  const upper99 = percentile(joint, 99);

  const verdicts = Object.fromEntries(METRICS.map((metric) => [
    metric, classifyMetric(intervals[metric], metric === 'p95' ? bands.tail : bands.primary),
  ])) as Record<Metric, MetricVerdict>;
  let status: Status;
  if (baseline.length < minimumSamples) {
    status = 'insufficient-samples';
  } else if (METRICS.some((metric) => verdicts[metric] === 'slower')) {
    status = 'slower';
  } else if (verdicts.mean === 'faster' || (verdicts.p50 === 'faster' && verdicts.mean === 'equivalent')) {
    status = 'faster';
  } else if (PRIMARY.some((metric) => verdicts[metric] === 'uncertain')) {
    status = 'unproven';
  } else {
    status = 'equivalent';
  }

  return {
    summary, ratios, intervals, verdicts, upper99, tolerance: bands, status, samples: baseline.length,
    throughput: {baseline: 1000 / summary.baseline.mean, candidate: 1000 / summary.candidate.mean},
  };
}

// The equivalence bands for a machine, read off A/A comparisons -- a binary
// timed against itself, where every true ratio is 1 and anything else is
// noise. Each comparison's upper bounds say how far noise alone pushed each
// statistic above 1: the body band is a high percentile of the p50 and mean
// bounds, the tail band the same percentile of the p95 bounds, so a real A/B
// run has to clear the noise the machine actually showed, not a number
// picked in advance.
//
// `floor` keeps a suspiciously quiet calibration from making the gate
// meaningless: 2% is below any real regression this suite has caught and
// well above encoder rounding.
export function calibrate(aaComparisons: Array<{intervals: Record<Metric, {hi: number}>}>, {quantile = 95, floor = 0.02} = {}): Bands {
  const margin = (result: {intervals: Record<Metric, {hi: number}>}, metric: Metric) => Math.max(0, result.intervals[metric].hi - 1);
  const primary = aaComparisons.flatMap((result) => PRIMARY.map((metric) => margin(result, metric)));
  const tail = aaComparisons.map((result) => margin(result, 'p95'));
  const band = (margins: number[]) => margins.length ? Math.max(floor, percentile(margins, quantile)) : floor;
  return {primary: band(primary), tail: band(tail)};
}
