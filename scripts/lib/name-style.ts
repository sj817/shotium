// Blink's NameStyleConverter, as
// third_party/blink/renderer/build/scripts/blinkbuild/name_style_converter.py
// defines it: the tokenizer that decides "USVString" is USV + String and
// "unsigned long" is Unsigned + Long, and the case conversions built on it.
//
// The IDL generators depend on this being exactly Blink's rule -- the call
// sites in renderer/core were written against upstream's generator, which
// used this tokenizer -- so the special-token list is copied verbatim and the
// regexes are the same ones. `pnpm gen:idl --check` is what proves the copy
// is faithful.

const SPECIAL_TOKENS = [
  // This list should be sorted by length.
  'WebCodecs', 'WebSocket', 'String16', 'Float32', 'Float64', 'Base64', 'IFrame', 'Latin1', 'MathML', 'PlugIn', 'SQLite',
  'Uint16', 'Uint32', 'WebGL2', 'webgl2', 'WebGPU', 'ASCII', 'CSSOM', 'CType', 'DList', 'Int16', 'Int32', 'MPath', 'OList',
  'TSpan', 'UList', 'UTF16', 'Uint8', 'WebGL', 'XPath', 'ETC1', 'etc1', 'HTML', 'Int8', 'S3TC', 's3tc', 'SPv2', 'UTF8', 'sRGB',
  'URLs', 'API', 'CSS', 'DNS', 'DOM', 'EXT', 'RTC', 'SVG', 'XSS', '2D', 'AX', 'FE', 'JS', 'V0', 'V8', 'v8', 'XR',
];
const SPECIAL_TOKENS_WITH_NUMBERS = SPECIAL_TOKENS.filter((t) => /[0-9]/.test(t));

// Applying these repeatedly captures any sequence of a-z, A-Z, 0-9.
const TOKEN_PATTERNS = [
  '[A-Z]?[a-z]+',       // 'Foo' 'foo'
  '[A-Z]+(?![a-z])',    // only 'FOO' in 'FOOElement'
  '[0-9][Dd](?![a-z])', // '2D' '3D', but not '2Dimension'
  '[0-9]+',
];
const TOKEN_RE = new RegExp(`(${[...SPECIAL_TOKENS, ...TOKEN_PATTERNS].join('|')})`, 'g');
const LEADING_NUMBER_TOKEN = new RegExp(`^(${SPECIAL_TOKENS_WITH_NUMBERS.join('|')})`, 'i');

// A token consists of A-Z, a-z and 0-9; other characters delimit, and so do
// capital letters: 'FooBar-baz' -> ['Foo', 'Bar', 'baz']. Special cases that
// need knowledge -- in SVGSVGElement the first two SVGs are separate tokens,
// but WebGL is one -- come from the list above.
export function tokenizeName(name: string): string[] {
  // In case |name| is written in lowerCamelCase, match special tokens that
  // contain numbers ignoring case at the first step only.
  const tokens: string[] = [];
  const m = LEADING_NUMBER_TOKEN.exec(name);
  if (m) {
    tokens.push(m[0]);
    name = name.slice(m[0].length);
  }
  return [...tokens, ...(name.match(TOKEN_RE) ?? [])];
}

export class NameStyleConverter {
  readonly tokens: string[];
  readonly original: string;

  constructor(name: string) {
    this.tokens = tokenizeName(name);
    this.original = name;
  }

  toSnakeCase(): string {
    return this.tokens.map((t) => t.toLowerCase()).join('_');
  }

  toUpperCamelCase(): string {
    let tokens = this.tokens;
    // If the first token is one of SPECIAL_TOKENS in lower case, replace it
    // with the matched special token: ['css', 'External'] -> 'CSSExternal'.
    if (tokens.length && tokens[0].toLowerCase() === tokens[0]) {
      for (const special of SPECIAL_TOKENS) {
        if (special.toLowerCase() === tokens[0]) {
          tokens = [special, ...tokens.slice(1)];
          break;
        }
      }
    }
    return tokens.map((t) => t[0].toUpperCase() + t.slice(1)).join('');
  }

  toLowerCamelCase(): string {
    if (!this.tokens.length) return '';
    return this.tokens[0].toLowerCase() + this.tokens.slice(1).map((t) => t[0].toUpperCase() + t.slice(1)).join('');
  }

  toMacroCase(): string {
    return this.tokens.map((t) => t.toUpperCase()).join('_');
  }
}
