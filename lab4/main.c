#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>

int check_if_numeric(const char *input_str) {
    int index = 0;
    while (input_str[index] != '\0') {
        if (input_str[index] < '0' || input_str[index] > '7') {
            return 0;
        }
        index++;
    }
    return 1;
}

mode_t convert_octal_mode(const char *mode_string) {
    char *terminator;
    long octal_result = strtol(mode_string, &terminator, 8);
    
    // Проверка корректности
    if (*terminator != '\0' || octal_result < 0 || octal_result > 07777) {
        fprintf(stderr, "Некорректный режим доступа: '%s'\n", mode_string);
        return (mode_t)-1;
    }
    
    return (mode_t)octal_result;
}

int process_symbolic_permission(const char *perm_string, mode_t existing_mode, mode_t *updated_mode) {
    const char *current_pos = perm_string;
    mode_t target_mask = 0;
    char operation_type = 0;
    mode_t perm_bits = 0;
    
    int flags_specified = 0;
    while (*current_pos && (*current_pos == 'u' || *current_pos == 'g' || *current_pos == 'o' || *current_pos == 'a')) {
        flags_specified = 1;
        switch (*current_pos) {
            case 'u':
                target_mask |= S_IRWXU;
                break;
            case 'g':
                target_mask |= S_IRWXG;
                break;
            case 'o':
                target_mask |= S_IRWXO;
                break;
            case 'a':
                target_mask |= S_IRWXU | S_IRWXG | S_IRWXO;
                break;
        }
        current_pos++;
    }
    
    if (!flags_specified) {
        target_mask = S_IRWXU | S_IRWXG | S_IRWXO;
    }
    
    if (*current_pos == '+' || *current_pos == '-' || *current_pos == '=') {
        operation_type = *current_pos;
        current_pos++;
    } else {
        fprintf(stderr, "Неправильный синтаксис в '%s'\n", perm_string);
        return -1;
    }
    
    while (*current_pos && (*current_pos == 'r' || *current_pos == 'w' || *current_pos == 'x')) {
        switch (*current_pos) {
            case 'r':
                perm_bits |= (S_IRUSR | S_IRGRP | S_IROTH);
                break;
            case 'w':
                perm_bits |= (S_IWUSR | S_IWGRP | S_IWOTH);
                break;
            case 'x':
                perm_bits |= (S_IXUSR | S_IXGRP | S_IXOTH);
                break;
        }
        current_pos++;
    }
    
    if (*current_pos != '\0') {
        fprintf(stderr, "Синтаксическая ошибка в '%s'\n", perm_string);
        return -1;
    }
    
    switch (operation_type) {
        case '+':
            *updated_mode = existing_mode | (perm_bits & target_mask);
            break;
        case '-':
            *updated_mode = existing_mode & ~(perm_bits & target_mask);
            break;
        case '=':
            *updated_mode = (existing_mode & ~target_mask) | (perm_bits & target_mask);
            break;
    }
    
    return 0;
}

int apply_symbolic_changes(const char *mode_expression, mode_t current_perms, mode_t *result_perms) {
    char expression_copy[512];
    strncpy(expression_copy, mode_expression, sizeof(expression_copy) - 1);
    expression_copy[sizeof(expression_copy) - 1] = '\0';
    
    *result_perms = current_perms;
    
    char *tokenized_part = strtok(expression_copy, ",");
    while (tokenized_part != NULL) {
        if (process_symbolic_permission(tokenized_part, *result_perms, result_perms) == -1) {
            return -1;
        }
        tokenized_part = strtok(NULL, ",");
    }
    
    return 0;
}

int modify_file_permissions(const char *file_path, const char *mode_specification) {
    struct stat file_info;
    mode_t final_mode;
    
    if (stat(file_path, &file_info) == -1) {
        perror("Ошибка получения информации о файле");
        return -1;
    }
    
    if (check_if_numeric(mode_specification)) {
        final_mode = convert_octal_mode(mode_specification);
        if (final_mode == (mode_t)-1) {
            return -1;
        }
    } else {
        if (apply_symbolic_changes(mode_specification, file_info.st_mode, &final_mode) == -1) {
            return -1;
        }
    }
    
    if (chmod(file_path, final_mode) == -1) {
        perror("Ошибка изменения прав");
        return -1;
    }
    
    printf("Права доступа для '%s' обновлены успешно\n", file_path);
    return 0;
}

int main(int arg_count, char *arg_values[]) {
    if (arg_count != 3) {
        fprintf(stderr, "Формат команды: %s <режим> <файл>\n", arg_values[0]);
        fprintf(stderr, "Доступные варианты:\n");
        fprintf(stderr, "  %s +x filename\n", arg_values[0]);
        fprintf(stderr, "  %s u-w filename\n", arg_values[0]);
        fprintf(stderr, "  %s g+rx filename\n", arg_values[0]);
        fprintf(stderr, "  %s ug+rw filename\n", arg_values[0]);
        fprintf(stderr, "  %s a+rwx filename\n", arg_values[0]);
        fprintf(stderr, "  %s u=rwx,g=rx,o= filename\n", arg_values[0]);
        fprintf(stderr, "  %s 755 filename\n", arg_values[0]);
        return EXIT_FAILURE;
    }
    
    const char *permission_mode = arg_values[1];
    const char *target_file = arg_values[2];
    
    if (modify_file_permissions(target_file, permission_mode) == -1) {
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}