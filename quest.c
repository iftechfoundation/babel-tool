/* quest.c  Treaty of Babel module for Quest files
 *
 * Quest 5+ packages are ZIP archives (".quest" extension) whose directory
 * contains game.aslx plus resources. Newly published packages also brand
 * UUID://…// in the ZIP comment and embed metadata.iFiction. Legacy
 * packages expose bibliographic data (including <gameid>) inside the
 * deflated game.aslx XML. Raw .aslx files are also claimed.
 *
 * Older Quest 1–4 story files use ".cas" (compiled) or ".asl" (text).
 * Bibliographic data for both is taken from the define game block; .cas
 * files are decompiled first. IFIDs for these legacy forms are
 * "QUEST-" followed by the whole-file MD5.
 *
 * This file depends on treaty_builder.h and tinfl.h
 *
 * This file is public domain, but note that any changes to this file
 * may render it noncompliant with the Treaty of Babel
 */

#define FORMAT quest
#define HOME_PAGE "https://textadventures.co.uk/quest"
#define FORMAT_EXT ".quest,.aslx,.cas,.asl"

#include "treaty_builder.h"
#include "ifiction.h"
#include "md5.h"
#include "tinfl.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * Minimal ZIP reader (central directory) + tinfl raw inflate.
 * ---------------------------------------------------------------------- */

static uint32 rd16(const unsigned char *p)
{
    return (uint32)p[0] | ((uint32)p[1] << 8);
}

static uint32 rd32(const unsigned char *p)
{
    return (uint32)p[0] | ((uint32)p[1] << 8) | ((uint32)p[2] << 16) |
           ((uint32)p[3] << 24);
}

static bool zip_find_entry(const unsigned char *zip, int32 extent,
                           const char *name, int32 *off, int32 *comp,
                           int32 *raw, int *method, uint32 *dos_time,
                           uint32 *dos_date)
{
    const unsigned char *eocd = NULL;
    int32 maxback, i;
    int count;
    uint32 cd_off;
    const unsigned char *p;
    size_t namelen;
    const unsigned char *ci_p = NULL;
    int ci_method = 0;
    int32 ci_comp = 0, ci_raw = 0;
    uint32 ci_local = 0;
    uint32 ci_dos_time = 0, ci_dos_date = 0;
    int e;

    if (extent < 22 || memcmp(zip, "PK\003\004", 4) != 0)
        return false;

    maxback = extent < 65557 ? extent : 65557;
    for (i = extent - 22; i >= 0 && i + maxback + 22 >= extent; --i) {
        if (zip[i] == 'P' && zip[i + 1] == 'K' && zip[i + 2] == 5 &&
            zip[i + 3] == 6) {
            eocd = zip + i;
            break;
        }
    }
    if (!eocd)
        return false;

    count = (int)rd16(eocd + 10);
    cd_off = rd32(eocd + 16);
    if (cd_off >= (uint32)extent)
        return false;
    p = zip + cd_off;
    namelen = strlen(name);

    for (e = 0; e < count && p + 46 <= zip + extent; ++e) {
        int m, nl, el, cl;
        int32 c, r;
        uint32 local;
        uint32 entry_time, entry_date;
        const unsigned char *ename;
        bool exact, ci;
        size_t k;

        if (!(p[0] == 'P' && p[1] == 'K' && p[2] == 1 && p[3] == 2))
            break;
        m = (int)rd16(p + 10);
        entry_time = rd16(p + 12);
        entry_date = rd16(p + 14);
        c = (int32)rd32(p + 20);
        r = (int32)rd32(p + 24);
        nl = (int)rd16(p + 28);
        el = (int)rd16(p + 30);
        cl = (int)rd16(p + 32);
        local = rd32(p + 42);
        ename = p + 46;
        if (ename + nl > zip + extent)
            break;

        exact = ((size_t)nl == namelen && memcmp(ename, name, namelen) == 0);
        ci = false;
        if (!exact && (size_t)nl == namelen) {
            ci = true;
            for (k = 0; k < namelen; k++) {
                if (tolower(ename[k]) != tolower((unsigned char)name[k])) {
                    ci = false;
                    break;
                }
            }
        }

        if (exact || ci) {
            const unsigned char *lh = zip + local;
            if (lh + 30 <= zip + extent && lh[0] == 'P' && lh[1] == 'K' &&
                lh[2] == 3 && lh[3] == 4) {
                int32 payload = (int32)(lh - zip) + 30 + (int32)rd16(lh + 26) +
                                (int32)rd16(lh + 28);
                if (payload >= 0 && payload + c <= extent) {
                    if (exact) {
                        *off = payload;
                        *comp = c;
                        *raw = r;
                        *method = m;
                        if (dos_time)
                            *dos_time = entry_time;
                        if (dos_date)
                            *dos_date = entry_date;
                        return true;
                    }
                    if (!ci_p) {
                        ci_p = lh;
                        ci_method = m;
                        ci_comp = c;
                        ci_raw = r;
                        ci_local = (uint32)payload;
                        ci_dos_time = entry_time;
                        ci_dos_date = entry_date;
                    }
                }
            }
        }
        p += 46 + nl + el + cl;
    }

    if (ci_p) {
        *off = (int32)ci_local;
        *comp = ci_comp;
        *raw = ci_raw;
        *method = ci_method;
        if (dos_time)
            *dos_time = ci_dos_time;
        if (dos_date)
            *dos_date = ci_dos_date;
        return true;
    }
    return false;
}

/* Format the ZIP last-mod date of name as YYYY-MM-DD. Returns false if the
 * entry is missing or the DOS date fields are invalid / unset. */
static bool zip_entry_ymd(const unsigned char *zip, int32 extent,
                          const char *name, char ymd[11])
{
    int32 off, comp, raw;
    int method;
    uint32 dos_time = 0, dos_date = 0;
    int year, month, day;

    if (!zip_find_entry(zip, extent, name, &off, &comp, &raw, &method,
                        &dos_time, &dos_date))
        return false;
    year = 1980 + (int)((dos_date >> 9) & 0x7f);
    month = (int)((dos_date >> 5) & 0x0f);
    day = (int)(dos_date & 0x1f);
    (void)dos_time;
    if (month < 1 || month > 12 || day < 1 || day > 31)
        return false;
    snprintf(ymd, 11, "%04d-%02d-%02d", year, month, day);
    return true;
}

