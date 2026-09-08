/*

SDC (an abbreviation of "Simon Danielsson's C library")

A header-only C library with utilities frequently needed across my various C
projects.

Repository     https://github.com/simon-danielsson/sdc
Author         Simon Danielsson
Contact        contact@simondanielsson.se
License        MIT

See the end of this file for more information.

*/

#ifndef SDC_H_INCLUDE
#define SDC_H_INCLUDE

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#endif // SDC_H_INCLUDE

#ifdef SDC_IMPLEMENTATION

#define _SDC_internal static

// MATH = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

// Source: https://en.wikipedia.org/wiki/Feature_scaling
int SDC_math_min_max_rescale_value(const int old_value, const int old_min,
                                   const int old_max, const int new_min,
                                   const int new_max) {
  double new_value = ((old_value - old_min) / (double)(old_max - old_min));
  new_value *= (new_max - new_min);
  return (new_value + new_min);
}

// Source: https://en.wikipedia.org/wiki/Feature_scaling
double SDC_math_min_max_rescale_value_f(const double old_value,
                                        const double old_min,
                                        const double old_max,
                                        const double new_min,
                                        const double new_max) {
  double new_value = ((old_value - old_min) / (old_max - old_min));
  new_value *= (new_max - new_min);
  return (new_value + new_min);
}

// Source: https://stackoverflow.com/a/41871699
double SDC_math_floor(double num) {
  long n;
  double d;
  if (num >= (double)INT64_MIN || num <= (double)INT64_MIN || num != num) {
    return num;
  }
  n = (long)num;
  d = (double)n;
  if (d == num || num >= 0)
    return d;
  else
    return d - 1;
}

// Source: https://stackoverflow.com/a/16659263
double SDC_math_clamp(double d, double min, double max) {
  const double t = d < min ? min : d;
  return t > max ? max : t;
}

// Get the character width of an integer.
// Useful when working with CLI/TUI applications.
size_t SDC_math_char_width_of_int(int i) {
  size_t len;
  if (i == INT32_MIN)
    return 11;
  if (i < 0)
    i = -i;

  len = 1;
  for (; i >= 10; i /= 10)
    len++;
  return len;
}

// RAND = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

_SDC_internal uint32_t _SDC_rand_state;
_SDC_internal bool _SDC_rand_state_initialized = false;
#define _SDC_rand_seed                                                         \
  if (!_SDC_rand_state_initialized) {                                          \
    _SDC_rand_state = (unsigned long)&_SDC_rand_state_initialized;             \
    _SDC_rand_state_initialized = true;                                        \
  }

// Source: https://en.wikipedia.org/wiki/Xorshift#xoroshiro
_SDC_internal uint32_t _SDC_rand_xorshift(uint32_t *state) {
  _SDC_rand_seed;
  uint32_t x = *state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return *state = x;
}

bool SDC_rand_bool(void) { return _SDC_rand_xorshift(&_SDC_rand_state) % 2; }

_SDC_internal const uint32_t _SDC_rand_range_old_max = 1000 * 64;

// Quick and dirty XORshift followed by min-max rescaling.
int SDC_rand_range(const int floor, const int ceiling) {
  uint32_t rand =
      _SDC_rand_xorshift(&_SDC_rand_state) % _SDC_rand_range_old_max;
  return SDC_math_min_max_rescale_value((int)rand, 0, _SDC_rand_range_old_max,
                                        floor, ceiling);
}

// Quick and dirty XORshift followed by min-max rescaling.
double SDC_rand_range_f(const double floor, const double ceiling) {
  uint32_t rand =
      _SDC_rand_xorshift(&_SDC_rand_state) % _SDC_rand_range_old_max;
  return SDC_math_min_max_rescale_value_f((int)rand, 0, _SDC_rand_range_old_max,
                                          floor, ceiling);
}

// Get random value between 0 and RAND_MAX
int SDC_rand(void) {
  int r = (int)_SDC_rand_xorshift(&_SDC_rand_state);
  return SDC_math_clamp(r, 0, RAND_MAX);
}

// Source: https://stackoverflow.com/a/6127606
// Shuffle elements of a static array. Array needs to be cast to void.
// Example: SDC_rand_shuffle((void *)array, n, size);
void SDC_rand_shuffle(void *array, const size_t n, const size_t size) {
  char tmp[size], *arr = array;
  size_t stride = size * sizeof(char), i, j;

  if (n > 1) {
    for (i = 0; i < n - 1; ++i) {
      j = i + (size_t)SDC_rand() / (RAND_MAX / (n - i) + 1);

      memcpy(tmp, arr + j * stride, size);
      memcpy(arr + j * stride, arr + i * stride, size);
      memcpy(arr + i * stride, tmp, size);
    }
  }
}

// DYNAMIC ARRAY = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

typedef struct SDC_TYPE {
  enum {
    SDC_NULL,
    SDC_INT,
    SDC_STR,
    SDC_LONG,
    SDC_DOUBLE,
    SDC_FLOAT,
  } kind;
  union {
    int i;
    char *str;
    long l;
    double d;
    float f;
  } as;
} SDC_TYPE;

