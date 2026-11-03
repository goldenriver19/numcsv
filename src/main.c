#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    double *items;
    size_t count;
    size_t capacity;
} Values;

static int append_value(Values *values, double value) {
    if (values->count == values->capacity) {
        size_t capacity;
        if (values->capacity == 0) {
            capacity = 16;
        } else {
            if (values->capacity > SIZE_MAX / (2 * sizeof *values->items)) {
                return 0;
            }
            capacity = values->capacity * 2;
        }
        double *items = realloc(values->items, capacity * sizeof *items);
        if (items == NULL) return 0;
        values->items = items;
        values->capacity = capacity;
    }
    values->items[values->count++] = value;
    return 1;
}

static char *field_at(const char *line, size_t index) {
    size_t field = 0;
    size_t start = 0;
    size_t i = 0;
    int in_quotes = 0;

    while (field < index) {
        in_quotes = 0;
        while (line[i] != '\0') {
            if (line[i] == '"') {
                if (in_quotes && line[i + 1] == '"') {
                    i += 2;
                    continue;
                }
                in_quotes = !in_quotes;
                i++;
            } else if (!in_quotes &&
                       (line[i] == ',' || line[i] == '\n' ||
                        line[i] == '\r')) {
                break;
            } else {
                i++;
            }
        }

        if (line[i] == ',') {
            i++;
            field++;
            start = i;
            continue;
        }
        return NULL;
    }

    in_quotes = 0;
    while (line[i] != '\0') {
        if (line[i] == '"') {
            if (in_quotes && line[i + 1] == '"') {
                i += 2;
                continue;
            }
            in_quotes = !in_quotes;
            i++;
        } else if (!in_quotes &&
                   (line[i] == ',' || line[i] == '\n' ||
                    line[i] == '\r')) {
            break;
        } else {
            i++;
        }
    }

    size_t end = i;

    while (start < end && (line[start] == ' ' || line[start] == '\t' ||
                           line[start] == '\r' || line[start] == '\n')) {
        start++;
    }
    while (start < end && (line[end - 1] == ' ' || line[end - 1] == '\t' ||
                           line[end - 1] == '\r' || line[end - 1] == '\n')) {
        end--;
    }

    int quoted = end > start && line[start] == '"' &&
                 line[end - 1] == '"';
    char *result;

    if (quoted) {
        size_t close = end - 1;
        size_t max_length = close - start;
        result = malloc(max_length + 1);
        if (result == NULL) return NULL;

        size_t output = 0;
        size_t j = start + 1;
        while (j < close) {
            if (line[j] == '"' && j + 1 < close && line[j + 1] == '"') {
                result[output++] = '"';
                j += 2;
            } else {
                result[output++] = line[j++];
            }
        }
        result[output] = '\0';
    } else {
        size_t length = end - start;
        result = malloc(length + 1);
        if (result == NULL) return NULL;
        memcpy(result, line + start, length);
        result[length] = '\0';
    }

    char *p = result;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
        p++;
    }
    if (p != result) {
        memmove(result, p, strlen(p) + 1);
    }
    size_t length = strlen(result);
    while (length > 0 &&
           (result[length - 1] == ' ' || result[length - 1] == '\t' ||
            result[length - 1] == '\r' || result[length - 1] == '\n')) {
        result[--length] = '\0';
    }
    return result;
}

static int compare_double(const void *a, const void *b) {
    double x = *(const double *)a;
    double y = *(const double *)b;
    return (x > y) - (x < y);
}

static int parse_column(const char *text) {
    char *end = NULL;
    errno = 0;
    long value = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value < 1 || value > INT_MAX) {
        return -1;
    }
    return (int)value;
}

static void usage(const char *program) {
    fprintf(stderr, "Usage: %s [-c COLUMN] [-H] FILE\n", program);
}

int main(int argc, char **argv) {
    int column = 1;
    int header = 0;
    const char *path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            int parsed = parse_column(argv[++i]);
            if (parsed < 0) {
                fprintf(stderr, "numcsv: invalid column '%s'\n", argv[i]);
                return 2;
            }
            column = parsed;
        } else if (strcmp(argv[i], "-H") == 0) {
            header = 1;
        } else {
            path = argv[i];
        }
    }

    if (path == NULL || column < 1) {
        usage(argv[0]);
        return 2;
    }

    setlocale(LC_NUMERIC, "C");

    FILE *file;
    if (strcmp(path, "-") == 0) {
        file = stdin;
    } else {
        file = fopen(path, "r");
        if (file == NULL) {
            fprintf(stderr, "numcsv: cannot open %s: %s\n", path, strerror(errno));
            return 1;
        }
    }

    Values values = {0};
    char *line = NULL;
    size_t line_capacity = 0;
    ssize_t length;
    int first = 1;

    while ((length = getline(&line, &line_capacity, file)) != -1) {
        if (first && header) {
            first = 0;
            continue;
        }
        first = 0;
        char *text = field_at(line, (size_t)column - 1);
        if (text == NULL) continue;
        if (text[0] == '\0') {
            free(text);
            continue;
        }
        char *end = NULL;
        errno = 0;
        double value = strtod(text, &end);
        if (errno != 0 || end == text || *end != '\0' || !isfinite(value)) {
            free(text);
            continue;
        }
        free(text);
        if (!append_value(&values, value)) {
            fprintf(stderr, "numcsv: out of memory\n");
            free(line);
            if (file != stdin) fclose(file);
            free(values.items);
            return 1;
        }
    }

    free(line);
    if (file != stdin) fclose(file);

    if (values.count == 0) {
        fprintf(stderr, "numcsv: no numeric values found\n");
        free(values.items);
        return 1;
    }

    qsort(values.items, values.count, sizeof(double), compare_double);
    double sum = 0.0;
    for (size_t i = 0; i < values.count; i++) sum += values.items[i];
    double mean = sum / (double)values.count;
    double median;
    if (values.count % 2 == 1) {
        median = values.items[values.count / 2];
    } else {
        median = (values.items[values.count / 2 - 1] +
                  values.items[values.count / 2]) /
                 2.0;
    }

    printf("count: %zu\n", values.count);
    printf("min: %.10g\n", values.items[0]);
    printf("max: %.10g\n", values.items[values.count - 1]);
    printf("mean: %.10g\n", mean);
    printf("median: %.10g\n", median);

    free(values.items);
    return 0;
}
