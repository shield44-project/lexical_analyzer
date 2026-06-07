#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define HOST "127.0.0.1"
#define PORT 8000
#define LISTEN_BACKLOG 16
#define REQUEST_LIMIT (1024 * 1024)
#define IO_CHUNK 4096
#define PROMPT "Store this analyser output in a text file? (yes/no): "

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverlength-strings"
#endif
static const char *HTML_PAGE =
"<!doctype html>\n"
"<html lang=\"en\">\n"
"<head>\n"
"  <meta charset=\"utf-8\" />\n"
"  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />\n"
"  <title>Lexical and Syntax Analyser</title>\n"
"  <style>\n"
"    :root {\n"
"      color-scheme: dark;\n"
"      --bg: #08111f;\n"
"      --panel: #0f172a;\n"
"      --panel-2: #111827;\n"
"      --text: #e5e7eb;\n"
"      --muted: #94a3b8;\n"
"      --line: #243044;\n"
"      --accent: #60a5fa;\n"
"      --good: #4ade80;\n"
"      --bad: #f87171;\n"
"      --shadow: rgba(0, 0, 0, 0.25);\n"
"    }\n"
"    * { box-sizing: border-box; }\n"
"    body {\n"
"      margin: 0;\n"
"      font-family: Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, \"Segoe UI\", sans-serif;\n"
"      background:\n"
"        radial-gradient(circle at top left, rgba(96, 165, 250, 0.18), transparent 32%),\n"
"        radial-gradient(circle at top right, rgba(74, 222, 128, 0.12), transparent 28%),\n"
"        linear-gradient(180deg, #07111f, #020617 75%);\n"
"      color: var(--text);\n"
"      min-height: 100vh;\n"
"    }\n"
"    .shell { max-width: 1500px; margin: 0 auto; padding: 24px; }\n"
"    .hero {\n"
"      display: flex; gap: 18px; align-items: end; justify-content: space-between;\n"
"      margin-bottom: 18px; flex-wrap: wrap;\n"
"    }\n"
"    .hero h1 { font-size: clamp(28px, 4vw, 48px); line-height: 1; margin: 0; letter-spacing: -0.04em; }\n"
"    .hero p { margin: 10px 0 0; color: var(--muted); max-width: 70ch; }\n"
"    .badge {\n"
"      border: 1px solid rgba(96, 165, 250, 0.35); background: rgba(15, 23, 42, 0.8);\n"
"      border-radius: 999px; padding: 10px 14px; color: #cbd5e1; box-shadow: 0 16px 40px var(--shadow); white-space: nowrap;\n"
"    }\n"
"    .grid { display: grid; grid-template-columns: minmax(0, 1fr) minmax(0, 1.15fr); gap: 16px; }\n"
"    .panel {\n"
"      background: rgba(15, 23, 42, 0.92); border: 1px solid rgba(36, 48, 68, 0.9);\n"
"      border-radius: 18px; box-shadow: 0 18px 50px var(--shadow); overflow: hidden; min-height: 0;\n"
"    }\n"
"    .panel-head {\n"
"      display: flex; justify-content: space-between; align-items: center; gap: 12px;\n"
"      padding: 14px 16px; border-bottom: 1px solid rgba(36, 48, 68, 0.9); background: rgba(2, 6, 23, 0.35);\n"
"    }\n"
"    .panel-head h2 {\n"
"      font-size: 14px; text-transform: uppercase; letter-spacing: 0.12em; margin: 0; color: #dbeafe;\n"
"    }\n"
"    .controls {\n"
"      display: flex; flex-wrap: wrap; gap: 10px; align-items: center;\n"
"      padding: 14px 16px; border-bottom: 1px solid rgba(36, 48, 68, 0.75);\n"
"    }\n"
"    button, .file-label {\n"
"      appearance: none; border: 0; border-radius: 12px; padding: 11px 14px; font: inherit; font-weight: 600;\n"
"    }\n"
"    button {\n"
"      background: linear-gradient(180deg, #60a5fa, #3b82f6); color: white; cursor: pointer;\n"
"      transition: transform 0.15s ease, filter 0.15s ease;\n"
"    }\n"
"    button:hover { transform: translateY(-1px); filter: brightness(1.05); }\n"
"    button.secondary { background: #1f2937; color: #e5e7eb; border: 1px solid rgba(148, 163, 184, 0.24); }\n"
"    .file-label {\n"
"      position: relative; display: inline-flex; align-items: center; gap: 10px;\n"
"      background: #111827; color: #cbd5e1; border: 1px solid rgba(148, 163, 184, 0.18); cursor: pointer;\n"
"    }\n"
"    .file-label input { position: absolute; inset: 0; opacity: 0; cursor: pointer; }\n"
"    .status { color: var(--muted); font-size: 14px; }\n"
"    textarea {\n"
"      width: 100%; min-height: 480px; resize: vertical; border: 0; outline: 0; background: #020617; color: #e2e8f0;\n"
"      padding: 16px; font: 13px/1.5 \"JetBrains Mono\", \"SFMono-Regular\", Consolas, \"Liberation Mono\", Menlo, monospace;\n"
"      tab-size: 4;\n"
"    }\n"
"    .editor-wrap { padding: 0; }\n"
"    .results { display: grid; grid-template-rows: 1fr auto; min-height: 0; }\n"
"    .table-wrap { overflow: auto; min-height: 0; }\n"
"    table { width: 100%; border-collapse: collapse; font-size: 14px; }\n"
"    thead th {\n"
"      position: sticky; top: 0; background: #0b1220; z-index: 1; text-align: left; padding: 12px 14px;\n"
"      border-bottom: 1px solid rgba(36, 48, 68, 0.9); color: #dbeafe; font-size: 12px; text-transform: uppercase; letter-spacing: 0.1em;\n"
"    }\n"
"    tbody td { padding: 10px 14px; border-bottom: 1px solid rgba(36, 48, 68, 0.55); vertical-align: top; color: #e5e7eb; }\n"
"    tbody tr:hover { background: rgba(96, 165, 250, 0.06); }\n"
"    tbody td:nth-child(1), tbody td:nth-child(2) { color: #93c5fd; width: 72px; white-space: nowrap; }\n"
"    tbody td:nth-child(3) { color: #6ee7b7; width: 150px; white-space: nowrap; }\n"
"    .syntax {\n"
"      border-top: 1px solid rgba(36, 48, 68, 0.9); background: rgba(2, 6, 23, 0.45); padding: 14px 16px;\n"
"      max-height: 220px; overflow: auto;\n"
"    }\n"
"    .syntax h3 { margin: 0 0 8px; font-size: 12px; text-transform: uppercase; letter-spacing: 0.12em; color: #dbeafe; }\n"
"    pre { margin: 0; white-space: pre-wrap; word-break: break-word; color: #cbd5e1; font: 13px/1.6 \"JetBrains Mono\", \"SFMono-Regular\", Consolas, \"Liberation Mono\", Menlo, monospace; }\n"
"    .empty { padding: 18px 16px; color: var(--muted); }\n"
"    @media (max-width: 1100px) { .grid { grid-template-columns: 1fr; } textarea { min-height: 320px; } }\n"
"  </style>\n"
"</head>\n"
"<body>\n"
"  <div class=\"shell\">\n"
"    <div class=\"hero\">\n"
"      <div>\n"
"        <h1>Lexical and Syntax Analyser</h1>\n"
"        <p>Paste C code or load a file, then run the existing analyzer and inspect the same token data you see in the terminal.</p>\n"
"      </div>\n"
"      <div class=\"badge\">Local browser GUI backed by <code>lexical_analyser</code></div>\n"
"    </div>\n"
"    <div class=\"grid\">\n"
"      <section class=\"panel\">\n"
"        <div class=\"panel-head\">\n"
"          <h2>Source Code</h2>\n"
"          <div class=\"status\" id=\"status\">Ready</div>\n"
"        </div>\n"
"        <div class=\"controls\">\n"
"          <button id=\"analyzeBtn\">Analyze Code</button>\n"
"          <button class=\"secondary\" id=\"clearBtn\">Clear</button>\n"
"          <label class=\"file-label\">Load File\n"
"            <input id=\"fileInput\" type=\"file\" accept=\".c,.h,.txt,.cpp,.cc,.cxx,.hpp,.hh,text/plain\" />\n"
"          </label>\n"
"          <span class=\"status\" id=\"fileName\">No file loaded</span>\n"
"        </div>\n"
"        <div class=\"editor-wrap\">\n"
"          <textarea id=\"source\" spellcheck=\"false\">#include <stdio.h>\n"
"\n"
"int main(void) {\n"
"    int value = 10;\n"
"    if (value > 0) {\n"
"        printf(\"value=%d\\\\n\", value);\n"
"    }\n"
"    return 0;\n"
"}</textarea>\n"
"        </div>\n"
"      </section>\n"
"      <section class=\"panel results\">\n"
"        <div class=\"panel-head\">\n"
"          <h2>Terminal Output</h2>\n"
"          <div class=\"status\" id=\"summary\">Ready</div>\n"
"        </div>\n"
"        <div class=\"controls\">\n"
"          <button id=\"terminalBtn\">Terminal</button>\n"
"          <select id=\"tokenTypeFilter\" style=\"appearance: none; background: #1f2937; color: #e5e7eb; border: 1px solid rgba(148, 163, 184, 0.24); border-radius: 12px; padding: 10px 14px; font: inherit; font-weight: 600; cursor: pointer; outline: none;\">\n"
"            <option value=\"terminal\">All Tokens</option>\n"
"            <option value=\"KEYWORD\">Keywords</option>\n"
"            <option value=\"IDENTIFIER\">Identifiers</option>\n"
"            <option value=\"NUMBER\">Numbers</option>\n"
"            <option value=\"STRING\">Strings</option>\n"
"            <option value=\"CHAR\">Chars</option>\n"
"            <option value=\"OPERATOR\">Operators</option>\n"
"            <option value=\"SYMBOL\">Symbols</option>\n"
"            <option value=\"COMMENT\">Comments</option>\n"
"            <option value=\"PREPROCESSOR\">Preprocessor</option>\n"
"            <option value=\"UNKNOWN\">Unknown</option>\n"
"          </select>\n"
"          <button class=\"secondary\" id=\"exportBtn\">Export TXT</button>\n"
"        </div>\n"
"        <pre id=\"outputBox\">Run analysis to see terminal output here.</pre>\n"
"      </section>\n"
"    </div>\n"
"  </div>\n"
"  <script>\n"
"    const source = document.getElementById('source');\n"
"    const fileInput = document.getElementById('fileInput');\n"
"    const fileName = document.getElementById('fileName');\n"
"    const status = document.getElementById('status');\n"
"    const summary = document.getElementById('summary');\n"
"    const outputBox = document.getElementById('outputBox');\n"
"    const analyzeBtn = document.getElementById('analyzeBtn');\n"
"    const clearBtn = document.getElementById('clearBtn');\n"
"    const exportBtn = document.getElementById('exportBtn');\n"
"    const terminalBtn = document.getElementById('terminalBtn');\n"
"    const tokenTypeFilter = document.getElementById('tokenTypeFilter');\n"
"    let currentTokens = [];\n"
"    let currentSyntaxText = '';\n"
"    let currentSourceName = 'analysis';\n"
"    let currentView = 'terminal';\n"
"    fileInput.addEventListener('change', async () => {\n"
"      const file = fileInput.files && fileInput.files[0];\n"
"      if (!file) return;\n"
"      fileName.textContent = file.name;\n"
"      currentSourceName = file.name.replace(/\\.[^.]+$/, '') || 'analysis';\n"
"      source.value = await file.text();\n"
"      status.textContent = 'File loaded';\n"
"    });\n"
"    clearBtn.addEventListener('click', () => {\n"
"      source.value = '';\n"
"      currentTokens = [];\n"
"      currentSyntaxText = '';\n"
"      currentSourceName = 'analysis';\n"
"      currentView = 'terminal';\n"
"      tokenTypeFilter.value = 'terminal';\n"
"      outputBox.textContent = 'Run analysis to see terminal output here.';\n"
"      summary.textContent = 'Ready';\n"
"      status.textContent = 'Cleared';\n"
"      fileName.textContent = 'No file loaded';\n"
"      fileInput.value = '';\n"
"    });\n"
"    function viewLabel() {\n"
"      if (currentView === 'terminal') return 'Terminal';\n"
"      return currentView.charAt(0) + currentView.slice(1).toLowerCase() + 's';\n"
"    }\n"
"    function filterTokens(tokens) {\n"
"      if (currentView === 'terminal') {\n"
"        return tokens;\n"
"      }\n"
"      return tokens.filter((token) => token.type === currentView);\n"
"    }\n"
"    function buildReportText() {\n"
"      const tokens = filterTokens(currentTokens);\n"
"      const lines = [];\n"
"      lines.push('Lexical and Syntax Analyser');\n"
"      lines.push('');\n"
"      lines.push(`Token view: ${currentView === 'terminal' ? 'ALL TOKENS' : currentView}`);\n"
"      lines.push('LINE  COL   TOKEN          VALUE');\n"
"      lines.push('--------------------------------------------------');\n"
"      if (!tokens.length) {\n"
"        lines.push('No tokens available.');\n"
"      } else {\n"
"        for (const token of tokens) {\n"
"          const line = String(token.line).padEnd(5, ' ');\n"
"          const col = String(token.col).padEnd(5, ' ');\n"
"          const type = String(token.type).padEnd(14, ' ');\n"
"          lines.push(`${line} ${col} ${type} ${token.value}`);\n"
"        }\n"
"      }\n"
"      lines.push('');\n"
"      lines.push('Syntax Analysis');\n"
"      lines.push('---------------');\n"
"      lines.push(currentSyntaxText || 'No syntax output available.');\n"
"      lines.push('');\n"
"      lines.push(`Displayed tokens: ${tokens.length}`);\n"
"      lines.push(`Total tokens: ${currentTokens.length}`);\n"
"      return lines.join('\\n');\n"
"    }\n"
"    function renderOutput() {\n"
"      outputBox.textContent = buildReportText();\n"
"      summary.textContent = `${viewLabel()} view`;\n"
"    }\n"
"    function setView(view) {\n"
"      currentView = view;\n"
"      tokenTypeFilter.value = view;\n"
"      renderOutput();\n"
"    }\n"
"    exportBtn.addEventListener('click', () => {\n"
"      if (!currentTokens.length) {\n"
"        status.textContent = 'Run analysis before exporting';\n"
"        return;\n"
"      }\n"
"      const blob = new Blob([buildReportText()], { type: 'text/plain;charset=utf-8' });\n"
"      const url = URL.createObjectURL(blob);\n"
"      const link = document.createElement('a');\n"
"      link.href = url;\n"
"      link.download = `${currentSourceName}_run.txt`;\n"
"      document.body.appendChild(link);\n"
"      link.click();\n"
"      link.remove();\n"
"      URL.revokeObjectURL(url);\n"
"      status.textContent = 'Exported TXT';\n"
"    });\n"
"    terminalBtn.addEventListener('click', () => setView('terminal'));\n"
"    tokenTypeFilter.addEventListener('change', (e) => setView(e.target.value));\n"
"    analyzeBtn.addEventListener('click', async () => {\n"
"      const code = source.value;\n"
"      if (!code.trim()) { status.textContent = 'Paste or load code first'; return; }\n"
"      status.textContent = 'Analyzing...';\n"
"      analyzeBtn.disabled = true;\n"
"      try {\n"
"        const response = await fetch('/analyze', {\n"
"          method: 'POST',\n"
"          headers: { 'Content-Type': 'text/plain; charset=utf-8' },\n"
"          body: code\n"
"        });\n"
"        const payload = await response.json();\n"
"        if (!response.ok) throw new Error(payload.error || 'Analysis failed');\n"
"        currentTokens = payload.tokens || [];\n"
"        currentSyntaxText = payload.syntax || '';\n"
"        renderOutput();\n"
"        status.textContent = 'Done';\n"
"      } catch (error) {\n"
"        currentTokens = [];\n"
"        currentSyntaxText = '';\n"
"        status.textContent = 'Error';\n"
"        outputBox.textContent = String(error.message || error);\n"
"        summary.textContent = 'Error';\n"
"      } finally {\n"
"        analyzeBtn.disabled = false;\n"
"      }\n"
"    });\n"
"  </script>\n"
"</body>\n"
"</html>\n";
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} Buffer;