typedef struct {
  SDC_TYPE *items;
  size_t count, capacity;
} SDC_da;

void SDC_da_init(SDC_da *da) {
  da->capacity = 8;
  da->count = 0;
  da->items = malloc(sizeof(SDC_TYPE) * da->capacity);
}

/* Shortens the vector, keeping the first N items and removing the rest. Returns
   1 if index out of bounds.*/
int SDC_da_truncate(SDC_da *da, const size_t idx) {
  if (idx > da->count || idx < 0 || da->count == 0)
    return 1;
  da->count = idx;
  return 0;
}

void SDC_da_free(SDC_da *da) {
  if (!da)
    return;

  for (size_t i = 0; i < da->count; i++) {
    if (da->items[i].kind == SDC_STR)
      free(da->items[i].as.str);
  }

  free(da->items);

  da->items = NULL;
  da->count = 0;
  da->capacity = 0;
}

// Adds new item to the back, returns false if failure.
bool SDC_da_push(SDC_da *da, const SDC_TYPE item) {
  if (da->count == da->capacity) {
    size_t new_capacity = da->capacity * 2;
    void *new_items = realloc(da->items, new_capacity * sizeof(SDC_TYPE));
    if (new_items == NULL)
      return false;
    da->items = new_items;
    da->capacity = new_capacity;
  }
  da->items[da->count] = item;
  da->count++;
  return true;
}

/* Pops last item from array and returns its value in 'out' (if you don't need
   the popped item, provide NULL), returns false if failure. */
bool SDC_da_pop(SDC_da *da, SDC_TYPE *out) {
  if (da->count == 0)
    return false;
  da->count--;
  if (out != NULL)
    *out = da->items[da->count];
  return true;
}

/* Takes a stack static array and fills it with a copy of the dynamic array.
 * Returns false on failure.
 *
 * Example:
 * ``` c
 * SDC_TYPE static_arr[SDC_da_len(&da)];
 * size_t static_arr_len = SDC_da_len(&da);
 * SDC_da_copy_to_stack(&da, static_arr, static_arr_len);
 * // The dynamic array can be freed safely past this point if you want to
 * SDC_da_free(&da);
 *
 * for (size_t i = 0; i < static_arr_len; i++) {
 *     printf("%s\n", static_arr[i].as.str);
 * }
 * ``` */
bool SDC_da_copy_to_stack(SDC_da *da, SDC_TYPE new_array[],
                          const size_t new_array_len) {
  size_t i;
  if (da->count == 0 || da->count != new_array_len)
    return false;
  for (i = 0; i < da->count; i++)
    new_array[i] = da->items[i];
  return true;
}

/* Sort dynamic array using stdlib qsort with provided comparison function.
 * Returns false if failure.
 *
 * Example comparison function:
 * ``` c
 * int da_sort_int(const void *a, const void *b) {
 *   SDC_TYPE arg1 = *(const SDC_TYPE *)a;
 *   SDC_TYPE arg2 = *(const SDC_TYPE *)b;
 *
 *   if (arg1.as.i < arg2.as.i)
 *       return -1;
 *   if (arg1.as.i > arg2.as.i)
 *       return 1;
 *   return 0;
 * }
 * ```  */
bool SDC_da_qsort(SDC_da *da, int (*comp)(const void *, const void *)) {
  if (!da || !da->items || da->count == 0)
    return false;
  qsort(da->items, da->count, sizeof(SDC_TYPE), comp);
  return true;
}

/* Removes item at idx, shifting all items after it to the left. Returns false
 if failure. */
bool SDC_da_remove(SDC_da *da, const size_t idx) {
  size_t i;
  SDC_TYPE next;
  if (idx > da->count || idx < 0 || da->count == 0)
    return false;
  for (i = idx - 1; i < da->count; i++) {
    if (i + 1 < da->count) {
      next = da->items[i + 1];
      da->items[i] = next;
    }
  }
  da->count--;
  return true;
}

// Sugar function for retrieving current length of array.
size_t SDC_da_len(SDC_da *da) { return da->count; }

/* Inserts new item at idx, shifting all items after it to the right. Returns
 false if failure. */
int SDC_da_insert(SDC_da *da, const SDC_TYPE item, const size_t idx) {
  size_t i;
  SDC_TYPE prev;
  if (idx > da->count || idx < 0 || da->count == 0)
    return false;
  if (da->count == da->capacity) {
    size_t new_capacity = da->capacity * 2;
    void *new_items = realloc(da->items, new_capacity * sizeof(SDC_TYPE));
    if (new_items == NULL)
      return false;
    da->items = new_items;
    da->capacity = new_capacity;
  }

  for (i = da->count; i >= idx; i--) {
    if (i >= idx) {
      prev = da->items[i - 1];
      da->items[i] = prev;
    }
  }
  da->items[idx - 1] = item;

  da->count++;

  return true;
}