static unsigned char *inflate_raw(const unsigned char *src, int32 srclen,
                                  int32 rawlen)
{
    unsigned char *out;
    size_t got;

    if (rawlen <= 0 || srclen <= 0)
        return NULL;
    out = (unsigned char *)malloc((size_t)rawlen + 1);
    if (!out)
        return NULL;
    got = tinfl_decompress_mem_to_mem(out, (size_t)rawlen, src, (size_t)srclen,
                                      0);
    if (got == TINFL_DECOMPRESS_MEM_TO_MEM_FAILED || (int32)got != rawlen) {
        free(out);
        return NULL;
    }
    out[rawlen] = '\0';
    return out;
}

static unsigned char *zip_extract(const unsigned char *zip, int32 extent,
                                  const char *name, int32 *outlen)
{
    int32 off, comp, raw;
    int method;

    if (!zip_find_entry(zip, extent, name, &off, &comp, &raw, &method, NULL,
                        NULL))
        return NULL;
    if (method == 0) {
        unsigned char *out = (unsigned char *)malloc((size_t)raw + 1);
        if (!out)
            return NULL;
        memcpy(out, zip + off, (size_t)raw);
        out[raw] = '\0';
        *outlen = raw;
        return out;
    }
    if (method == 8) {
        unsigned char *out = inflate_raw(zip + off, comp, raw);
        if (out)
            *outlen = raw;
        return out;
    }
    return NULL;
}

static bool quest_package_found(unsigned char *story_file, int32 extent)
{
    int32 off, comp, raw;
    int method;

    if (story_file == NULL || extent < 30)
        return false;
    if (memcmp(story_file, "PK\003\004", 4) != 0)
        return false;
    return zip_find_entry(story_file, extent, "game.aslx", &off, &comp, &raw,
                          &method, NULL, NULL);
}

static bool aslx_header_found(unsigned char *story_file, int32 extent)
{
    int32 i = 0;
    int32 limit;

    if (story_file == NULL)
        return false;
    if (extent >= 3 && story_file[0] == 0xef && story_file[1] == 0xbb &&
        story_file[2] == 0xbf)
        i = 3;
    while (i < extent && isspace(story_file[i]))
        i++;
    if (i >= extent || story_file[i] != '<')
        return false;
    limit = extent < 2048 ? extent : 2048;
    for (; i + 4 < limit; i++) {
        if (memcmp(story_file + i, "<asl", 4) == 0 &&
            (story_file[i + 4] == ' ' || story_file[i + 4] == '>' ||
             story_file[i + 4] == '\t' || story_file[i + 4] == '\r' ||
             story_file[i + 4] == '\n'))
            return true;
    }
    return false;
}

/* If string is found, returns a pointer to the char AFTER it */
static unsigned char *find_string(unsigned char *storyvp, int32 extent,
                                  const char *string, int32 stringlength)
{
    int32 i;
    for (i = 0; i < extent - stringlength - 1; i++)
        if (memcmp(string, storyvp + i, (size_t)stringlength) == 0)
            return storyvp + i + stringlength;
    return NULL;
}

/* Compiled Quest 1–4: QCGF001 / QCGF002 / QCGF003 */
static bool quest_cas_header_found(unsigned char *story_file)
{
    if (story_file == NULL)
        return false;
    if (memcmp(story_file, "QCGF00", 6) != 0)
        return false;
    return story_file[6] >= '1' && story_file[6] <= '3';
}

/* Quest 1–4 ASL source: "define game" ... "asl-version" ... "end define" */
static bool asl_header_found(unsigned char *story_file, int32 extent)
{
    unsigned char *start, *end, *version;
    size_t offset;

    if (story_file == NULL)
        return false;
    start = find_string(story_file, extent, "define game ", 12);
    if (start == NULL)
        return false;
    offset = (size_t)(start - story_file);
    end = find_string(start, extent - (int32)offset, "end define", 10);
    if (end == NULL)
        return false;
    version = find_string(start, (int32)(end - start) - 10, "asl-version", 11);
    return version != NULL;
}

/* Case-insensitive match of len bytes at p against lower-case ascii key. */
static bool ci_prefix(const char *p, const char *key, size_t len)
{
    size_t i;
    for (i = 0; i < len; i++) {
        if (tolower((unsigned char)p[i]) != (unsigned char)key[i])
            return false;
    }
    return true;
}

static int cas_version_of(const unsigned char *story_file, int32 extent)
{
    if (extent < 8 || memcmp(story_file, "QCGF", 4) != 0)
        return 0;
    if (memcmp(story_file + 4, "001", 3) == 0)
        return 1;
    if (memcmp(story_file + 4, "002", 3) == 0)
        return 2;
    if (memcmp(story_file + 4, "003", 3) == 0)
        return 3;
    return 0;
}

/*
 * CAS keyword table (byte → ASL keyword), from Quest's quest.dat / geas
 * readfile.cc. Control-byte slots stay empty; decompile handles those by value.
 */