typedef struct {
    int line;
    int col;
    char type[32];
    char value[256];
} GuiToken;

typedef struct {
    GuiToken *items;
    size_t len;
    size_t cap;
} TokenList;

static void buffer_init(Buffer *b) {
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static void buffer_free(Buffer *b) {
    free(b->data);
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static int buffer_reserve(Buffer *b, size_t extra) {
    size_t need = b->len + extra + 1;
    if (need <= b->cap) return 1;
    size_t new_cap = b->cap ? b->cap : 1024;
    while (new_cap < need) new_cap *= 2;
    char *new_data = realloc(b->data, new_cap);
    if (!new_data) return 0;
    b->data = new_data;
    b->cap = new_cap;
    return 1;
}

static int buffer_append(Buffer *b, const char *s, size_t n) {
    if (!buffer_reserve(b, n)) return 0;
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
    return 1;
}

static int buffer_append_str(Buffer *b, const char *s) {
    return buffer_append(b, s, strlen(s));
}

static int buffer_append_char(Buffer *b, char c) {
    return buffer_append(b, &c, 1);
}

static int buffer_append_fmt(Buffer *b, const char *fmt, ...) {
    va_list ap;
    va_list copy;
    int needed;
    char stack[256];
    char *tmp = stack;
    size_t size = sizeof(stack);

    va_start(ap, fmt);
    va_copy(copy, ap);
    needed = vsnprintf(stack, sizeof(stack), fmt, ap);
    va_end(ap);
    if (needed < 0) {
        va_end(copy);
        return 0;
    }
    if ((size_t)needed >= sizeof(stack)) {
        size = (size_t)needed + 1;
        tmp = malloc(size);
        if (!tmp) {
            va_end(copy);
            return 0;
        }
        if (vsnprintf(tmp, size, fmt, copy) < 0) {
            free(tmp);
            va_end(copy);
            return 0;
        }
    }
    va_end(copy);
    {
        int ok = buffer_append(b, tmp, strlen(tmp));
        if (tmp != stack) free(tmp);
        return ok;
    }
}

static void token_list_free(TokenList *list) {
    free(list->items);
    list->items = NULL;
    list->len = 0;
    list->cap = 0;
}

static int token_list_push(TokenList *list, int line, int col, const char *type, const char *value) {
    if (list->len == list->cap) {
        size_t new_cap = list->cap ? list->cap * 2 : 128;
        GuiToken *new_items = realloc(list->items, new_cap * sizeof(*new_items));
        if (!new_items) return 0;
        list->items = new_items;
        list->cap = new_cap;
    }
    list->items[list->len].line = line;
    list->items[list->len].col = col;
    snprintf(list->items[list->len].type, sizeof(list->items[list->len].type), "%s", type);
    snprintf(list->items[list->len].value, sizeof(list->items[list->len].value), "%s", value);
    list->len++;
    return 1;
}

static int send_all(int fd, const char *buf, size_t len) {
    while (len > 0) {
        ssize_t sent = send(fd, buf, len, 0);
        if (sent < 0) {
            if (errno == EINTR) continue;
            return 0;
        }
        buf += (size_t)sent;
        len -= (size_t)sent;
    }
    return 1;
}

static int send_response(int fd, int status, const char *ctype, const char *body, size_t body_len) {
    const char *status_text = status == 200 ? "OK" :
                              status == 400 ? "Bad Request" :
                              status == 404 ? "Not Found" :
                              status == 500 ? "Internal Server Error" : "OK";
    char header[512];
    int header_len = snprintf(header, sizeof(header),
                              "HTTP/1.1 %d %s\r\n"
                              "Content-Type: %s\r\n"
                              "Content-Length: %zu\r\n"
                              "Cache-Control: no-store\r\n"
                              "Connection: close\r\n\r\n",
                              status, status_text, ctype, body_len);
    if (header_len < 0 || (size_t)header_len >= sizeof(header)) return 0;
    return send_all(fd, header, (size_t)header_len) && send_all(fd, body, body_len);
}

static char *strip_prompt(const char *input) {
    const char *pos = strstr(input, PROMPT);
    if (!pos) return strdup(input);
    size_t prefix = (size_t)(pos - input);
    size_t suffix = strlen(pos + strlen(PROMPT));
    char *out = malloc(prefix + 2 + suffix + 1);
    if (!out) return NULL;
    memcpy(out, input, prefix);
    out[prefix] = '\n';
    memcpy(out + prefix + 1, pos + strlen(PROMPT), suffix);
    out[prefix + 1 + suffix] = '\0';
    return out;
}

static char *strip_ansi(const char *input) {
    size_t len = strlen(input);
    char *out = malloc(len + 1);
    size_t j = 0;
    if (!out) return NULL;
    for (size_t i = 0; i < len; i++) {
        if (input[i] == '\033' && input[i + 1] == '[') {
            i += 2;
            while (input[i] && ((input[i] >= '0' && input[i] <= '9') || input[i] == ';')) i++;
            if (input[i]) continue;
            break;
        }
        out[j++] = input[i];
    }
    out[j] = '\0';
    return out;
}

static char *read_fd_all(int fd) {
    Buffer b;
    char chunk[IO_CHUNK];
    ssize_t n;

    buffer_init(&b);
    while ((n = read(fd, chunk, sizeof(chunk))) > 0) {
        if (!buffer_append(&b, chunk, (size_t)n)) {
            buffer_free(&b);
            return NULL;
        }
    }
    if (n < 0) {
        buffer_free(&b);
        return NULL;
    }
    return b.data ? b.data : strdup("");
}

static char *run_analyzer(const char *code, size_t code_len) {
    char tmp_path[] = "/tmp/lexical_gui_XXXXXX";
    int tmp_fd = mkstemp(tmp_path);
    int pipefd[2];
    pid_t pid;
    char *raw = NULL;
    char *clean = NULL;
    int status;

    if (tmp_fd < 0) return NULL;
    if (write(tmp_fd, code, code_len) < 0) {
        close(tmp_fd);
        unlink(tmp_path);
        return NULL;
    }
    close(tmp_fd);

    if (pipe(pipefd) != 0) {
        unlink(tmp_path);
        return NULL;
    }

    pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        unlink(tmp_path);
        return NULL;
    }
    if (pid == 0) {
        int devnull = open("/dev/null", O_RDONLY);
        if (devnull >= 0) {
            dup2(devnull, STDIN_FILENO);
            close(devnull);
        }
        setenv("ANALYZER_NONINTERACTIVE", "1", 1);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execl("./lexical_analyser", "lexical_analyser", tmp_path, (char *)NULL);
        _exit(127);
    }

    close(pipefd[1]);
    raw = read_fd_all(pipefd[0]);
    close(pipefd[0]);
    waitpid(pid, &status, 0);
    unlink(tmp_path);
    if (!raw) return NULL;

    clean = strip_ansi(raw);
    free(raw);
    if (!clean) return NULL;

    {
        char *normalized = strip_prompt(clean);
        free(clean);
        return normalized;
    }
}

static char *json_escape(const char *s) {
    Buffer b;
    buffer_init(&b);
    if (!buffer_append_char(&b, '"')) {
        buffer_free(&b);
        return NULL;
    }
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        switch (*p) {
            case '\\':
                if (!buffer_append_str(&b, "\\\\")) goto fail;
                break;
            case '\"':
                if (!buffer_append_str(&b, "\\\"")) goto fail;
                break;
            case '\b':
                if (!buffer_append_str(&b, "\\b")) goto fail;
                break;
            case '\f':
                if (!buffer_append_str(&b, "\\f")) goto fail;
                break;
            case '\n':
                if (!buffer_append_str(&b, "\\n")) goto fail;
                break;
            case '\r':
                if (!buffer_append_str(&b, "\\r")) goto fail;
                break;
            case '\t':
                if (!buffer_append_str(&b, "\\t")) goto fail;
                break;
            default:
                if (*p < 0x20) {
                    if (!buffer_append_fmt(&b, "\\u%04x", *p)) goto fail;
                } else {
                    if (!buffer_append_char(&b, (char)*p)) goto fail;
                }
                break;
        }
    }
    if (!buffer_append_char(&b, '"')) goto fail;
    return b.data;
fail:
    buffer_free(&b);
    return NULL;
}