// STRINGS = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

// Returns NULL if failure
char *SDC_str_dup(const char *s) {
  char *out;
  if (!s)
    return NULL;
  out = malloc(strlen(s) + 1);
  if (!out)
    return NULL;
  strcpy(out, s);
  return out;
}

// Source: https://www.geeksforgeeks.org/dsa/xor-cipher/
/* XOR String obfuscation (self-reversible). Note that this doesn't offer
  any proper security - it's only good for obfuscating text for human eyes. */
void SDC_str_obf(char *s) {
  char key = 'P';
  int len = strlen(s), i;
  for (i = 0; i < len; i++)
    s[i] = s[i] ^ key;
}

/* Replaces all occurences of a substring in a string with a new one. Returns
   an allocated string. Returns NULL on failure. */
char *SDC_str_replace_substr(const char *s, const char *sub,
                             const char *new_sub) {
  if (!s || !sub || !new_sub)
    return NULL;
#define _SDC_str_replace_substr_tmp_cap 2048

#define _SDC_str_replace_substr_append_to_tmp(s)                               \
  if (tmp_len + 1 != _SDC_str_replace_substr_tmp_cap) {                        \
    tmp[tmp_len++] = (s);                                                      \
  } else {                                                                     \
    return NULL;                                                               \
  }

  size_t sub_len = strlen(sub), i, tmp_len = 0;
  char tmp[_SDC_str_replace_substr_tmp_cap] = {0}, *pos = strstr(s, sub),
       *last_pos;

  for (i = 0; i < (size_t)(pos - s); i++)
    _SDC_str_replace_substr_append_to_tmp(s[i]);
  for (i = 0; new_sub[i] != '\0'; i++)
    _SDC_str_replace_substr_append_to_tmp(new_sub[i]);

  for (;;) {
    last_pos = pos;
    pos = strstr(s + ((size_t)(last_pos - s) + sub_len), sub);

    if (pos) {
      for (i = (size_t)(last_pos - s) + sub_len; i < (size_t)(pos - s); i++)
        _SDC_str_replace_substr_append_to_tmp(s[i]);
      for (i = 0; new_sub[i] != '\0'; i++)
        _SDC_str_replace_substr_append_to_tmp(new_sub[i]);
      continue;
    } else {
      for (i = (size_t)last_pos - (size_t)s + sub_len; s[i] != '\0'; i++)
        _SDC_str_replace_substr_append_to_tmp(s[i]);
    }
    break;
  }
  if (tmp_len == 0)
    return NULL;
  tmp[tmp_len] = '\0';
  return SDC_str_dup(tmp);
}

/*
 * Takes an `SDC_da` and fills it with the splits of an input string. (Note that
 * `SDC_da_init()` is called inside this function and should not be executed
 * by its caller).
 *
 * Example:
 * ``` c
 * char *s = "This is a string that I want to split by whitespace.";
 * SDC_da da = {0};
 * SDC_str_split_by_delim(&da, s, strlen(s), ' ');
 *
 * for (size_t i = 0; i < SDC_da_len(&da); i++) {
 *     SDC_str_remove_special_chars(da.items[i].as.str);
 *     printf("%s\n", da.items[i].as.str);
 * }
 * SDC_da_free(&da);
 * ```
 */
bool SDC_str_split_by_delim(SDC_da *da, const char *input,
                            const size_t input_len, const char delim) {

  if (!da)
    return false;

  SDC_da_init(da);

  for (size_t pos = 0; pos < input_len;) {
    size_t start = pos;

    if (input[pos] == delim) {
      pos++;
      continue;
    }

    while (pos < input_len && input[pos] != delim)
      pos++;

    size_t segm_len = pos - start;
    if (segm_len > 0) {
      char s[segm_len + 1];
      memcpy(s, input + start, segm_len);
      s[segm_len] = '\0';
      if (!SDC_da_push(da,
                       (SDC_TYPE){.as.str = SDC_str_dup(s), .kind = SDC_STR})) {
        SDC_da_free(da);
        return false;
      };
    }
  }
  return true;
}

// Un-capitalize entire string
void SDC_str_lower(char *s) {
  for (char *c = s; *c; c++)
    *c = tolower(*c);
}

// Capitalize entire string
void SDC_str_upper(char *s) {
  for (char *c = s; *c; c++)
    *c = toupper(*c);
}

void SDC_str_remove_special_chars(char *str) {
  char *dst = str;
  while (*str) {
    if (isalnum((unsigned char)*str) || *str == '_')
      *dst++ = *str;
    str++;
  }
  *dst = '\0';
}

// Append a single char to a string in-place.
void SDC_str_append_char(char *s, char c) {
  int len = strlen(s);
  s[len] = c;
  s[len + 1] = '\0';
}

