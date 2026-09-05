#include "main.h"
#define SDC_IMPLEMENTATION
#include "../../sdc/sdc.h"

#include <libc.h>
#include <pwd.h>

char *get_current_dir(void) {
    char *home_dir;
    {
        uid_t uid = getuid();
        struct passwd *info = getpwuid(uid);
        home_dir = info->pw_dir;
    }
    char curr_dir[128];
    getcwd(curr_dir, 128);

    if (strlen(curr_dir) < strlen(home_dir)) {
        return SDC_str_dup(curr_dir);
    } else {
        char *curr_dir_s = curr_dir;
        curr_dir_s += strlen(home_dir);
        char result[128];
        result[0] = '~';
        result[1] = '\0';
        strcat(result, curr_dir_s);
        return SDC_str_dup(result);
    }
}

#define COL_GRAY "\033[38;5;240m"
#define COL_ORANGE "\033[38;5;130m"
#define COL_GREEN "\033[38;5;35m"
#define COL_RESET "\033[0m"

int run_cmd(const char *cmd) {
    // char buffer[512];

    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        perror("popen");
        return 1;
    }

    // char tmp[512];
    //
    // while (fgets(buffer, sizeof(buffer), fp) != NULL)
    //     strcat(tmp, buffer);

    int status = pclose(fp);

    if (status == -1) {
        perror("pclose");
        return 1;
    }

    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    else
        return 1;

    // return SDC_str_dup(tmp);
}

bool dir_is_git_repo(void) {
    if (run_cmd("git rev-parse --is-inside-work-tree >/dev/null 2>&1") != 0)
        return false;
    return true;
}

bool is_dirty_git_repo(void) {
    if (run_cmd("git status --porcelain 2>/dev/null | grep -q .") == 0)
        return true;
    return false;
}

int main(void) {

    printf("\n");

    printf("%s", COL_GRAY);
    char *dir = get_current_dir();
    printf("%s", dir);
    free(dir);
    printf("%s", COL_RESET);

    if (dir_is_git_repo() && is_dirty_git_repo()) {
        printf("%s", COL_ORANGE);
        printf(" •");
        printf("%s", COL_RESET);

    } else if (dir_is_git_repo()) {
        printf("%s", COL_GREEN);
        printf(" •");
        printf("%s", COL_RESET);
    }

    printf(" → ");

    return 0;
}
