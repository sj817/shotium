import {SHARD_SCENARIOS} from './constants.ts';
import {balancedOrder} from './statistics.ts';

export function buildGroups(engines, profile, {shard = 'all', seed = 'local'} = {}) {
  const groups: any[] = [];
  let rotation = 0;
  const selectedScenarios = shard === 'all' ? null : new Set(SHARD_SCENARIOS[shard]);
  const add = (scenario, repeat, concurrency, iterations = 1, timeoutMs = undefined,
      selectedEngines = engines) => {
    if (selectedScenarios && !selectedScenarios.has(scenario)) return;
    groups.push({
      scenario, repeat, concurrency, iterations, timeoutMs,
      engines: balancedOrder(selectedEngines, rotation++, `${seed}:${scenario}`),
    });
  };
  const chunks = (total, count) => Array.from({length: count}, (_, index) =>
    Math.floor(total / count) + (index < total % count ? 1 : 0));
  for (const scenario of ['cold', 'cold-settled']) {
    for (let repeat = 1; repeat <= profile.repeats; repeat += 1) {
      add(scenario, repeat, 1);
    }
  }
  chunks(profile.warmIterations, profile.warmChunks)
      .forEach((iterations, index) => add('warm', index + 1, 1, iterations));
  add('reuse-page', 1, 1, profile.warmIterations, undefined,
      engines.filter((engine) => engine !== 'shotium'));
  // Batch and parallel are steady-state request measurements. Run every round
  // against one settled engine instance so launch/readiness costs stay in the
  // startup scenarios and do not multiply the CI wall clock. The worker still
  // records every case in every round as an individual latency sample.
  add('batch', 1, 1, profile.batchRounds);
  // Resident measures new clients against one already-warm host. Keep the same
  // number of measured clients, but do not pay host launch plus 3-10 settling
  // clients again for every sample.
  add('resident', 1, 1, profile.repeats);
  chunks(profile.lifecycleCycles, profile.lifecycleChunks)
      .forEach((iterations, index) => add('lifecycle', index + 1, 1, iterations));
  add('faults', 1, 1);
  for (const concurrency of profile.concurrencies) {
    add('parallel', 1, concurrency, profile.batchRounds);
  }
  add('soak', 1, 4, profile.soakIterations, profile.soakTimeoutMs);
  return groups;
}