// Trims whitespace, newlines etc. at beginning and end of string in-place.
void SDC_str_trim(char *s) {
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

// Returns true if `s` starts with `start`.
bool SDC_str_starts_with(const char *s, const char *start) {
  for (int i = 0; start[i] != '\0'; i++) {
    if (s[i] != start[i] || s[i] == '\0')
      return false;
  }
  return true;
}

/* If `s` ends with `end`: returns index starting pos of `end` in `s`.
 * Else returns -1. */
int SDC_str_ends_with(const char *s, const char *end) {
  size_t len_s = strlen(s), len_end = strlen(end);
  if (len_end > len_s)
    return -1;
  if (strncmp(s + (len_s - len_end), end, len_end) == 0)
    return len_s - len_end;
  return -1;
}

// Remove first N chars from string.
void SDC_str_remove_first_n(char *s, int n) {
  int len = strlen(s);
  if (n >= len)
    s[0] = '\0';
  else
    memmove(s, s + n, len - n + 1);
}

// Returns true if string is effectively empty.
// i.e "  \n  " or "\t "
bool SDC_str_is_empty(const char *s) {
  for (int i = 0; s[i] != '\0'; i++) {
    if (isalpha(s[i]))
      return false;
  }
  return true;
}

// Replaces all occurences `a` in `s` with `b`
void SDC_str_replace_occ(char *s, const char a, const char b) {
  for (char *c = s; *c; c++)
    (*c == a) ? *c = b : a;
}

// IO = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

#define _SDC_io_read_buffer_kb 2048 * 100

/* Reads file into provided buffer; stack-allocated (max 2MB). Silently stops
  reading into buffer without throwing an error if stack-limit is reached.
  Returns 1 on failure. */
bool SDC_io_read_entire_file(char *buffer, const char *path) {
  int c, len = 0;
  char tmp[_SDC_io_read_buffer_kb];
  FILE *f = fopen(path, "rb");
  if (!f)
    return false;
  while ((c = fgetc(f)) != EOF) {
    if (len >= _SDC_io_read_buffer_kb)
      break;
    tmp[len++] = c;
  }
  tmp[len] = '\0';
  strncpy(buffer, tmp, len + 1);
  fclose(f);
  return true;
}

/* Reads user input into buffer with a simple prompt, finish with newline.
   If you don't want a prompt header, provide NULL. */
void SDC_io_prompt(char **buffer, const char *prompt_header) {
  char tmp[96] = {0};
  int c_count = 0, ch;
  if (prompt_header != NULL)
    printf("%s\n", prompt_header);
  printf("> ");
  while ((ch = getchar()) != EOF) {
    if (ch == '\n') {
      tmp[c_count] = '\0';
      SDC_str_trim(tmp);
      break;
    }
    tmp[c_count] = ch;
    c_count++;
  }
  *buffer = tmp;
}

/* Takes `cmd` string and `sout` stdout buffer. `sout` can be NULL if you don't
   want to read stdout. Returns exit code. */
_SDC_internal int _SDC_io_cmd(const char *cmd, char *sout) {
  if (!cmd)
    return 1;

  FILE *fp = popen(cmd, "r");
  if (fp == NULL) {
    perror("popen");
    return 1;
  }

  if (sout) {
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
      strcat(sout, buffer);
  }

  int status = pclose(fp);

  if (status == -1) {
    perror("pclose");
    return 1;
  }

  if (WIFEXITED(status))
    return WEXITSTATUS(status);
  else
    return 1;
}

#ifdef _WIN32
#error "SDC_io_cmd: windows not supported"
#elif defined(__linux__) || defined(__APPLE__)
int SDC_io_cmd(const char *cmd, char *sout) { return _SDC_io_cmd(cmd, sout); }
#else
#error "SDC_io_cmd: unsupported platform"
#endif

/*
 * Example:
 * ``` c
 * int main(void) {
 *     SDC_io_LoadingBar bar = {.ch_fill = '#',
 *         .ch_empty = '.',
 *         .msg = "The process has started...",
 *         .prog_start = 0,
 *         .progress = 0,
 *         .prog_end = 100,
 *         ._t_start = -1};
 *
 *     while (bar.progress <= bar.prog_end) {
 *         SDC_io_LoadingBar_draw(&bar);
 *         if (bar.progress > 50)
 *             SDC_io_LoadingBar_msg(&bar, "Almost there!");
 *         sleep(1);
 *         bar.progress += SDC_rand_range(1, 18);
 *     }
 *     strcpy(bar.msg, "Finished!");
 *     SDC_io_LoadingBar_draw(&bar);
 *     return 0;
 * } ```
 */
typedef struct {
  time_t _t_start;
  int prog_start, prog_end, progress;
  char ch_fill, ch_empty, msg[128];
} SDC_io_LoadingBar;

void SDC_io_LoadingBar_draw(SDC_io_LoadingBar *bar) {
  int cols = -1, i, prog_norm, bar_size;
  char label[256];

  // build label
  {
    time_t elapsed;
    struct tm *t;

    if (bar->_t_start == -1) {
      bar->_t_start = time(NULL);
      elapsed = 0;
    } else {
      elapsed = time(NULL) - bar->_t_start;
    }
    t = gmtime(&elapsed);

    if (bar->progress > bar->prog_end)
      bar->progress = bar->prog_end;

    if (strlen(bar->msg) > 1 || !SDC_str_is_empty(bar->msg))
      snprintf(label, 256, "%02d:%02d:%02d • %s ", t->tm_hour, t->tm_min,
               t->tm_sec, bar->msg);
    else
      snprintf(label, 256, "%d/%d • %02d:%02d:%02d", bar->progress,
               bar->prog_end, t->tm_hour, t->tm_min, t->tm_sec);
  }

  { // get terminal columns
    // Source: https://codereview.stackexchange.com/q/292621
#if defined(TIOCGWINSZ)
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0)
      cols = ws.ws_col;
#elif defined(TIOCGSIZE)
    struct ttysize ts;
    if (ioctl(STDOUT_FILENO, TIOCGSIZE, &ts) == 0)
      cols = ws.ws_col;
#endif // defined(TIOCGWINSZ)

    if (cols == -1)
      return;
  }

  bar_size = cols - (int)strlen(label) - 2;

  prog_norm = SDC_math_min_max_rescale_value(bar->progress, bar->prog_start,
                                             bar->prog_end, 0, bar_size);
  printf("%s", label);
  printf("[");
  for (i = 0; i < bar_size; i++) {
    if (i < prog_norm) {
      printf("%c", bar->ch_fill);
    } else if (i % 2 == 0) {
      printf("%c", bar->ch_empty);
    } else {
      printf(" ");
    }
  }
  printf("]");
  printf("\r");
  fflush(stdout);
}

