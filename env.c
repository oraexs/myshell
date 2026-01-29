#include "env.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/***
update_shell_var
DESC : Update only the value if the name already exists
***/
void update_shell_var(const char *name, const char *value, int i)
{
    (void)name; /* name unused but kept for API clarity */

    free(shell_vars[i].value);

    shell_vars[i].value = malloc(strlen(value) + 1);
    if (shell_vars[i].value == NULL) {
        perror("Allocation failed : update_shell_var");
        return;
    }

    strcpy(shell_vars[i].value, value);
}

/***
create_shell_var
DESC : Create a new shell variable
***/
void create_shell_var(const char *name, const char *value)
{
    shell_vars[shell_var_count].name = malloc(strlen(name) + 1);
    if (shell_vars[shell_var_count].name == NULL) {
        perror("Allocation failed : create_shell_var(name)");
        return;
    }

    strcpy(shell_vars[shell_var_count].name, name);

    shell_vars[shell_var_count].value = malloc(strlen(value) + 1);
    if (shell_vars[shell_var_count].value == NULL) {
        free(shell_vars[shell_var_count].name);
        perror("Allocation failed : create_shell_var(value)");
        return;
    }

    strcpy(shell_vars[shell_var_count].value, value);
    shell_var_count++;
}

/***
set_shell_var
DESC :
    Update if exists, else create
***/
void set_shell_var(const char *name, const char *value)
{
    for (int i = 0; i < shell_var_count; i++) {
        if (strcmp(name, shell_vars[i].name) == 0) {
            update_shell_var(name, value, i);
            return;
        }
    }

    if (shell_var_count < MAX_SHELL_VARS) {
        create_shell_var(name, value);
    } else {
        perror("Maximum shell variable limit reached");
    }
}

/***
get_shell_var
DESC : Fetch value of shell variable
***/
const char* get_shell_var(const char* name)
{
    for (int i = 0; i < shell_var_count; i++) {
        if (strcmp(name, shell_vars[i].name) == 0) {
            return shell_vars[i].value;
        }
    }
    return NULL;
}

/***
print_shell_var
DESC : Print all shell variables
***/
void print_shell_var(void)
{
    for (int i = 0; i < shell_var_count; i++) {
        printf("%s=%s\n", shell_vars[i].name, shell_vars[i].value);
    }
}

/***
is_valid_assignment
DESC :
    x=10  → valid
    =10   → invalid
***/
int is_valid_assignment(const char* input)
{
    int pos = 0;

    while (*input) {
        if (*input == '=') {
            return pos > 0;
        }
        pos++;
        input++;
    }
    return 0;
}

/***
handle_valid_assignment
DESC :
    Parse name=value and store atomically
***/
void handle_valid_assignment(const char *input)
{
    const char* eq = strchr(input, '=');
    if (!eq) return;

    size_t name_len = eq - input;
    size_t val_len  = strlen(eq + 1);

    char* name  = malloc(name_len + 1);
    char* value = malloc(val_len + 1);

    if (!name || !value) {
        perror("handle_valid_assignment : malloc failed");
        free(name);
        free(value);
        return;
    }

    strncpy(name, input, name_len);
    name[name_len] = '\0';

    strcpy(value, eq + 1);

    set_shell_var(name, value);

    free(name);
    free(value);
}

/***
expand_variables
DESC :
    Replace $VAR with its value
***/
char* expand_variables(const char* input)
{
    size_t in_len  = strlen(input);
    size_t out_cap = in_len + 1;
    size_t out_len = 0;

    char *output = malloc(out_cap);
    if (!output) {
        perror("malloc");
        return NULL;
    }

    for (size_t i = 0; i < in_len; i++) {

        if (input[i] == '$' &&
            (isalpha((unsigned char)input[i + 1]) || input[i + 1] == '_')) {

            i++;
            char varname[256];
            size_t vi = 0;

            while (isalnum((unsigned char)input[i]) || input[i] == '_') {
                varname[vi++] = input[i++];
            }
            varname[vi] = '\0';
            i--;

            const char *value = get_shell_var(varname);
            if (value) {
                size_t val_len = strlen(value);
                if (out_len + val_len + 1 > out_cap) {
                    out_cap = out_len + val_len + 16;
                    char *tmp = realloc(output, out_cap);
                    if (!tmp) {
                        free(output);
                        return NULL;
                    }
                    output = tmp;
                }
                memcpy(output + out_len, value, val_len);
                out_len += val_len;
            }

        } else {
            if (out_len + 2 > out_cap) {
                out_cap += 16;
                char *tmp = realloc(output, out_cap);
                if (!tmp) {
                    free(output);
                    return NULL;
                }
                output = tmp;
            }
            output[out_len++] = input[i];
        }
    }

    output[out_len] = '\0';
    return output;
}