static const char *cas_tokens[256] = {
    "",           "game",        "procedure",  "room",        "object",
    "character",  "text",        "selection",  "define",      "end",
    "",           "asl-version", "game",       "version",     "author",
    "copyright",  "info",        "start",      "possitems",   "startitems",
    "prefix",     "look",        "out",        "gender",      "speak",
    "take",       "alias",       "place",      "east",        "north",
    "west",       "south",       "give",       "hideobject",  "hidechar",
    "showobject", "showchar",    "collectable","collecatbles","command",
    "use",        "hidden",      "script",     "font",        "default",
    "fontname",   "fontsize",    "startscript","nointro",     "indescription",
    "description","function",    "setvar",     "for",         "error",
    "synonyms",   "beforeturn",  "afterturn",  "invisible",   "nodebug",
    "suffix",     "startin",     "northeast",  "northwest",   "southeast",
    "southwest",  "items",       "examine",    "detail",      "drop",
    "everywhere", "nowhere",     "on",         "anything",    "article",
    "gain",       "properties",  "type",       "action",      "displaytype",
    "override",   "enabled",     "disabled",   "variable",    "value",
    "display",    "nozero",      "onchange",   "timer",       "alt",
    "lib",        "up",          "down",       "gametype",    "singleplayer",
    "multiplayer","verb",        "menu",       "container",   "surface",
    "transparent","opened",      "parent",     "open",        "close",
    "add",        "remove",      "list",       "empty",       "closed",
    "options",    "abbreviations","locked",
    /* 113–149 unused */
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "",
    /* 150+ */
    "do",         "if",          "got",        "then",        "else",
    "has",        "say",         "playwav",    "lose",        "msg",
    "not",        "playerlose",  "playerwin",  "ask",         "goto",
    "set",        "show",        "choice",     "choose",      "is",
    "setstring",  "displaytext", "exec",       "pause",       "clear",
    "debug",      "enter",       "movechar",   "moveobject",  "revealchar",
    "revealobject","concealchar","concealobject","mailto",    "and",
    "or",         "outputoff",   "outputon",   "here",        "playmidi",
    "drop",       "helpmsg",     "helpdisplaytext","helpclear","helpclose",
    "hide",       "show",        "move",       "conceal",     "reveal",
    "numeric",    "string",      "collectable","property",    "create",
    "exit",       "doaction",    "close",      "each",        "in",
    "repeat",     "while",       "until",      "timeron",     "timeroff",
    "stop",       "panes",       "on",         "off",         "return",
    "playmod",    "modvolume",   "clone",      "shellexe",    "background",
    "foreground", "wait",        "picture",    "nospeak",     "animate",
    "persist",    "inc",         "dec",        "flag",        "dontprocess",
    "destroy",    "beforesave",  "onload",     "playmp3",     "extract",
    "shell",      "popup",       "select",     "case",        "lock",
    "unlock",
    "", "", "", "", "", "", "", "", "", ""
};

typedef struct {
    char *p;
    size_t n;
    size_t cap;
} dbuf;

static int dbuf_grow(dbuf *d, size_t need)
{
    char *np;
    size_t ncap;
    if (d->n + need + 1 <= d->cap)
        return 1;
    ncap = d->cap ? d->cap * 2 : 4096;
    while (ncap < d->n + need + 1)
        ncap *= 2;
    np = (char *)realloc(d->p, ncap);
    if (!np)
        return 0;
    d->p = np;
    d->cap = ncap;
    return 1;
}

static int dbuf_putc(dbuf *d, char c)
{
    if (!dbuf_grow(d, 1))
        return 0;
    d->p[d->n++] = c;
    return 1;
}

static int dbuf_puts(dbuf *d, const char *s)
{
    size_t len = strlen(s);
    if (!dbuf_grow(d, len))
        return 0;
    memcpy(d->p + d->n, s, len);
    d->n += len;
    return 1;
}

static int dbuf_append_line(dbuf *out, dbuf *line)
{
    if (!dbuf_grow(out, line->n + 1))
        return 0;
    memcpy(out->p + out->n, line->p, line->n);
    out->n += line->n;
    return dbuf_putc(out, '\n');
}

/* Decompile QCGF CAS to raw ASL-like text (malloc'd). Stops after the first
 * define game … end define block when present (enough for iFiction). */
static char *cas_decompile_asl(const unsigned char *cas, int32 extent,
                               int32 *out_len)
{
    int cas_version = cas_version_of(cas, extent);
    dbuf out = {NULL, 0, 0};
    dbuf line = {NULL, 0, 0};
    int obfus = 0;
    int expect_text = 0;
    int32 i;
    int seen_game = 0;
    int ok = 1;

    if (cas_version == 0 || extent < 9)
        return NULL;

    for (i = 8; ok && i < extent; i++) {
        unsigned char ch = cas[i];
        const char *tok;

        if (cas_version >= 3 && ch == 252 && obfus == 0 && expect_text != 2)
            break;

        if (obfus == 1 && ch == 0) {
            ok = dbuf_puts(&line, "> ");
            obfus = 0;
        } else if (obfus == 1) {
            ok = dbuf_putc(&line, (char)(255 - ch));
        } else if (obfus == 2 && ch == 254) {
            obfus = 0;
            ok = dbuf_putc(&line, ' ');
        } else if (obfus == 2) {
            ok = dbuf_putc(&line, (char)ch);
        } else if (expect_text == 2) {
            if (ch == 253) {
                expect_text = 0;
                ok = dbuf_append_line(&out, &line);
                line.n = 0;
            } else if (ch == 0) {
                ok = dbuf_append_line(&out, &line);
                line.n = 0;
            } else {
                ok = dbuf_putc(&line, (char)(255 - ch));
            }
        } else if (obfus == 0 && ch == 10) {
            ok = dbuf_putc(&line, '<');
            obfus = 1;
        } else if (obfus == 0 && ch == 254) {
            obfus = 2;
        } else if (ch == 255) {
            if (expect_text == 1)
                expect_text = 2;
            if (line.n >= 12 && ci_prefix(line.p, "define game ", 12))
                seen_game = 1;
            ok = dbuf_append_line(&out, &line);
            if (ok && seen_game && line.n >= 10 &&
                ci_prefix(line.p, "end define", 10)) {
                line.n = 0;
                break;
            }
            line.n = 0;
        } else {
            tok = cas_tokens[ch];
            if (line.n == 7 && memcmp(line.p, "define ", 7) == 0 &&
                (strcmp(tok, "text") == 0 ||
                 (cas_version >= 2 &&
                  (strcmp(tok, "synonyms") == 0 || strcmp(tok, "type") == 0 ||
                   strcmp(tok, "menu") == 0))))
                expect_text = 1;
            ok = dbuf_puts(&line, tok) && dbuf_putc(&line, ' ');
        }
    }

    free(line.p);
    if (!ok || !out.p) {
        free(out.p);
        return NULL;
    }
    if (!dbuf_grow(&out, 1)) {
        free(out.p);
        return NULL;
    }
    out.p[out.n] = '\0';
    *out_len = (int32)out.n;
    return out.p;
}

