/**
 * Lux language syntax highlighter for the code editor.
 * Produces HTML with <span class="tok-*"> tokens for coloring.
 */

const LUX_KEYWORDS = new Set([
  "effect", "sprite", "text", "layer", "use", "color", "bitmap",
  "visible", "x", "y", "scale", "rotation", "blend",
]);

const LUX_FUNCTIONS = new Set([
  "sin", "cos", "abs", "min", "max", "clamp", "fmod", "step", "smoothstep",
  "lerp", "rgb", "hsv", "temp", "humidity", "random",
]);

const BLEND_MODES = new Set([
  "normal", "add", "multiply", "screen",
]);

function escapeHtml(text: string): string {
  return text
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;");
}

function highlightLine(line: string, inBitmap: boolean): { html: string; bitmap: boolean } {
  // Inside bitmap block: everything is bitmap-colored until closing """
  if (inBitmap) {
    if (line.trimEnd() === '"""') {
      return { html: `<span class="tok-string">${escapeHtml(line)}</span>`, bitmap: false };
    }
    return { html: `<span class="tok-bitmap">${escapeHtml(line)}</span>`, bitmap: true };
  }

  // Check for comment line
  const commentMatch = line.match(/^(\s*)(\/\/.*)/);
  if (commentMatch) {
    return {
      html: escapeHtml(commentMatch[1]) + `<span class="tok-comment">${escapeHtml(commentMatch[2])}</span>`,
      bitmap: false,
    };
  }

  // Check for opening """ (bitmap or text string)
  if (line.trimEnd() === '"""' || line.trim().endsWith('"""')) {
    const idx = line.indexOf('"""');
    const before = line.slice(0, idx);
    const tripleQuote = '"""';
    // Check if there's text before the """
    const highlightedBefore = tokenizeLine(before);
    return {
      html: highlightedBefore + `<span class="tok-string">${escapeHtml(tripleQuote)}</span>`,
      bitmap: true,
    };
  }

  return { html: tokenizeLine(line), bitmap: false };
}

function tokenizeLine(line: string): string {
  let result = "";
  let i = 0;

  while (i < line.length) {
    // Whitespace
    if (line[i] === " " || line[i] === "\t") {
      let j = i;
      while (j < line.length && (line[j] === " " || line[j] === "\t")) j++;
      result += line.slice(i, j);
      i = j;
      continue;
    }

    // String literal "..."
    if (line[i] === '"') {
      let j = i + 1;
      while (j < line.length && line[j] !== '"') {
        if (line[j] === "\\") j++;
        j++;
      }
      if (j < line.length) j++; // include closing quote
      result += `<span class="tok-string">${escapeHtml(line.slice(i, j))}</span>`;
      i = j;
      continue;
    }

    // Inline comment
    if (line[i] === "/" && line[i + 1] === "/") {
      result += `<span class="tok-comment">${escapeHtml(line.slice(i))}</span>`;
      break;
    }

    // Number
    if (/[0-9]/.test(line[i]) || (line[i] === "-" && i + 1 < line.length && /[0-9]/.test(line[i + 1]))) {
      let j = i;
      if (line[j] === "-") j++;
      while (j < line.length && /[0-9.]/.test(line[j])) j++;
      result += `<span class="tok-number">${escapeHtml(line.slice(i, j))}</span>`;
      i = j;
      continue;
    }

    // Identifier / keyword
    if (/[a-zA-Z_]/.test(line[i])) {
      let j = i;
      while (j < line.length && /[a-zA-Z0-9_]/.test(line[j])) j++;
      const word = line.slice(i, j);

      if (LUX_KEYWORDS.has(word)) {
        result += `<span class="tok-keyword">${escapeHtml(word)}</span>`;
      } else if (LUX_FUNCTIONS.has(word)) {
        result += `<span class="tok-function">${escapeHtml(word)}</span>`;
      } else if (BLEND_MODES.has(word)) {
        result += `<span class="tok-string">${escapeHtml(word)}</span>`;
      } else {
        result += `<span class="tok-name">${escapeHtml(word)}</span>`;
      }
      i = j;
      continue;
    }

    // Operators / punctuation
    if ("=+-*/%(){},".includes(line[i])) {
      result += `<span class="tok-operator">${escapeHtml(line[i])}</span>`;
      i++;
      continue;
    }

    // Anything else
    result += escapeHtml(line[i]);
    i++;
  }

  return result;
}

/**
 * Highlight full Lux source code.
 * Returns HTML string with colored spans.
 * `activeLine` is 0-based index of the line to highlight as active.
 */
export function highlightLux(source: string, activeLine: number): string {
  const lines = source.split("\n");
  let inBitmap = false;
  const htmlLines: string[] = [];

  for (let i = 0; i < lines.length; i++) {
    const { html, bitmap } = highlightLine(lines[i], inBitmap);
    inBitmap = bitmap;

    const lineClass = i === activeLine ? "editor-line--active" : "";
    // Each line is a <span> block so the active line background works
    htmlLines.push(`<span class="${lineClass}">${html || " "}\n</span>`);
  }

  return htmlLines.join("");
}
