#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>

#define COLOR_DIR  "\033[01;34m"
#define COLOR_EXEC "\033[01;32m"
#define COLOR_LINK "\033[01;36m"
#define COLOR_RESET "\033[0m"

int show_all = 0;    //  -a
int long_format = 0; //  -l

typedef struct
{
    char name[1024];
    char link_target[1024];
    struct stat st;
} FileEntry;

int compare_names(const void *a, const void *b)
{
    return strcmp(((FileEntry *)a)->name, ((FileEntry *)b)->name);
}

char *get_username(uid_t uid)
{
    struct passwd *pw = getpwuid(uid);
    if (pw)
        return pw->pw_name;
    static char buf[32];
    snprintf(buf, sizeof(buf), "%d", (int)uid);
    return buf;
}

char *get_groupname(gid_t gid)
{
    struct group *gr = getgrgid(gid);
    if (gr)
        return gr->gr_name;
    static char buf[32];
    snprintf(buf, sizeof(buf), "%d", (int)gid);
    return buf;
}

void format_time(time_t t, char *buf)
{
    time_t now = time(NULL);
    double diff = difftime(now, t);
    struct tm *tm = localtime(&t);

    if (diff > 15552000 || diff < -15552000)
    {
        strftime(buf, 20, "%b %d  %Y", tm);
    }
    else
    {
        strftime(buf, 20, "%b %d %H:%M", tm);
    }
}

void print_long_entry(FileEntry *entry)
{
    char time_buf[20];
    mode_t mode = entry->st.st_mode;

    char perms[11];
    perms[0] = (S_ISDIR(mode)) ? 'd' : (S_ISLNK(mode)) ? 'l' : '-';
    perms[1] = (mode & S_IRUSR) ? 'r' : '-';
    perms[2] = (mode & S_IWUSR) ? 'w' : '-';
    perms[3] = (mode & S_IXUSR) ? 'x' : '-';
    perms[4] = (mode & S_IRGRP) ? 'r' : '-';
    perms[5] = (mode & S_IWGRP) ? 'w' : '-';
    perms[6] = (mode & S_IXGRP) ? 'x' : '-';
    perms[7] = (mode & S_IROTH) ? 'r' : '-';
    perms[8] = (mode & S_IWOTH) ? 'w' : '-';
    perms[9] = (mode & S_IXOTH) ? 'x' : '-';
    perms[10] = '\0';

    printf("%s", perms);
    printf(" %2ld", (long)entry->st.st_nlink);

    char *user = get_username(entry->st.st_uid);
    char *group = get_groupname(entry->st.st_gid);
    printf(" %s %s", user, group);

    printf(" %7ld ", (long)entry->st.st_size);

    format_time(entry->st.st_mtime, time_buf);
    printf("%s ", time_buf);

    char *color = "";
    if (S_ISDIR(mode))
        color = COLOR_DIR;
    else if (S_ISLNK(mode))
        color = COLOR_LINK;
    else if (mode & S_IXUSR || mode & S_IXGRP || mode & S_IXOTH)
        color = COLOR_EXEC;

    printf("%s%s%s", color, entry->name, COLOR_RESET);

    if (S_ISLNK(mode) && entry->link_target[0] != '\0')
    {
        printf(" -> %s%s%s", COLOR_LINK, entry->link_target, COLOR_RESET);
    }

    printf("\n");
}

int process_directory(const char *path, int print_header)
{
    DIR *dir = opendir(path);
    if (!dir)
    {
        perror(path);
        return 1;
    }

    if (print_header)
    {
        printf("%s:\n", path);
    }

    FileEntry *entries = NULL;
    int count = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (!show_all && entry->d_name[0] == '.')
            continue;

        FileEntry fe;
        strncpy(fe.name, entry->d_name, sizeof(fe.name) - 1);
        fe.name[sizeof(fe.name) - 1] = '\0';
        fe.link_target[0] = '\0';

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (lstat(full_path, &fe.st) == -1)
        {
            continue;
        }

        if (S_ISLNK(fe.st.st_mode))
        {
            ssize_t len = readlink(full_path, fe.link_target, sizeof(fe.link_target) - 1);
            if (len != -1)
            {
                fe.link_target[len] = '\0';
            }
        }

        entries = realloc(entries, (count + 1) * sizeof(FileEntry));
        if (!entries)
        {
            perror("realloc");
            closedir(dir);
            return 1;
        }
        entries[count++] = fe;
    }

    closedir(dir);

    if (count == 0)
    {
        free(entries);
        return 0;
    }

    qsort(entries, count, sizeof(FileEntry), compare_names);

    if (long_format)
    {
        long long total_blocks = 0;
        for (int i = 0; i < count; i++)
        {
            total_blocks += entries[i].st.st_blocks;
        }
        printf("total %lld\n", total_blocks);

        for (int i = 0; i < count; i++)
        {
            print_long_entry(&entries[i]);
        }
    }
    else
    {
        int terminal_width = 80;
        int max_len = 0;
        for (int i = 0; i < count; i++)
        {
            int len = strlen(entries[i].name);
            if (len > max_len) max_len = len;
        }
        int col_width = max_len + 2;
        int cols = (col_width > 0) ? terminal_width / col_width : 1;
        if (cols < 1) cols = 1;

        for (int i = 0; i < count; i++)
        {
            char *color = "";
            mode_t mode = entries[i].st.st_mode;
            if (S_ISDIR(mode))
                color = COLOR_DIR;
            else if (S_ISLNK(mode))
                color = COLOR_LINK;
            else if (mode & S_IXUSR || mode & S_IXGRP || mode & S_IXOTH)
                color = COLOR_EXEC;

            printf("%s%s%s", color, entries[i].name, COLOR_RESET);
            if (i < count - 1)
            {
                if ((i + 1) % cols == 0)
                    printf("\n");
                else
                    printf("  ");
            }
        }
        printf("\n");
    }

    free(entries);
    return 0;
}

void print_simple_entry(FileEntry *entry)
{
    char *color = "";
    mode_t mode = entry->st.st_mode;

    if (S_ISDIR(mode))
    {
        color = COLOR_DIR;
    }
    else if (S_ISLNK(mode))
    {
        color = COLOR_LINK;
    }
    else if (mode & S_IXUSR || mode & S_IXGRP || mode & S_IXOTH)
    {
        color = COLOR_EXEC;
    }

    printf("%s%s%s", color, entry->name, COLOR_RESET);
}

int main(int argc, char *argv[])
{
    show_all = 0;
    long_format = 0;

    int opt;
    while ((opt = getopt(argc, argv, "la")) != -1)
    {
        switch (opt)
        {
        case 'l':
            long_format = 1;
            break;
        case 'a':
            show_all = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l] [-a] [directory...]\n", argv[0]);
            exit(1);
        }
    }

    if (optind >= argc)
    {
        return process_directory(".", 0);
    }

    int num_dirs = argc - optind;
    int need_header = (num_dirs > 1);

    int exit_code = 0;
    for (int i = optind; i < argc; i++)
    {
        if (num_dirs > 1 && i > optind)
            printf("\n");
        if (process_directory(argv[i], need_header) != 0)
            exit_code = 1;
    }

    return exit_code;
}