/* Locate define game … end define. *block points at "define game", *bend at
 * "end define". Returns false if not found. */
static bool find_asl_game_block(const char *text, int32 extent,
                                const char **block, const char **bend)
{
    const char *p = text;
    const char *end = text + extent;
    const char *start, *ed;

    while (p + 12 <= end) {
        if (ci_prefix(p, "define game ", 12)) {
            start = p;
            ed = start + 12;
            while (ed + 10 <= end) {
                if (ci_prefix(ed, "end define", 10)) {
                    *block = start;
                    *bend = ed;
                    return true;
                }
                ed++;
            }
            return false;
        }
        p++;
    }
    return false;
}

/* Copy the first <…> value after a case-insensitive field name within
 * [lo, hi). Caller frees. */
static char *get_asl_angled_field(const char *lo, const char *hi,
                                  const char *field)
{
    size_t flen = strlen(field);
    const char *p = lo;

    while (p + (int32)flen < hi) {
        if (ci_prefix(p, field, flen)) {
            const char *q = p + flen;
            const char *gt, *out_s;
            size_t n;
            char *out;

            while (q < hi && (*q == ' ' || *q == '\t'))
                q++;
            if (q >= hi || *q != '<') {
                p++;
                continue;
            }
            out_s = q + 1;
            gt = memchr(out_s, '>', (size_t)(hi - out_s));
            if (!gt)
                return NULL;
            n = (size_t)(gt - out_s);
            out = (char *)malloc(n + 1);
            if (!out)
                return NULL;
            memcpy(out, out_s, n);
            out[n] = '\0';
            /* trim ends */
            {
                char *a = out;
                char *b = out + n;
                while (*a && isspace((unsigned char)*a))
                    a++;
                while (b > a && isspace((unsigned char)b[-1]))
                    b--;
                if (a != out)
                    memmove(out, a, (size_t)(b - a));
                out[b - a] = '\0';
            }
            return out;
        }
        p++;
    }
    return NULL;
}

/* Title from "define game <…>" on the opening line of the block. */
static char *get_asl_title(const char *block, const char *bend)
{
    const char *p = block;
    const char *line_end;

    if (!ci_prefix(p, "define game ", 12))
        return NULL;
    p += 12;
    while (p < bend && (*p == ' ' || *p == '\t'))
        p++;
    line_end = p;
    while (line_end < bend && *line_end != '\n' && *line_end != '\r')
        line_end++;
    if (p < line_end && *p == '<') {
        const char *gt = memchr(p + 1, '>', (size_t)(line_end - (p + 1)));
        size_t n;
        char *out;
        if (!gt)
            return NULL;
        n = (size_t)(gt - (p + 1));
        out = (char *)malloc(n + 1);
        if (!out)
            return NULL;
        memcpy(out, p + 1, n);
        out[n] = '\0';
        return out;
    }
    return NULL;
}

/* Strip Quest text codes (|n → newline; drop |b/|xb/|i/|xi). In place. */
static void strip_quest_codes(char *s)
{
    char *w = s;
    const char *r = s;
    while (*r) {
        if (*r == '|') {
            char c1 = r[1] ? (char)tolower((unsigned char)r[1]) : 0;
            char c2 = r[2] ? (char)tolower((unsigned char)r[2]) : 0;
            if (c1 == 'n') {
                *w++ = '\n';
                r += 2;
                continue;
            }
            if (c1 == 'b' || c1 == 'i') {
                r += 2;
                continue;
            }
            if (c1 == 'x' && (c2 == 'b' || c2 == 'i')) {
                r += 3;
                continue;
            }
        }
        *w++ = *r++;
    }
    *w = '\0';
}

/* Earliest 19xx/20xx year in s; writes YYYY into out[5]. Returns false if none. */
static bool earliest_year(const char *s, char out[5])
{
    const char *p = s;
    const char *best = NULL;

    while (*p) {
        if ((p[0] == '1' && p[1] == '9') || (p[0] == '2' && p[1] == '0')) {
            if (isdigit((unsigned char)p[2]) && isdigit((unsigned char)p[3])) {
                if (!isdigit((unsigned char)p[4]) &&
                    (p == s || !isdigit((unsigned char)p[-1]))) {
                    if (!best || memcmp(p, best, 4) < 0)
                        best = p;
                }
            }
        }
        p++;
    }
    if (!best)
        return false;
    memcpy(out, best, 4);
    out[4] = '\0';
    return true;
}

static int32 claim_story_file(void *storyvp, int32 extent)
{
    if (extent > 30 && quest_package_found(storyvp, extent))
        return VALID_STORY_FILE_RV;
    if (extent > 10 && aslx_header_found(storyvp, extent))
        return VALID_STORY_FILE_RV;
    if (extent > 8 && quest_cas_header_found(storyvp))
        return VALID_STORY_FILE_RV;
    if (extent > 25 && asl_header_found(storyvp, extent))
        return VALID_STORY_FILE_RV;
    return INVALID_STORY_FILE_RV;
}

static char *get_aslx_text(void *storyvp, int32 extent, int32 *len)
{
    unsigned char *story = storyvp;

    if (extent >= 4 && memcmp(story, "PK\003\004", 4) == 0) {
        int32 l = 0;
        unsigned char *raw = zip_extract(story, extent, "game.aslx", &l);
        if (!raw)
            return NULL;
        *len = l;
        return (char *)raw;
    }
    {
        char *s = (char *)malloc((size_t)extent + 1);
        if (!s)
            return NULL;
        memcpy(s, story, (size_t)extent);
        s[extent] = '\0';
        *len = extent;
        return s;
    }
}

/* -------------------------------------------------------------------------
 * Light XML / HTML helpers for game.aslx
 * ---------------------------------------------------------------------- */

static bool find_game_range(const char *text, const char **gs, const char **ge)
{
    const char *p = text;
    while ((p = strstr(p, "<game")) != NULL) {
        char after = p[5];
        if (after == ' ' || after == '\t' || after == '\r' || after == '\n' ||
            after == '>') {
            const char *end = strstr(p, "</game>");
            if (!end)
                return false;
            *gs = p;
            *ge = end;
            return true;
        }
        p += 5;
    }
    return false;
}

