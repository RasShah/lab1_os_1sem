#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct {
    int number_all_lines;
    int number_nonblank;      
    int show_ends;            
} CatOptions;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [-n] [-b] [-E] [FILE ...]\n", progname);
    fprintf(stderr, "       %s [--] [FILE ...]\n", progname);
}

static int process_stream(FILE *fp, const char *name, const CatOptions *opts) {
    int c;
    int exit_code = 0;
    long long line_no = 1;
    int at_line_start = 1;

    while ((c = fgetc(fp)) != EOF) {
        if (at_line_start) {
            int should_number = 0;
            if (opts->number_nonblank) {
                should_number = (c != '\n');
            } else if (opts->number_all_lines) {
                should_number = 1;
            }
            if (should_number) {
                fprintf(stdout, "%6lld\t", line_no);
                line_no++;
            }
            at_line_start = 0;
        }

        if (opts->show_ends && c == '\n') {
            fputc('$', stdout);
        }

        fputc(c, stdout);

        if (c == '\n') {
            at_line_start = 1;
        }
    }

    if (ferror(fp)) {
        fprintf(stderr, "mycat: error reading '%s': %s\n", name, strerror(errno));
        exit_code = 1;
    }

    return exit_code;
}

int main(int argc, char **argv) {
    CatOptions opts;
    memset(&opts, 0, sizeof(opts));

    int i = 1;
    int end_of_options = 0;
    for (; i < argc; ++i) {
        const char *arg = argv[i];
        if (!end_of_options && strcmp(arg, "--") == 0) {
            end_of_options = 1;
            continue;
        }
        if (!end_of_options && arg[0] == '-' && arg[1] != '\0') {
            for (int j = 1; arg[j] != '\0'; ++j) {
                char f = arg[j];
                if (f == 'n') {
                    opts.number_all_lines = 1;
                } else if (f == 'b') {
                    opts.number_nonblank = 1;
                } else if (f == 'E') {
                    opts.show_ends = 1;
                } else {
                    fprintf(stderr, "mycat: unknown option -- %c\n", f);
                    print_usage(argv[0]);
                    return 2;
                }
            }
        } else {
            break;
        }
    }
    if (opts.number_nonblank) {
        opts.number_all_lines = 0;
    }

    int exit_code = 0;

    if (i >= argc) {
        int rc = process_stream(stdin, "-", &opts);
        if (rc != 0) exit_code = rc;
        return exit_code;
    }

    for (; i < argc; ++i) {
        const char *path = argv[i];
        if (strcmp(path, "-") == 0) {
            int rc = process_stream(stdin, "-", &opts);
            if (rc != 0) exit_code = rc;
            continue;
        }

        FILE *fp = fopen(path, "rb");
        if (!fp) {
            fprintf(stderr, "mycat: cannot open '%s': %s\n", path, strerror(errno));
            exit_code = 1;
            continue;
        }

        int rc = process_stream(fp, path, &opts);
        if (rc != 0) exit_code = rc;
        fclose(fp);
    }

    return exit_code;
}