static int parse_tokens(const char *output, TokenList *tokens, char **syntax_out) {
    const char *p = output;
    int in_tokens = 0;
    const char *syntax_start = NULL;
    Buffer syntax;

    buffer_init(&syntax);
    while (*p) {
        const char *line_start = p;
        const char *line_end = strchr(p, '\n');
        size_t line_len = line_end ? (size_t)(line_end - p) : strlen(p);
        char line[512];
        size_t copy = line_len < sizeof(line) - 1 ? line_len : sizeof(line) - 1;

        memcpy(line, line_start, copy);
        line[copy] = '\0';

        if (strncmp(line, "LINE", 4) == 0 && strstr(line, "TOKEN") && strstr(line, "VALUE")) {
            in_tokens = 1;
        } else if (strncmp(line, "Syntax Analysis", 15) == 0) {
            in_tokens = 0;
            syntax_start = line_start;
            break;
        } else if (in_tokens) {
            int line_no, col;
            char type[32];
            char value[256];
            if (sscanf(line, "%d %d %31s %255[^\n]", &line_no, &col, type, value) == 4) {
                if (!token_list_push(tokens, line_no, col, type, value)) {
                    buffer_free(&syntax);
                    return 0;
                }
            }
        }

        if (!line_end) break;
        p = line_end + 1;
    }

    if (syntax_start) {
        if (!buffer_append_str(&syntax, syntax_start)) {
            buffer_free(&syntax);
            return 0;
        }
    } else {
        if (!buffer_append_str(&syntax, output)) {
            buffer_free(&syntax);
            return 0;
        }
    }

    *syntax_out = syntax.data ? syntax.data : strdup("");
    if (!*syntax_out) {
        buffer_free(&syntax);
        return 0;
    }
    return 1;
}