static char *get_attr(const char *tag, const char *tagend, const char *attr)
{
    char pat[64];
    const char *p;
    size_t patlen;

    snprintf(pat, sizeof(pat), "%s=\"", attr);
    patlen = strlen(pat);
    p = tag;
    while (p < tagend) {
        const char *hit = strstr(p, pat);
        if (!hit || hit >= tagend)
            return NULL;
        if (hit == tag || isspace((unsigned char)hit[-1])) {
            const char *v = hit + patlen;
            const char *q = strchr(v, '"');
            size_t n;
            char *out;
            if (!q || q > tagend)
                return NULL;
            n = (size_t)(q - v);
            out = (char *)malloc(n + 1);
            if (!out)
                return NULL;
            memcpy(out, v, n);
            out[n] = '\0';
            return out;
        }
        p = hit + patlen;
    }
    return NULL;
}

static char *get_child(const char *lo, const char *hi, const char *tag)
{
    char open[64];
    size_t openlen;
    const char *p;

    snprintf(open, sizeof(open), "<%s", tag);
    openlen = strlen(open);
    p = lo;
    while (p < hi) {
        const char *hit = strstr(p, open);
        char after;
        const char *gt;
        char close[64];
        const char *end;
        const char *v;
        size_t n;
        char *out;

        if (!hit || hit >= hi)
            return NULL;
        after = hit[openlen];
        if (after == ' ' || after == '\t' || after == '\r' || after == '\n' ||
            after == '>' || after == '/') {
            gt = strchr(hit, '>');
            if (!gt || gt >= hi)
                return NULL;
            if (gt[-1] == '/')
                return NULL;
            snprintf(close, sizeof(close), "</%s>", tag);
            end = strstr(gt + 1, close);
            if (!end || end > hi)
                return NULL;
            v = gt + 1;
            n = (size_t)(end - v);
            if (n >= 12 && memcmp(v, "<![CDATA[", 9) == 0) {
                const char *cend = strstr(v, "]]>");
                if (cend && cend < end) {
                    v += 9;
                    n = (size_t)(cend - v);
                }
            }
            out = (char *)malloc(n + 1);
            if (!out)
                return NULL;
            memcpy(out, v, n);
            out[n] = '\0';
            return out;
        }
        p = hit + openlen;
    }
    return NULL;
}

static void decode_entities(char *s)
{
    char *w = s;
    const char *r = s;
    while (*r) {
        if (*r == '&') {
            const char *sc = strchr(r, ';');
            if (sc && sc - r <= 10) {
                size_t n = (size_t)(sc - r - 1);
                char ent[11];
                int code = -1;
                memcpy(ent, r + 1, n);
                ent[n] = '\0';
                if (ent[0] == '#') {
                    code = (ent[1] == 'x' || ent[1] == 'X')
                               ? (int)strtol(ent + 2, NULL, 16)
                               : (int)strtol(ent + 1, NULL, 10);
                } else if (strcmp(ent, "lt") == 0)
                    code = '<';
                else if (strcmp(ent, "gt") == 0)
                    code = '>';
                else if (strcmp(ent, "amp") == 0)
                    code = '&';
                else if (strcmp(ent, "quot") == 0)
                    code = '"';
                else if (strcmp(ent, "apos") == 0)
                    code = '\'';
                else if (strcmp(ent, "nbsp") == 0)
                    code = ' ';
                if (code > 0) {
                    if (code < 0x80) {
                        *w++ = (char)code;
                    } else if (code < 0x800) {
                        *w++ = (char)(0xC0 | (code >> 6));
                        *w++ = (char)(0x80 | (code & 0x3F));
                    } else {
                        *w++ = (char)(0xE0 | (code >> 12));
                        *w++ = (char)(0x80 | ((code >> 6) & 0x3F));
                        *w++ = (char)(0x80 | (code & 0x3F));
                    }
                    r = sc + 1;
                    continue;
                }
            }
        }
        *w++ = *r++;
    }
    *w = '\0';
}

static void strip_tags(char *s)
{
    char *w = s;
    const char *r = s;
    while (*r) {
        if (*r == '<') {
            const char *gt = strchr(r, '>');
            if (!gt)
                break;
            if ((r[1] == 'b' || r[1] == 'B') && (r[2] == 'r' || r[2] == 'R'))
                *w++ = '\n';
            r = gt + 1;
            continue;
        }
        *w++ = *r++;
    }
    while (*r)
        *w++ = *r++;
    *w = '\0';
}

static void clean_html(char *s, bool collapse)
{
    decode_entities(s);
    strip_tags(s);
    if (collapse) {
        char *out = s;
        const char *in = s;
        int pending = 0;
        while (isspace((unsigned char)*in))
            in++;
        while (*in) {
            if (isspace((unsigned char)*in)) {
                pending = 1;
                in++;
            } else {
                if (pending) {
                    *out++ = ' ';
                    pending = 0;
                }
                *out++ = *in++;
            }
        }
        *out = '\0';
    }
}

/* -------------------------------------------------------------------------
 * iFiction synthesis / cover
 * ---------------------------------------------------------------------- */

typedef struct {
    char *buf;
    int32 cap;
    int32 total;
} sink;

static void put(sink *s, const char *src, size_t n)
{
    int32 room = s->cap - s->total;
    if (room > 0 && s->buf) {
        int32 copy = (int32)n < room ? (int32)n : room;
        memcpy(s->buf + s->total, src, (size_t)copy);
    }
    s->total += (int32)n;
}

static void putz(sink *s, const char *src) { put(s, src, strlen(src)); }

static void put_escaped(sink *s, const char *src, bool as_desc)
{
    const char *p;
    for (p = src; *p; p++) {
        switch (*p) {
        case '&':
            putz(s, "&amp;");
            break;
        case '<':
            putz(s, "&lt;");
            break;
        case '>':
            putz(s, "&gt;");
            break;
        case '\n':
            if (as_desc)
                putz(s, "<br/>");
            else
                put(s, " ", 1);
            break;
        case '\r':
            break;
        default:
            put(s, p, 1);
            break;
        }
    }
}

