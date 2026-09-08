/*

IniLite

A header-only C library for serializing and de-serializing `.ini` files.

Repository     https://github.com/simon-danielsson/inilite
Author         Simon Danielsson
Contact        contact@simondanielsson.se
License        MIT

See the end of this file for more information.

*/

#ifndef INILITE_H_INCLUDE
#define INILITE_H_INCLUDE

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#endif // INILITE_H_INCLUDE

#ifdef INILITE_IMPLEMENTATION

#define _Ini_internal static

#define _Ini_COMMENT ';'
#define _Ini_DELIM '='
#define _Ini_SEC_HEAD_BEGIN '['
#define _Ini_SEC_HEAD_END ']'

#define _IniSection_max_kv_count 512

#ifdef INILITE_DEBUG
#define _Ini_log(...) fprintf(stderr, __VA_ARGS__)
#else
#define _Ini_log(...)
#endif

typedef struct IniSection {
  char *name;
  char *kv[_IniSection_max_kv_count][2]; // 0 = key, 1 = value
  size_t kv_count;                       // number of key-value pairs
} IniSection;

typedef struct Ini {
  IniSection *sections;
  size_t count;
  size_t cap;
} Ini;

_Ini_internal void _Ini_quote_trim(char *s) {
  char *start = s;
  char *end;
  size_t len;

  while (*start == '"')
    start++;

  if (*start == '\0') {
    s[0] = '\0';
    return;
  }

  end = start + strlen(start) - 1;

  while (end > start && *end == '"')
    end--;

  len = (size_t)(end - start) + 1;
  memmove(s, start, len);
  s[len] = '\0';
}

_Ini_internal void _Ini_str_trim(char *s) {
  char *start = s, *end;
  size_t len;
  while (*start && isspace((unsigned char)*start))
    start++;
  if (*start == '\0') {
    s[0] = '\0';
    return;
  }
  end = start + strlen(start) - 1;
  while (end > start && isspace((unsigned char)*end))
    end--;
  len = (size_t)(end - start) + 1;
  memmove(s, start, len);
  s[len] = '\0';
}

_Ini_internal char *_Ini_strdup(const char *s) {
  char *out;
  if (!s)
    return NULL;
  out = malloc(strlen(s) + 1);
  if (!out)
    return NULL;
  strcpy(out, s);
  return out;
}

_Ini_internal bool _Ini_str_is_empty(const char *s) {
  int i;
  for (i = 0; s[i] != '\0'; i++) {
    if (isalpha(s[i])) {
      return false;
    }
  }
  return true;
}

#define _Ini_value_line_len 54

_Ini_internal char *_Ini_add_linebreaks(char *val) {
  char tmp[2048], *c = val;
  size_t tmp_len = 0;
  bool line_break_due = false;
  while (*c) {
    if (tmp_len > (_Ini_value_line_len / 2) &&
        tmp_len % _Ini_value_line_len == 0)
      line_break_due = true;
    if ((*c == ' ' || *c == '_' || *c == '-' || *c == '.' || *c == ',') &&
        line_break_due == true) {
      tmp[tmp_len++] = *c;
      c++;
      tmp[tmp_len++] = '\\';
      tmp[tmp_len++] = '\n';
      line_break_due = false;
    }
    tmp[tmp_len++] = *c;
    c++;
  }
  tmp[tmp_len] = '\0';
  return _Ini_strdup(tmp);
}

typedef enum {
  _INI_APPEND_KV_WRITE,
  _INI_APPEND_KV_READ,
} _Ini_append_kv_type;

_Ini_internal bool _Ini_append_kv_intern(Ini *ini, char *key, char *val,
                                         _Ini_append_kv_type type) {
  if (!key || !val)
    return false;

#define SEC ini->sections[ini->count - 1]
#define KV_COUNT SEC.kv_count

  if (KV_COUNT >= _IniSection_max_kv_count)
    return false;

  SEC.kv[KV_COUNT][0] = _Ini_strdup(key);

  if ((strlen(val) > _Ini_value_line_len) && type == _INI_APPEND_KV_WRITE)
    SEC.kv[KV_COUNT][1] = _Ini_add_linebreaks(val);
  else
    SEC.kv[KV_COUNT][1] = _Ini_strdup(val);

  SEC.kv_count++;
  return true;
#undef SEC
#undef KV_COUNT
}