void SDC_io_LoadingBar_msg(SDC_io_LoadingBar *bar, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vsnprintf(bar->msg, sizeof bar->msg, fmt, args);
  va_end(args);
}

#if defined(__linux__) || defined(__APPLE__)
#else
#error "SDC_io_cmd: unsupported platform"
#endif

// HASH TABLE = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

typedef struct _SDC_BucketNode _SDC_BucketNode;
struct _SDC_BucketNode {
  char *key;
  SDC_TYPE value;
  _SDC_BucketNode *next;
};

_SDC_internal _SDC_BucketNode *_SDC_BucketNode_new(const char *key,
                                                   const SDC_TYPE *value) {
  _SDC_BucketNode *n = malloc(sizeof *n);
  if (n == NULL)
    return NULL;

  n->key = SDC_str_dup(key);
  if (n->key == NULL) {
    free(n);
    return NULL;
  }

  n->value.kind = value->kind;

  if (value->kind == SDC_STR) {
    n->value.as.str = SDC_str_dup(value->as.str);
    if (n->value.as.str == NULL) {
      free(n->key);
      free(n);
      return NULL;
    }
  } else {
    n->value.as = value->as;
  }

  n->next = NULL;
  return n;
}

typedef struct {
  _SDC_BucketNode *n;
} _SDC_Bucket;

#define _SDC_HashTable_len 200

typedef struct {
  _SDC_Bucket buckets[_SDC_HashTable_len];
  size_t buckets_len;
} SDC_HashTable;

int SDC_HashTable_init(SDC_HashTable *ht) {
  size_t i;

  ht->buckets_len = _SDC_HashTable_len;

  for (i = 0; i < ht->buckets_len; i++)
    ht->buckets[i].n = NULL;

  return 0;
}

_SDC_internal void _SDC_Bucket_insert(_SDC_Bucket *b, const char *key,
                                      SDC_TYPE *value) {
  // TODO: insert should double as a replace
  _SDC_BucketNode *bn = _SDC_BucketNode_new(key, value);
  _SDC_BucketNode *last;
  if (b->n == NULL) {
    b->n = bn;
    return;
  }
  last = b->n;
  while (last->next != NULL)
    last = last->next;
  last->next = bn;
}

_SDC_internal void _SDC_Bucket_free(_SDC_Bucket *b) {
  _SDC_BucketNode *current = b->n;

  while (current != NULL) {
    _SDC_BucketNode *next = current->next;

    if (current->value.kind == SDC_STR)
      free(current->value.as.str);

    free(current->key);
    free(current);
    current = next;
  }

  b->n = NULL;
}

void SDC_HashTable_free(SDC_HashTable *ht) {
  size_t i;
  for (i = 0; i < ht->buckets_len; i++) {
    _SDC_Bucket_free(&ht->buckets[i]);
  }
}

#define _SDC_FNV_offset_basis 0xcbf29ce484222325
#define _SDC_FNV_prime 0x100000001b3

_SDC_internal unsigned long _SDC_fnv_hash(const char *ip,
                                          const size_t array_len) {
  size_t i, ip_len = strlen(ip);
  unsigned char byte_of_data;
  unsigned long hash = (unsigned long)_SDC_FNV_offset_basis;
  for (i = 0; i < ip_len; i++) {
    byte_of_data = (unsigned)ip[i];
    hash *= _SDC_FNV_prime;
    hash ^= (unsigned long)byte_of_data;
  }
  return hash % array_len;
}