static char *build_json_response(const TokenList *tokens, const char *syntax) {
    Buffer b;
    buffer_init(&b);
    if (!buffer_append_str(&b, "{\"tokens\":[")) goto fail;
    for (size_t i = 0; i < tokens->len; i++) {
        char *value = json_escape(tokens->items[i].value);
        if (!value) goto fail;
        if (i && !buffer_append_char(&b, ',')) {
            free(value);
            goto fail;
        }
        if (!buffer_append_fmt(&b,
                               "{\"line\":%d,\"col\":%d,\"type\":\"%s\",\"value\":%s}",
                               tokens->items[i].line,
                               tokens->items[i].col,
                               tokens->items[i].type,
                               value)) {
            free(value);
            goto fail;
        }
        free(value);
    }
    if (!buffer_append_str(&b, "],\"syntax\":")) goto fail;
    {
        char *escaped = json_escape(syntax ? syntax : "");
        if (!escaped) goto fail;
        if (!buffer_append_str(&b, escaped)) {
            free(escaped);
            goto fail;
        }
        free(escaped);
    }
    if (!buffer_append_str(&b, "}")) goto fail;
    return b.data;
fail:
    buffer_free(&b);
    return NULL;
}

static int handle_analyze(int client, const char *body, size_t body_len) {
    char *raw = run_analyzer(body, body_len);
    TokenList tokens = {0};
    char *syntax = NULL;
    char *json = NULL;
    int ok;

    if (!raw) {
        const char *msg = "{\"error\":\"failed to run analyzer\"}";
        return send_response(client, 500, "application/json; charset=utf-8", msg, strlen(msg));
    }

    ok = parse_tokens(raw, &tokens, &syntax);
    free(raw);
    if (!ok) {
        token_list_free(&tokens);
        const char *msg = "{\"error\":\"failed to parse analyzer output\"}";
        return send_response(client, 500, "application/json; charset=utf-8", msg, strlen(msg));
    }

    json = build_json_response(&tokens, syntax);
    token_list_free(&tokens);
    free(syntax);
    if (!json) {
        const char *msg = "{\"error\":\"failed to build response\"}";
        return send_response(client, 500, "application/json; charset=utf-8", msg, strlen(msg));
    }

    ok = send_response(client, 200, "application/json; charset=utf-8", json, strlen(json));
    free(json);
    return ok;
}

