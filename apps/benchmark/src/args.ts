export type ParsedArgs = Record<string, any>;

export function parseArgs(argv: string[], defaults: ParsedArgs = {}): ParsedArgs {
  const result: ParsedArgs = {...defaults};
  const positionals: string[] = [];
  for (let index = 0; index < argv.length; index += 1) {
    const item = argv[index];
    if (!item.startsWith('--')) {
      positionals.push(item);
      continue;
    }
    const [rawKey, inline] = item.slice(2).split('=', 2);
    const key = rawKey.replace(/-([a-z])/g, (_, letter) => letter.toUpperCase());
    if (inline !== undefined) {
      result[key] = inline;
      continue;
    }
    if (argv[index + 1] && !argv[index + 1].startsWith('--')) {
      result[key] = argv[index + 1];
      index += 1;
    } else {
      result[key] = true;
    }
  }
  if (positionals.length) result._ = positionals;
  return result;
}

// npm 11 on Windows treats option names ending in "version" as npm options
// even after `npm run ... --`: it leaves the flag in place and moves its
// separated value to the end of argv. Preserve the documented CLI form while
// still rejecting every genuinely stray positional argument.
export function recoverNpmRunValues(args: ParsedArgs, keys: string[]): void {
  const positionals = Array.isArray(args._) ? [...args._] : [];
  for (const key of keys) {
    const npmKey = `npm_config_${key.replace(/[A-Z]/g, (letter) => `_${letter.toLowerCase()}`)}`;
    const npmValue = process.env[npmKey];
    if (npmValue && npmValue !== 'true') args[key] = npmValue;
    else if (npmValue === 'true') args[key] = positionals.length ? positionals.shift() : true;
    else if (args[key] === true && positionals.length) args[key] = positionals.shift();
  }
  if (positionals.length) throw new Error(`unexpected argument ${positionals[0]}`);
  delete args._;
}

export function booleanArg(value: unknown, fallback = false): boolean {
  if (value === undefined) return fallback;
  if (typeof value === 'boolean') return value;
  if (value === 'true' || value === '1') return true;
  if (value === 'false' || value === '0') return false;
  throw new Error(`expected boolean, got ${value}`);
}