bool SDC_HashTable_contains_key(SDC_HashTable *ht, const char *key) {
  unsigned long idx = _SDC_fnv_hash(key, ht->buckets_len);
  _SDC_BucketNode *current;
  {
    if (ht->buckets[idx].n == NULL)
      return false;
    current = ht->buckets[idx].n;
    while (current != NULL) {
      if (strcmp(current->key, key) == 0)
        return true;
      current = current->next;
    }
    return false;
  }
}

// Returns false on failure
bool SDC_HashTable_insert(SDC_HashTable *ht, const char *key, SDC_TYPE *value) {
  unsigned long idx = _SDC_fnv_hash(key, ht->buckets_len);
  _SDC_BucketNode *bn, *last;
  if (SDC_HashTable_contains_key(ht, key))
    return false;
  {
    // TODO: insert should double as a replace
    bn = _SDC_BucketNode_new(key, value);
    if (ht->buckets[idx].n == NULL) {
      ht->buckets[idx].n = bn;
      return true;
    }
    last = ht->buckets[idx].n;
    while (last->next != NULL)
      last = last->next;
    last->next = bn;
  }
  return true;
}

/* Returns a shallow copy of SDC_TYPE. If not found, returns SDC_TYPE of kind
 'SDC_NULL' */
SDC_TYPE SDC_HashTable_get_value_by_key(SDC_HashTable *ht, const char *key) {
  unsigned long idx = _SDC_fnv_hash(key, ht->buckets_len);
  _SDC_BucketNode *current = ht->buckets[idx].n;

  while (current != NULL) {
    if (strcmp(current->key, key) == 0)
      return current->value;

    current = current->next;
  }

  return (SDC_TYPE){.kind = SDC_NULL};
}

// Returns false on failure
bool SDC_HashTable_remove(SDC_HashTable *ht, const char *key) {
  unsigned long idx = _SDC_fnv_hash(key, ht->buckets_len);
  _SDC_BucketNode *current, *prev;
  {
    if (ht->buckets[idx].n == NULL)
      return false;

    current = ht->buckets[idx].n;
    prev = NULL;

    while (current != NULL) {
      if (strcmp(current->key, key) == 0) {
        if (prev == NULL) {
          ht->buckets[idx].n = current->next;
        } else {
          prev->next = current->next;
        }
        free(current->key);
        free(current);
        return true;
      }
      prev = current;
      current = current->next;
    }

    return false;
  }
}

// Modify/replace value by key, returns false on failure
bool SDC_HashTable_modify(SDC_HashTable *ht, const char *key,
                          const SDC_TYPE *new_value) {
  unsigned long idx = _SDC_fnv_hash(key, ht->buckets_len);
  _SDC_BucketNode *current = ht->buckets[idx].n;

  while (current != NULL) {
    if (strcmp(current->key, key) == 0) {
      SDC_TYPE replacement;
      replacement.kind = new_value->kind;
      if (new_value->kind == SDC_STR) {
        replacement.as.str = SDC_str_dup(new_value->as.str);
        if (replacement.as.str == NULL)
          return false;
      } else {
        replacement.as = new_value->as;
      }
      if (current->value.kind == SDC_STR)
        free(current->value.as.str);
      current->value = replacement;

      return true;
    }
    current = current->next;
  }
  return false;
}

typedef struct SDC_KV {
  char *key;
  SDC_TYPE val;
} SDC_KV;

#define _SDC_HashTable_max_entries 10000

void SDC_KV_array_free(SDC_KV *array, const size_t array_len) {

  if (array == NULL)
    return;

  for (size_t i = 0; i < array_len; i++) {
    free(array[i].key);
  }

  free(array);
}

/*
 * If successful, returns a new allocated array SDC_KV* and
 * frees the memory of the input HashTable. Returns NULL on
 * allocation failure. The returned array must be freed with
 * SDC_KV_array_free(). Unused elements have key == NULL and val ==
 * NULL. Takes an 'out_len' which recieves the length of outputted array.
 *
 * Example:
 * ``` c
 * size_t array_len;
 * SDC_KV *array = SDC_HashTable_reduce_to_array(&ht, &array_len);
 *
 * if (array == NULL)
 *     err("Failed to reduce table");
 *
 * for (size_t i = 0; i < array_len; i++)
 *     printf("%s: %d\n", array[i].key, array[i].val.as.i);
 *
 * SDC_HashTable_free(&ht);
 * SDC_KV_array_free(array, array_len);
 * ```
 */