static char *find_header_value(const char *headers, const char *name) {
    size_t name_len = strlen(name);
    const char *p = headers;
    while ((p = strstr(p, name)) != NULL) {
        if ((p == headers || p[-1] == '\n' || p[-1] == '\r') && p[name_len] == ':') {
            p += name_len + 1;
            while (*p == ' ' || *p == '\t') p++;
            return (char *)p;
        }
        p += name_len;
    }
    return NULL;
}

static int handle_client(int client) {
    char *request = malloc(REQUEST_LIMIT + 1);
    size_t used = 0;
    ssize_t n;
    char *header_end;
    size_t headers_len;
    char *method;
    char *path;
    char *version;
    char *content_length_value;
    long content_length = 0;
    size_t body_offset;
    size_t body_len;

    if (!request) return 0;
    while ((n = recv(client, request + used, REQUEST_LIMIT - used, 0)) > 0) {
        used += (size_t)n;
        request[used] = '\0';
        header_end = strstr(request, "\r\n\r\n");
        if (header_end) break;
        if (used >= REQUEST_LIMIT) break;
    }
    if (n < 0 || !strstr(request, "\r\n\r\n")) {
        free(request);
        return 0;
    }

    header_end = strstr(request, "\r\n\r\n");
    headers_len = (size_t)(header_end - request);
    request[headers_len] = '\0';

    method = request;
    path = strchr(method, ' ');
    if (!path) {
        free(request);
        return 0;
    }
    *path++ = '\0';
    version = strchr(path, ' ');
    if (!version) {
        free(request);
        return 0;
    }
    *version++ = '\0';

    content_length_value = find_header_value(version, "Content-Length");
    if (content_length_value) {
        content_length = strtol(content_length_value, NULL, 10);
        if (content_length < 0) content_length = 0;
    }

    body_offset = headers_len + 4;
    body_len = used > body_offset ? used - body_offset : 0;
    while (body_len < (size_t)content_length) {
        n = recv(client, request + used, REQUEST_LIMIT - used, 0);
        if (n <= 0) {
            free(request);
            return 0;
        }
        used += (size_t)n;
        request[used] = '\0';
        body_len = used > body_offset ? used - body_offset : 0;
    }

    if (strcmp(method, "GET") == 0 && (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0)) {
        int ok = send_response(client, 200, "text/html; charset=utf-8", HTML_PAGE, strlen(HTML_PAGE));
        free(request);
        return ok;
    }

    if (strcmp(method, "POST") == 0 && strcmp(path, "/analyze") == 0) {
        const char *body = request + body_offset;
        if ((size_t)content_length > body_len) content_length = (long)body_len;
        {
            int ok = handle_analyze(client, body, (size_t)content_length);
            free(request);
            return ok;
        }
    }

    {
        const char *msg = "Not found";
        int ok = send_response(client, 404, "text/plain; charset=utf-8", msg, strlen(msg));
        free(request);
        return ok;
    }
}

static void open_browser(const char *url) {
    pid_t pid = fork();
    if (pid != 0) return;
    execlp("xdg-open", "xdg-open", url, (char *)NULL);
    _exit(0);
}

int main(void) {
    int server_fd;
    struct sockaddr_in addr;
    int opt = 1;
    char url[64];

    signal(SIGPIPE, SIG_IGN);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, HOST, &addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid host address\n");
        close(server_fd);
        return 1;
    }

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, LISTEN_BACKLOG) != 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    snprintf(url, sizeof(url), "http://%s:%d", HOST, PORT);
    printf("GUI available at %s\n", url);
    open_browser(url);

    for (;;) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            break;
        }
        handle_client(client);
        close(client);
    }

    close(server_fd);
    return 0;
}