bool Ini_append_kv(Ini *ini, char *key, char *val) {
  return _Ini_append_kv_intern(ini, key, val, _INI_APPEND_KV_WRITE);
}

_Ini_internal bool _Ini_intern_append_section(Ini *ini,
                                              const IniSection section) {
  if (ini->count == ini->cap) {
    size_t new_cap = ini->cap * 2;
    void *new_sections = realloc(ini->sections, new_cap * sizeof(IniSection));
    if (new_sections == NULL)
      return false;
    ini->sections = new_sections;
    ini->cap = new_cap;
  }
  ini->sections[ini->count] = section;
  if (section.name == NULL)
    return false;
  ini->sections[ini->count].name = _Ini_strdup(section.name);
  ini->count++;
  return true;
}

IniSection *Ini_get_section(Ini *ini, char *section_name) {
  if (!section_name || !ini)
    return NULL;
  size_t i;
  for (i = 0; i < ini->count; i++) {
    if (strstr(ini->sections[i].name, section_name) != 0) {
      return &ini->sections[i];
    }
  }
  return NULL;
}

char *IniSection_get_value(IniSection *sec, const char *key) {
  if (!sec || !key)
    return NULL;
  size_t i;
  for (i = 0; i < sec->kv_count; i++) {
    if (strstr(sec->kv[i][0], key) != 0) {
      return sec->kv[i][1];
    }
  }
  return NULL;
}

void Ini_free(Ini *ini) {
  if (!ini)
    return;
  for (size_t i = 0; i < ini->count; i++) {
    for (size_t j = 0; j < ini->sections[i].kv_count; j++) {
      free(ini->sections[i].kv[j][0]);
      free(ini->sections[i].kv[j][1]);
    }
    free(ini->sections[i].name);
  }
  free(ini->sections);
}

void Ini_append_section(Ini *ini, char *name) {
  if (!ini || !name)
    return;
  _Ini_intern_append_section(
      ini, (IniSection){.kv = {0}, .kv_count = 0, .name = name});
}

_Ini_internal char **_Ini_split_rows(size_t *n_lines, char *content) {
  char row_tmp[2048], *c, **split_lines;
  size_t count = 0, row_tmp_len = 0, split_lines_n = 0;

  // first pass: logical lines accounting for '\' new-line escape
  // note: newline preceded by '' is considered a continuation
  c = content;
  while (*c) {
    if (*c == '\n' && (c == content || *(c - 1) != '\\'))
      count++;
    c++;
  }

  // If doesn't end in newline, there's a final line
  if (c != content && *(c - 1) != '\n')
    count++;

  *n_lines = count;

  split_lines = malloc(sizeof(char *) * count);
  assert(split_lines != NULL);

  // second pass with line count collected after first pass

  c = content;
  while (*c) {
    if (*c == '\\' && *(c + 1) == '\n') {
      c += 2;
      continue;
    }

    if (*c == '\n') {
      row_tmp[row_tmp_len] = '\0';
      split_lines[split_lines_n++] = _Ini_strdup(row_tmp);

      assert(split_lines_n <= count);

      row_tmp_len = 0;
      c++;
      continue;
    }

    assert(row_tmp_len < sizeof(row_tmp) - 1);
    row_tmp[row_tmp_len++] = *c++;
  }

  // final line
  if (row_tmp_len > 0) {
    row_tmp[row_tmp_len] = '\0';
    split_lines[split_lines_n++] = _Ini_strdup(row_tmp);
  }

  *n_lines = split_lines_n;
  return split_lines;
}