static void put_field(sink *s, const char *tag, const char *val, bool as_desc)
{
    if (!val || !*val)
        return;
    putz(s, "      <");
    putz(s, tag);
    putz(s, ">");
    put_escaped(s, val, as_desc);
    putz(s, "</");
    putz(s, tag);
    putz(s, ">\n");
}

/* Babel Treaty <releases><attached> for this story file; releasedate is
 * mandatory inside <release>. */
static void put_attached_release(sink *s, const char *ymd)
{
    if (!ymd || !*ymd)
        return;
    putz(s, "    <releases>\n"
            "      <attached>\n"
            "        <release>\n"
            "          <releasedate>");
    putz(s, ymd);
    putz(s, "</releasedate>\n"
            "        </release>\n"
            "      </attached>\n"
            "    </releases>\n");
}

/* Treaty 5.12.3: "a third-party tool which synthesizes metadata from a
story file should use either the compilation date (if it can be determined)
of the story file, or else the compilation date of the tool itself".

Copying tads.c, we'll just hardcode a date here: the date the PR was
submitted to add Quest support to Babel. */
#define QUEST_SYNTH_ORIGINATED "2026-09-10"

static void put_colophon(sink *s, const char *originated)
{
    putz(s, "    <colophon>\n"
            "      <generator>Babel</generator>\n"
            "      <generatorversion>" TREATY_VERSION "</generatorversion>\n"
            "      <originated>");
    putz(s, originated && *originated ? originated : QUEST_SYNTH_ORIGINATED);
    putz(s, "</originated>\n"
            "    </colophon>\n");
}

static int cover_format_of(const unsigned char *d, int32 n)
{
    if (n >= 8 && d[0] == 137 && d[1] == 'P' && d[2] == 'N' && d[3] == 'G')
        return PNG_COVER_FORMAT;
    if (n >= 3 && d[0] == 0xFF && d[1] == 0xD8 && d[2] == 0xFF)
        return JPEG_COVER_FORMAT;
    return 0;
}

static void png_dim(const unsigned char *d, int32 *w, int32 *h)
{
    *w = (int32)((d[16] << 24) | (d[17] << 16) | (d[18] << 8) | d[19]);
    *h = (int32)((d[20] << 24) | (d[21] << 16) | (d[22] << 8) | d[23]);
}

static bool jpeg_dim(const unsigned char *d, int32 n, int32 *w, int32 *h)
{
    const unsigned char *p = d + 2, *end = d + n;
    while (p + 9 < end) {
        int marker, seg;
        if (*p != 0xFF) {
            p++;
            continue;
        }
        marker = p[1];
        if ((marker & 0xF0) == 0xC0 && marker != 0xC4 && marker != 0xC8 &&
            marker != 0xCC) {
            *h = (p[5] << 8) | p[6];
            *w = (p[7] << 8) | p[8];
            return true;
        }
        if (p[1] == 0xD8 || p[1] == 0xD9 ||
            (p[1] >= 0xD0 && p[1] <= 0xD7)) {
            p += 2;
            continue;
        }
        seg = (p[2] << 8) | p[3];
        p += 2 + seg;
    }
    return false;
}

static char *coverleaf_from_ifiction(const char *md, int32 len)
{
    const char *lo, *hi, *q;
    char *name;
    (void)len;
    lo = strstr(md, "<quest");
    if (!lo)
        return NULL;
    /* require quest section open tag */
    q = lo + 6;
    if (!(*q == '>' || isspace((unsigned char)*q)))
        return NULL;
    hi = strstr(lo, "</quest>");
    if (!hi)
        return NULL;
    name = get_child(lo, hi, "coverleafname");
    return name;
}

static unsigned char *get_cover(void *storyvp, int32 extent, int32 *len,
                                int *fmt)
{
    unsigned char *story = storyvp;
    char *name = NULL;
    int32 clen = 0;
    unsigned char *img;
    int f;
    int32 mlen = 0;
    unsigned char *md;

    if (!(extent >= 4 && memcmp(story, "PK\003\004", 4) == 0))
        return NULL;

    md = zip_extract(story, extent, "metadata.iFiction", &mlen);
    if (md) {
        name = coverleaf_from_ifiction((char *)md, mlen);
        free(md);
    }
    if (!name || !*name) {
        free(name);
        name = NULL;
        {
            int32 atext_len = 0;
            char *atext = get_aslx_text(storyvp, extent, &atext_len);
            const char *gs, *ge;
            if (atext && find_game_range(atext, &gs, &ge))
                name = get_child(gs, ge, "cover");
            free(atext);
        }
    }
    if (!name || !*name) {
        free(name);
        return NULL;
    }

    img = zip_extract(story, extent, name, &clen);
    free(name);
    if (!img)
        return NULL;
    f = cover_format_of(img, clen);
    if (f == 0) {
        free(img);
        return NULL;
    }
    *fmt = f;
    *len = clen;
    return img;
}

static void md5_ifid(void *storyvp, int32 extent, char ifid[40])
{
    md5_state_t md5;
    unsigned char ob[16];
    int i;

    md5_init(&md5);
    md5_append(&md5, (const md5_byte_t *)storyvp, (int)extent);
    md5_finish(&md5, ob);
    memcpy(ifid, "QUEST-", 6);
    for (i = 0; i < 16; i++)
        sprintf(ifid + 6 + 2 * i, "%02X", ob[i]);
    ifid[38] = '\0';
}