SDC_KV *SDC_HashTable_reduce_to_array(SDC_HashTable *ht, size_t *out_len) {
  size_t i;
  size_t array_idx = 0;

  if (!ht || !out_len)
    return NULL;

  SDC_KV tmp[_SDC_HashTable_max_entries];

  for (i = 0; i < ht->buckets_len; i++) {
    _SDC_BucketNode *current = ht->buckets[i].n;

    while (current != NULL) {
      if (array_idx >= _SDC_HashTable_max_entries) {
        return NULL;
      }
      if (current->value.kind != SDC_INT) {
        return NULL;
      }
      tmp[array_idx].key = current->key;
      tmp[array_idx].val = current->value;
      array_idx++;
      current = current->next;
    }
  }
  *out_len = array_idx;

  SDC_KV *result;
  result = calloc(array_idx + 1, sizeof(*result));
  for (i = 0; i < array_idx; i++) {
    result[i].key = SDC_str_dup(tmp[i].key);
    result[i].val = tmp[i].val;
  }

  SDC_HashTable_free(ht);

  return result;
}

// ARENA ALLOCATOR  = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

_SDC_internal bool _SDC_Arena_is_power_of_two(uintptr_t x) {
  return (x & (x - 1)) == 0;
}

_SDC_internal uintptr_t _SDC_Arena_align_forward(uintptr_t ptr, size_t align) {
  uintptr_t p, a, modulo;

  assert(_SDC_Arena_is_power_of_two(align));

  p = ptr;
  a = (uintptr_t)align;
  // Same as (p % a) but faster as 'a' is a power of two
  modulo = p & (a - 1);

  if (modulo != 0) {
    // If 'p' address is not aligned, push the address to the
    // next value which is aligned
    p += a - modulo;
  }
  return p;
}

#define _SDC_Arena_default_alignment (2 * sizeof(void *))

/* Regarding SDC_Arena:
 * The credit for all the arena code goes to Ginger Bill.
 * The original source code was taken from:
 * https://www.gingerbill.org/article/2019/02/08/memory-allocation-strategies-002/
 *
 * Here the example code for this Arena implementation taken from the blog post
 * in the link above:
 *
 * ``` c
 * int main(void) {
 *
 *     // if you want the heap
 *     void *backing_buffer = malloc(256);
 *     // if you want the stack
 *     // unsigned char backing_buffer[256];
 *     SDC_Arena a = {0};
 *     SDC_Arena_init(&a, backing_buffer, 256);
 *
 *     for (int i = 0; i < 80; i++) {
 *         int *x;
 *         float *f;
 *         char *str;
 *
 *         // Reset all arena offsets for each loop
 *         SDC_Arena_free_all(&a);
 *
 *         x = (int *)SDC_Arena_alloc(&a, sizeof(int));
 *         f = (float *)SDC_Arena_alloc(&a, sizeof(float));
 *         str = SDC_Arena_alloc(&a, 8);
 *
 *         *x = 123;
 *         *f = 987;
 *         memmove(str, "Hellope", 7);
 *
 *         printf("%p: %d\n", (void *)x, *x);
 *         printf("%p: %f\n", (void *)f, *f);
 *         printf("%p: %s\n", str, str);
 *
 *         str = SDC_Arena_resize(&a, str, 10, 14);
 *         memmove(str + 7, " world!", 7);
 *         printf("%p: %s\n", str, str);
 *     }
 *
 *     SDC_Arena_free_all(&a);
 *
 *     return 0;
 * }
 * ```
 */
typedef struct SDC_Arena SDC_Arena;
struct SDC_Arena {
  unsigned char *buf;
  size_t buf_len;
  size_t prev_offset;
  size_t curr_offset;
};

void SDC_Arena_init(SDC_Arena *a, void *backing_buffer,
                    size_t backing_buffer_length) {
  a->buf = (unsigned char *)backing_buffer;
  a->buf_len = backing_buffer_length;
  a->curr_offset = 0;
  a->prev_offset = 0;
}

_SDC_internal void *_SDC_Arena_alloc_align(SDC_Arena *a, size_t size,
                                           size_t align) {
  uintptr_t curr_ptr = (uintptr_t)a->buf + (uintptr_t)a->curr_offset;
  uintptr_t offset = _SDC_Arena_align_forward(curr_ptr, align);
  offset -= (uintptr_t)a->buf; // Change to relative offset

  if (offset + size <= a->buf_len) {
    void *ptr = &a->buf[offset];
    a->prev_offset = offset;
    a->curr_offset = offset + size;

    memset(ptr, 0, size);
    return ptr;
  }
  return NULL;
}

_SDC_internal void *SDC_Arena_alloc(SDC_Arena *a, size_t size) {
  return _SDC_Arena_alloc_align(a, size, _SDC_Arena_default_alignment);
}

_SDC_internal void *_SDC_Arena_resize_align(SDC_Arena *a, void *old_memory,
                                            size_t old_size, size_t new_size,
                                            size_t align) {
  unsigned char *old_mem = (unsigned char *)old_memory;

  assert(_SDC_Arena_is_power_of_two(align));

  if (old_mem == NULL || old_size == 0) {
    return _SDC_Arena_alloc_align(a, new_size, align);
  } else if (a->buf <= old_mem && old_mem < a->buf + a->buf_len) {
    if (a->buf + a->prev_offset == old_mem) {
      a->curr_offset = a->prev_offset + new_size;
      if (new_size > old_size) {
        memset(&a->buf[a->curr_offset], 0, new_size - old_size);
      }
      return old_memory;
    } else {
      void *new_memory = _SDC_Arena_alloc_align(a, new_size, align);
      size_t copy_size = old_size < new_size ? old_size : new_size;
      memmove(new_memory, old_memory, copy_size);
      return new_memory;
    }

  } else {
    return NULL;
  }
}