#define _INI_PARSE_STACK_BUFF_SZ 2048 * 1000
_Ini_internal bool _Ini_parse(Ini *ini, char *content) {
  size_t row, col, i, n_rows;
  char *current_row, **rows;

  rows = _Ini_split_rows(&n_rows, content);

  for (row = 0; row < n_rows; row++) {
    col = 0;
    current_row = rows[row];

    if (_Ini_str_is_empty(current_row) || current_row[col] == _Ini_COMMENT)
      continue;

    if (current_row[col] == _Ini_SEC_HEAD_BEGIN) {
      col = 1;
      _Ini_log("SECTION ");
      char section[_INI_PARSE_STACK_BUFF_SZ];
      size_t section_len = 0;
      while (current_row[col] != _Ini_SEC_HEAD_END) {
        if (current_row[col] == '\0')
          goto failure;
        _Ini_log("%c", current_row[col]);
        section[section_len++] = current_row[col++];
      }
      _Ini_log("\n");
      section[section_len] = '\0';
      _Ini_intern_append_section(
          ini, (IniSection){.kv = {0}, .kv_count = 0, .name = section});
      continue;
    }

    // else this must be a key-value row

    {
      if (strstr(current_row, (char[2]){_Ini_DELIM, '\0'}) == 0)
        goto failure;

      char key[_INI_PARSE_STACK_BUFF_SZ], val[_INI_PARSE_STACK_BUFF_SZ];
      size_t key_len = 0, val_len = 0;

      while (current_row[col] != _Ini_DELIM) {
        if (current_row[col] == '\0')
          goto failure;
        key[key_len++] = current_row[col++];
      }
      col++;

      while (current_row[col] != '\0') {
        val[val_len++] = current_row[col++];
      }

      key[key_len] = '\0';
      val[val_len] = '\0';
      _Ini_str_trim(key);
      _Ini_str_trim(val);
      _Ini_quote_trim(val);
      if (!_Ini_append_kv_intern(ini, key, val, _INI_APPEND_KV_READ)) {
        return false;
      }
      _Ini_log("KEY %s VALUE %s\n", key, val);
    }
  }

  for (i = 0; i < n_rows; i++)
    free(rows[i]);
  free(rows);
  return true;

failure:
  for (i = 0; i < n_rows; i++)
    free(rows[i]);
  free(rows);
  return false;
}

/* Formats `Ini` data and inserts it into user-provided buffer; ready for
   writing to a file or whatever else. */
void Ini_build(Ini *ini, char *buf) {
  for (size_t i = 0; i < ini->count; i++) {
    strcat(buf, "\n[");
    strcat(buf, ini->sections[i].name);
    strcat(buf, "]\n");

    for (size_t j = 0; j < ini->sections[i].kv_count; j++) {
      strcat(buf, ini->sections[i].kv[j][0]);
      strcat(buf, " = ");
      strcat(buf, ini->sections[i].kv[j][1]);
      strcat(buf, "\n");
    }
  }
}

/* Initiates an `Ini` struct. File content provided with `content` parameter
   will be parsed and appended to the array. `content` can also be `NULL`, in
   which case the `Ini` struct will be initiated as empty.*/
void Ini_init(Ini *ini, char *content) {
  ini->cap = 8;
  ini->count = 0;
  ini->sections = calloc(sizeof(IniSection), ini->cap);

  if (content)
    _Ini_parse(ini, content);
}

// Print the contents of an Ini struct for debugging purposes.
void Ini_print(Ini *ini) {
  for (size_t i = 0; i < ini->count; i++) {
    printf("%s (%ld)\n", ini->sections[i].name, ini->sections[i].kv_count);
    for (size_t j = 0; j < ini->sections[i].kv_count; j++) {
      printf("  ");
      printf("%s: ", ini->sections[i].kv[j][0]);
      printf("%s", ini->sections[i].kv[j][1]);
      printf("\n");
    }
  }
}

#endif // INILITE_IMPLEMENTATION

/*

-------------------------------------------------------------------------------
References:

https://en.wikipedia.org/wiki/INI_file

-------------------------------------------------------------------------------
License:

Copyright © 2026 Simon Danielsson

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files, to deal in the Software
without restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/