static int32 synth_ifiction_asl_text(const char *asl, int32 asl_len,
                                     void *md5_src, int32 md5_extent,
                                     char *buf, int32 bufsize)
{
    const char *block, *bend;
    char *title, *author, *version, *desc, *copyright;
    char published[5];
    int have_published = 0;
    char ifid[40];
    sink s;

    if (!find_asl_game_block(asl, asl_len, &block, &bend))
        return NO_REPLY_RV;

    title = get_asl_title(block, bend);
    author = get_asl_angled_field(block, bend, "game author");
    version = get_asl_angled_field(block, bend, "game version");
    desc = get_asl_angled_field(block, bend, "game info");
    copyright = get_asl_angled_field(block, bend, "game copyright");

    if (title)
        clean_html(title, true);
    if (author)
        clean_html(author, true);
    if (version)
        clean_html(version, true);
    if (desc) {
        strip_quest_codes(desc);
        clean_html(desc, false);
    }
    if (copyright) {
        clean_html(copyright, true);
        have_published = earliest_year(copyright, published);
    }

    md5_ifid(md5_src, md5_extent, ifid);

    s.buf = buf;
    s.cap = bufsize;
    s.total = 0;

    putz(&s,
         "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
         "<ifindex version=\"1.0\" "
         "xmlns=\"http://babel.ifarchive.org/protocol/iFiction/\">\n"
         "  <story>\n");
    put_colophon(&s, NULL);
    putz(&s, "    <identification>\n"
             "      <ifid>");
    putz(&s, ifid);
    putz(&s, "</ifid>\n"
             "      <format>quest</format>\n"
             "    </identification>\n"
             "    <bibliographic>\n");
    put_field(&s, "title", title ? title : "An Interactive Fiction", false);
    put_field(&s, "author", author ? author : "Anonymous", false);
    if (have_published)
        put_field(&s, "firstpublished", published, false);
    put_field(&s, "description", desc, true);
    putz(&s, "    </bibliographic>\n");

    if (version && *version) {
        putz(&s, "    <quest>\n");
        put_field(&s, "version", version, false);
        putz(&s, "    </quest>\n");
    }

    putz(&s, "  </story>\n</ifindex>\n");

    free(title);
    free(author);
    free(version);
    free(desc);
    free(copyright);
    return s.total;
}

static int32 synth_ifiction_asl(void *storyvp, int32 extent, char *buf,
                                int32 bufsize)
{
    return synth_ifiction_asl_text((const char *)storyvp, extent, storyvp,
                                   extent, buf, bufsize);
}

static int32 synth_ifiction_cas(void *storyvp, int32 extent, char *buf,
                                int32 bufsize)
{
    int32 asl_len = 0;
    char *asl = cas_decompile_asl(storyvp, extent, &asl_len);
    int32 rv;

    if (!asl)
        return NO_REPLY_RV;
    rv = synth_ifiction_asl_text(asl, asl_len, storyvp, extent, buf, bufsize);
    free(asl);
    return rv;
}

static int32 synth_ifiction(void *storyvp, int32 extent, char *buf,
                            int32 bufsize)
{
    int32 atext_len = 0;
    char *atext;
    const char *gs, *ge, *tagend;
    char *title, *gameid, *author, *subtitle, *genre, *published, *desc;
    char *style = NULL, *version = NULL, *coverleaf = NULL;
    char ifid[40];
    char releasedate[11];
    int have_releasedate = 0;
    int32 cover_len = 0, cw = 0, ch = 0;
    int cover_fmt = 0;
    unsigned char *cover;
    sink s;

    if (quest_cas_header_found(storyvp) && cas_version_of(storyvp, extent) > 0)
        return synth_ifiction_cas(storyvp, extent, buf, bufsize);

    if (asl_header_found(storyvp, extent))
        return synth_ifiction_asl(storyvp, extent, buf, bufsize);

    atext = get_aslx_text(storyvp, extent, &atext_len);
    if (!atext)
        return NO_REPLY_RV;
    if (!find_game_range(atext, &gs, &ge)) {
        free(atext);
        return NO_REPLY_RV;
    }
    tagend = strchr(gs, '>');
    if (!tagend || tagend > ge)
        tagend = ge;

    title = get_attr(gs, tagend, "name");
    gameid = get_child(gs, ge, "gameid");
    author = get_child(gs, ge, "author");
    subtitle = get_child(gs, ge, "subtitle");
    genre = get_child(gs, ge, "category");
    published = get_child(gs, ge, "firstpublished");
    desc = get_child(gs, ge, "description");
    style = get_child(gs, ge, "style");
    version = get_child(gs, ge, "version");
    coverleaf = get_child(gs, ge, "cover");

    if (title)
        clean_html(title, true);
    if (author)
        clean_html(author, true);
    if (subtitle)
        clean_html(subtitle, true);
    if (genre)
        clean_html(genre, true);
    if (gameid)
        clean_html(gameid, true);
    if (published)
        clean_html(published, true);
    if (desc)
        clean_html(desc, false);
    if (style)
        clean_html(style, true);
    if (version)
        clean_html(version, true);
    if (coverleaf)
        clean_html(coverleaf, true);

    if (gameid && strlen(gameid) >= 8) {
        size_t n = strlen(gameid);
        size_t k;
        if (n > 38)
            n = 38;
        memcpy(ifid, gameid, n);
        ifid[n] = '\0';
        for (k = 0; ifid[k]; k++)
            ifid[k] = (char)toupper((unsigned char)ifid[k]);
    } else {
        md5_ifid(storyvp, extent, ifid);
    }

    cover = get_cover(storyvp, extent, &cover_len, &cover_fmt);
    if (cover) {
        if (cover_fmt == PNG_COVER_FORMAT && cover_len >= 24)
            png_dim(cover, &cw, &ch);
        else if (cover_fmt == JPEG_COVER_FORMAT)
            jpeg_dim(cover, cover_len, &cw, &ch);
    }

    if (extent >= 4 && memcmp(storyvp, "PK\003\004", 4) == 0)
        have_releasedate =
            zip_entry_ymd(storyvp, extent, "game.aslx", releasedate) ? 1 : 0;

    s.buf = buf;
    s.cap = bufsize;
    s.total = 0;

    putz(&s,
         "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
         "<ifindex version=\"1.0\" "
         "xmlns=\"http://babel.ifarchive.org/protocol/iFiction/\">\n"
         "  <story>\n");
    put_colophon(&s, have_releasedate ? releasedate : NULL);
    putz(&s, "    <identification>\n"
             "      <ifid>");
    putz(&s, ifid);
    putz(&s, "</ifid>\n"
             "      <format>quest</format>\n"
             "    </identification>\n"
             "    <bibliographic>\n");
    put_field(&s, "title", title ? title : "An Interactive Fiction", false);
    put_field(&s, "author", author ? author : "Anonymous", false);
    put_field(&s, "headline", subtitle, false);
    put_field(&s, "genre", genre, false);
    put_field(&s, "firstpublished", published, false);
    put_field(&s, "description", desc, true);
    putz(&s, "    </bibliographic>\n");

    if (cover && (cover_fmt == PNG_COVER_FORMAT ||
                  cover_fmt == JPEG_COVER_FORMAT) &&
        cw > 0 && ch > 0) {
        char b[160];
        snprintf(b, sizeof(b),
                 "    <cover>\n"
                 "      <format>%s</format>\n"
                 "      <height>%ld</height>\n"
                 "      <width>%ld</width>\n"
                 "    </cover>\n",
                 cover_fmt == PNG_COVER_FORMAT ? "png" : "jpg", (long)ch,
                 (long)cw);
        putz(&s, b);
    }

    if (have_releasedate)
        put_attached_release(&s, releasedate);

    if ((style && *style) || (version && *version) ||
        (coverleaf && *coverleaf)) {
        putz(&s, "    <quest>\n");
        put_field(&s, "style", style, false);
        put_field(&s, "version", version, false);
        put_field(&s, "coverleafname", coverleaf, false);
        putz(&s, "    </quest>\n");
    }

    putz(&s, "  </story>\n</ifindex>\n");

    free(cover);
    free(title);
    free(gameid);
    free(author);
    free(subtitle);
    free(genre);
    free(published);
    free(desc);
    free(style);
    free(version);
    free(coverleaf);
    free(atext);
    return s.total;
}