void *SDC_Arena_resize(SDC_Arena *a, void *old_memory, size_t old_size,
                       size_t new_size) {
  return _SDC_Arena_resize_align(a, old_memory, old_size, new_size,
                                 _SDC_Arena_default_alignment);
}

void SDC_Arena_free_all(SDC_Arena *a) {
  a->curr_offset = 0;
  a->prev_offset = 0;
}

#endif // SDC_IMPLEMENTATION

/*

-------------------------------------------------------------------------------
Revision history:

    2026-09-07  String
    ----------
                * Strings
                    - Refactor: remove a few calls to strlen
                    - SDC_str_replace_occ(char *s, const char a, const char b)

    2026-09-06  Strings, IO
    ----------
                * IO
                    - SDC_io_cmd(const char *cmd, char *sout)
                        - Only support for unix as of now.
                    - Loading bar implementation
                        - Only support for unix as of now.

                * Strings
                    - simplification: void SDC_str_obf(char *in)

    2026-09-05  Strings
    ----------
                * Strings
                    - SDC_str_obf(char *out, const char *in)
                    - void SDC_str_obf(char *in)

    2026-09-02  Strings
    ----------
                * Strings
                    - SDC_str_replace_substr(const char *s, const char *s...
                    - SDC_str_ends_with(const char *s, const char *suffix)
                    - Better naming scheme for starts-with/ends-with functions

                * Dyn arrays
                    - Fix memory leak occuring in void SDC_da_free(SDC_da *da)

    2026-09-01  Dynamic Array, Arena
    ----------
                * Strings
                    - SDC_str_split_by_delim() modified to use SDC_da

                * Dyn arrays
                    - Sorting function SDC_da_qsort(SDC_da *da, int (*comp...

                * Arena
                    - Steal implementation from Ginger Bill

    2026-08-31  Refactor, Strings
    ----------
                * C standard switch
                    - Move from C89 to C99 to facilitate compound literals.
                    - Re-style comments
                * Strings
                    - SDC_str_remove_special_characters()
                    - SDC_str_upper()
                    - SDC_str_lower()
                    - SDC_str_split_by_delim()
                * Hashtable
                    - Move from void* memory model to unions
                    - SDC_HashTable_reduce_to_array()
                * Dyn arrays
                    - Move from void* memory model to unions
                * Rand
                    - Change naming scheme
                    - SDC_rand_shuffle()
                    - SDC_rand()

    2026-08-30  IO, strings, rand, math
    ----------
                * Math
                    - New function for min-max range scaling.
                    - New function for clamping.
                    - New function for flooring.
                    - New function to get the char width of an integer.
                * Rand
                    - New function to get random bool.
                    - New function to get random range.
                * IO
                    - New function to read entire file.
                    - New function for simple user prompt.
                * Strings
                    - New function to check if string effectively empty.
                    - New function for appending a char to the end of a string.

    2026-08-29  Initial commit
    ----------
                * Hashtable
                    - Owns the memory of both keys and values.
                    - Hash function implemented with Fowler–Noll–Vo.
                * Dynamic array
                * Various string functions
                    - Func to dup. string.
                    - Func to trim beg. and end of string in-place.
                    - Func to check if a str starts with another.

-------------------------------------------------------------------------------
References:

https://en.wikipedia.org/wiki/Hash_table
https://www.w3schools.com/dsa/dsa_theory_hashtables.php
https://doc.rust-lang.org/std/collections/struct.HashMap.html
https://www.masaischool.com/blog/understanding-hashmap-data-structure-with-examples/
https://www.geeksforgeeks.org/dsa/singly-linked-list-tutorial/
https://www.w3schools.com/dsa/dsa_algo_linkedlists_operations.php
https://www.gingerbill.org/article/2019/02/08/memory-allocation-strategies-002/
https://www.bytesbeneath.com/p/the-arena-custom-memory-allocators
https://en.wikipedia.org/wiki/Non-cryptographic_hash_function
https://stackoverflow.com/questions/15821123/removing-elements-from-an-array-in-c
https://doc.rust-lang.org/std/vec/struct.Vec.html#method.remove
https://stackoverflow.com/questions/779875/what-function-is-to-replace-a-substring-from-a-string-in-c
https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
https://www.geeksforgeeks.org/c/bitwise-operators-in-c-cpp/
https://en.wikipedia.org/wiki/Xorshift#xoroshiro
https://en.wikipedia.org/wiki/Feature_scaling
https://www.cs.yale.edu/homes/aspnes/pinewiki/FrontPage.html

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
