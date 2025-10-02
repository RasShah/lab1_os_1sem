#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s PATTERN [FILE ...]\n", progname);
}

typedef struct {
    const char *pattern;
} GrepOptions;

static int line_matches(const char *line, const char *pattern) {
    return strstr(line, pattern) != NULL;
}

static int process_stream(FILE *fp, const char *name, const GrepOptions *opts, int print_filename_prefix) {
    char *buffer = NULL;
    size_t buffer_size = 0;
    size_t len = 0;
    int c;
    int matched_any = 0;

    while (1) {
        c = fgetc(fp);
        if (c == EOF) {
            if (len > 0) {
                buffer[len] = '\0';
                if (line_matches(buffer, opts->pattern)) {
                    if (print_filename_prefix) {
                        fprintf(stdout, "%s:", name);
                    }
                    fputs(buffer, stdout);
                    fputc('\n', stdout);
                    matched_any = 1;
                }
                len = 0;
            }
            break;
        }

        if (len + 1 >= buffer_size) {
            size_t new_size = buffer_size == 0 ? 256 : buffer_size * 2;
            char *new_buf = (char*)realloc(buffer, new_size);
            if (!new_buf) {
                fprintf(stderr, "mygrep: memory allocation failed\n");
                free(buffer);
                return 2;
            }
            buffer = new_buf;
            buffer_size = new_size;
        }

        if (c == '\r') {
            continue;
        }

        if (c == '\n') {
            buffer[len] = '\0';
            if (line_matches(buffer, opts->pattern)) {
                if (print_filename_prefix) {
                    fprintf(stdout, "%s:", name);
                }
                fputs(buffer, stdout);
                fputc('\n', stdout);
                matched_any = 1;
            }
            len = 0;
        } else {
            buffer[len++] = (char)c;
        }
    }

    if (ferror(fp)) {
        fprintf(stderr, "mygrep: error reading '%s': %s\n", name, strerror(errno));
        free(buffer);
        return 2;
    }

    free(buffer);
    return matched_any ? 0 : 1; 
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 2;
    }

    GrepOptions opts;
    opts.pattern = argv[1];

    int argi = 2;
    int exit_code = 0;

    if (argi >= argc) {
        int rc = process_stream(stdin, "-", &opts, 0);
        return rc;
    }

    int num_files = argc - argi;
    int print_filename_prefix = (num_files > 1);

    for (; argi < argc; ++argi) {
        const char *path = argv[argi];
        if (strcmp(path, "-") == 0) {
            int rc = process_stream(stdin, "-", &opts, print_filename_prefix);
            if (rc != 0) exit_code = rc;
            continue;
        }

        FILE *fp = fopen(path, "rb");
        if (!fp) {
            fprintf(stderr, "mygrep: cannot open '%s': %s\n", path, strerror(errno));
            exit_code = 2;
            continue;
        }
        int rc = process_stream(fp, path, &opts, print_filename_prefix);
        if (rc != 0) exit_code = rc;
        fclose(fp);
    }

    return exit_code;
}