static int32 get_story_file_metadata_extent(void *story_file, int32 extent)
{
    int32 mlen = 0;
    unsigned char *md;

    if (extent >= 4 && memcmp(story_file, "PK\003\004", 4) == 0) {
        md = zip_extract(story_file, extent, "metadata.iFiction", &mlen);
        if (md) {
            free(md);
            return mlen + 1; /* NUL for C-string consumers */
        }
    }
    {
        int32 need = synth_ifiction(story_file, extent, NULL, 0);
        if (need < 0)
            return need;
        return need + 1;
    }
}

static int32 get_story_file_metadata(void *story_file, int32 extent,
                                     char *output, int32 output_extent)
{
    int32 mlen = 0;
    unsigned char *md;

    if (extent >= 4 && memcmp(story_file, "PK\003\004", 4) == 0) {
        md = zip_extract(story_file, extent, "metadata.iFiction", &mlen);
        if (md) {
            if (mlen + 1 > output_extent) {
                free(md);
                return INVALID_USAGE_RV;
            }
            memcpy(output, md, (size_t)mlen);
            output[mlen] = '\0';
            free(md);
            return mlen + 1;
        }
    }
    {
        int32 need = synth_ifiction(story_file, extent, output, output_extent);
        if (need < 0)
            return need;
        if (need + 1 > output_extent)
            return INVALID_USAGE_RV;
        output[need] = '\0';
        return need + 1;
    }
}

static int32 get_story_file_cover_extent(void *story_file, int32 extent)
{
    int32 len = 0;
    int fmt = 0;
    unsigned char *c = get_cover(story_file, extent, &len, &fmt);
    if (!c)
        return NO_REPLY_RV;
    free(c);
    return len;
}

static int32 get_story_file_cover_format(void *story_file, int32 extent)
{
    int32 len = 0;
    int fmt = 0;
    unsigned char *c = get_cover(story_file, extent, &len, &fmt);
    if (!c)
        return NO_REPLY_RV;
    free(c);
    return fmt;
}

static int32 get_story_file_cover(void *story_file, int32 extent, void *output,
                                  int32 output_extent)
{
    int32 len = 0;
    int fmt = 0;
    unsigned char *c = get_cover(story_file, extent, &len, &fmt);
    if (!c)
        return NO_REPLY_RV;
    if (len > output_extent) {
        free(c);
        return INVALID_USAGE_RV;
    }
    memcpy(output, c, (size_t)len);
    free(c);
    return len;
}

static int32 get_story_file_IFID(void *storyvp, int32 extent, char *output,
                                 int32 output_extent)
{
    int32 ix;
    char *atext = NULL;
    unsigned char *search;
    int32 search_len;
    int32 atext_len = 0;
    unsigned char *p;
    char guid[64];
    int n;
    int32 left;

    if (claim_story_file(storyvp, extent) != VALID_STORY_FILE_RV)
        return INVALID_STORY_FILE_RV;

    ix = find_uuid_ifid_marker(storyvp, extent, output, output_extent);
    if (ix == VALID_STORY_FILE_RV || ix == INVALID_USAGE_RV)
        return ix;

    search = storyvp;
    search_len = extent;
    if (extent >= 4 && memcmp(storyvp, "PK\003\004", 4) == 0) {
        atext = get_aslx_text(storyvp, extent, &atext_len);
        if (atext) {
            search = (unsigned char *)atext;
            search_len = atext_len;
        }
    }

    p = NULL;
    {
        int32 i;
        int32 lim = search_len - 8;
        for (i = 0; i < lim; i++) {
            if (memcmp(search + i, "<gameid>", 8) == 0) {
                p = search + i + 8;
                break;
            }
        }
    }
    if (p != NULL) {
        n = 0;
        left = search_len - (int32)(p - search);
        while (n < 63 && n < left && p[n] != '<' &&
               (isxdigit(p[n]) || p[n] == '-')) {
            guid[n] = (char)toupper(p[n]);
            n++;
        }
        guid[n] = 0;
        if (n >= 8 && n < left && p[n] == '<') {
            ASSERT_OUTPUT_SIZE(n + 1);
            memcpy(output, guid, (size_t)(n + 1));
            free(atext);
            return 1;
        }
    }

    free(atext);
    ASSERT_OUTPUT_SIZE(7);
    strcpy(output, "QUEST-");
    return INCOMPLETE_REPLY_RV;
}
