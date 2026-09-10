#include "main.h"
#define SDC_IMPLEMENTATION
#define INILITE_IMPLEMENTATION
#include "external/inilite.h"
#include "external/sdc.h"
#include <pwd.h>

#define config_filename ".tolk.ini"

char *get_path_home(void) {
    char *home_dir;
    {
        uid_t uid = getuid();
        struct passwd *info = getpwuid(uid);
        home_dir = info->pw_dir;
    }
    return SDC_str_dup(home_dir);
}

char *get_path_current(void) {
    char *home_dir = get_path_home();
    size_t home_dir_len = strlen(home_dir);
    free(home_dir);

    char curr_dir[128];
    getcwd(curr_dir, 128);

    if (strlen(curr_dir) < home_dir_len) {
        return SDC_str_dup(curr_dir);
    } else {
        char *curr_dir_s = curr_dir;
        curr_dir_s += home_dir_len;
        char result[128];
        result[0] = '~';
        result[1] = '\0';
        strcat(result, curr_dir_s);
        return SDC_str_dup(result);
    }
}

bool dir_is_git_repo(void) {
    if (SDC_io_cmd("git rev-parse --is-inside-work-tree >/dev/null 2>&1", NULL) !=
            0)
        return false;
    return true;
}

bool is_dirty_git_repo(void) {
    if (SDC_io_cmd("git status --porcelain 2>/dev/null | grep -q .", NULL) == 0)
        return true;
    return false;
}

// not in use right now
char *get_git_branch(void) {
    char buf[128] = {0};
    if (SDC_io_cmd("git rev-parse --abbrev-ref HEAD", buf) == 0) {
        SDC_str_trim(buf);
        return SDC_str_dup(buf);
    }
    return NULL;
}

typedef struct {
    uint8_t r, g, b;
} ColorRGB;

static int hexval(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

bool hex_to_u8(const char *s, uint8_t *out) {
    int hi, lo;
    if (!s || !out)
        return false;
    hi = hexval(s[0]);
    lo = hexval(s[1]);
    if (hi < 0 || lo < 0)
        return false;
    *out = (uint8_t)((hi << 4) | lo);
    return true;
}

// input must be formatted like "#xxxxxx"
bool ColorRGB_from_hex(ColorRGB *rgb, char *hex_in) {
    if (hex_in[0] != '#' || strlen(hex_in) != 7 || !rgb)
        return false;
    char *h = hex_in;
    h++;

    if (!hex_to_u8((char[]){h[0], h[1], 0}, &rgb->r))
        return false;
    if (!hex_to_u8((char[]){h[2], h[3], 0}, &rgb->g))
        return false;
    if (!hex_to_u8((char[]){h[4], h[5], 0}, &rgb->b))
        return false;

    return true;
}

void ColorRGB_print(ColorRGB *rgb) {
    printf("\033[38;2;%d;%d;%dm", rgb->r, rgb->g, rgb->b);
}

void ColorRGB_print_reset(void) { printf("\033[0m"); }

void config_use_defaults(Ini *ini) {
    Ini_append_section(ini, "colors");
    Ini_append_kv(ini, "git_dirty", "#af5f00");
    Ini_append_kv(ini, "git_clean", "#00af5f");
    Ini_append_kv(ini, "current_dir", "#5f5f63");
    Ini_append_kv(ini, "prompt", "#8c8f92");
    Ini_append_section(ini, "icons");
    Ini_append_kv(ini, "git_dirty", "•");
    Ini_append_kv(ini, "git_clean", "•");
    Ini_append_kv(ini, "prompt", "✽");
}

char *get_path_config(void) {
    char *home = get_path_home();
    char buff[512];
    snprintf(buff, 512, "%s/%s", home, config_filename);
    char *result = SDC_str_dup(buff);
    free(home);
    return result;
}

void config_get_ini(Ini *ini, Ini *ini_fallback) {
    char content[2048];
    char *config_dir = get_path_config();
    bool could_read_file = SDC_io_read_entire_file(content, config_dir);
    if (could_read_file) {
        Ini_init(ini, content);
    } else {
        Ini_init(ini, NULL);
        Ini_init(ini_fallback, NULL);
        config_use_defaults(ini_fallback);
    }
    free(config_dir);
}

void config_get_color(ColorRGB *rgb, IniSection *config, IniSection *fallback,
        const char *key) {
    char *color;
    if (!(color = IniSection_get_value(config, key)))
        color = IniSection_get_value(fallback, key);
    if (!ColorRGB_from_hex(rgb, color)) {
        fprintf(stderr, "Error(~/%s): failed to parse field '%s'", config_filename,
                key);
        rgb = 0;
    }
}

void config_get_icon(char **icon_buf, IniSection *config, IniSection *fallback,
        const char *key) {
    if (!(*icon_buf = IniSection_get_value(config, key)))
        *icon_buf = IniSection_get_value(fallback, key);
    if (!icon_buf)
        fprintf(stderr, "Error(~/%s): failed to parse field '%s'", config_filename,
                key);
}

int main(void) {
    Ini config = {0};
    Ini config_fallback = {0};
    config_get_ini(&config, &config_fallback);

    // Ini_print(&config_fallback);

    ColorRGB rgb_git_clean, rgb_git_dirty, rgb_curr_dir, rgb_prompt;
    {
        IniSection *colors = Ini_get_section(&config, "colors");
        IniSection *colors_fallback = Ini_get_section(&config_fallback, "colors");
        config_get_color(&rgb_git_clean, colors, colors_fallback, "git_clean");
        config_get_color(&rgb_git_dirty, colors, colors_fallback, "git_dirty");
        config_get_color(&rgb_curr_dir, colors, colors_fallback, "current_dir");
        config_get_color(&rgb_prompt, colors, colors_fallback, "prompt");
    }

    char *icon_git_dirty, *icon_git_clean, *icon_prompt;
    {
        IniSection *icons = Ini_get_section(&config, "icons");
        IniSection *icons_fallback = Ini_get_section(&config_fallback, "icons");
        config_get_icon(&icon_git_clean, icons, icons_fallback, "git_clean");
        config_get_icon(&icon_git_dirty, icons, icons_fallback, "git_dirty");
        config_get_icon(&icon_prompt, icons, icons_fallback, "prompt");
    }

    printf("\n");

    ColorRGB_print(&rgb_curr_dir);
    char *dir = get_path_current();
    printf("%s", dir);
    free(dir);
    ColorRGB_print_reset();

#define branch                                                                 \
    printf("%s", COL_GRAY);                                                      \
    char *branch = get_git_branch();                                             \
    printf(" (%s)", branch);                                                     \
    free(branch);                                                                \
    printf("%s", COL_RESET);

    if (dir_is_git_repo() && is_dirty_git_repo()) {
        // branch;
        ColorRGB_print(&rgb_git_dirty);
        printf(" %s", icon_git_dirty);
        ColorRGB_print_reset();

    } else if (dir_is_git_repo()) {
        // branch;
        ColorRGB_print(&rgb_git_clean);
        printf(" %s", icon_git_clean);
        ColorRGB_print_reset();
    }

#undef branch

    ColorRGB_print(&rgb_prompt);
    printf(" %s ", icon_prompt);
    ColorRGB_print_reset();

    Ini_free(&config);
    Ini_free(&config_fallback);
    return 0;
}
