#include <errno.h>
#include <math.h>
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
        size_t capacity = values->capacity == 0 ? 16 : values->capacity * 2;
        double *items = realloc(values->items, capacity * sizeof(double));
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
    int in_quotes = 0;
    size_t i;
    for (i = 0; line[i] != '\0'; i++) {
        if (line[i] == '"') {
            in_quotes = !in_quotes;
        } else if (line[i] == ',' && !in_quotes) {
            if (field == index) break;
            field++;
            start = i + 1;
        }
    }
    size_t end = i;
    if (field != index) return NULL;
    size_t length = end - start;
    char *result = malloc(length + 1);
    if (result == NULL) return NULL;
    memcpy(result, line + start, length);
    result[length] = '\0';
    /* Trim surrounding whitespace and quotes. */
    char *p = result;
    while (*p == ' ' || *p == '\t' || *p == '"') p++;
    if (p != result) memmove(result, p, strlen(p) + 1);
    size_t len = strlen(result);
    while (len > 0 && (result[len - 1] == ' ' || result[len - 1] == '\t' || result[len - 1] == '"' || result[len - 1] == '\n' || result[len - 1] == '\r')) {
        result[--len] = '\0';
    }
    return result;
}

static int compare_double(const void *a, const void *b) {
    double x = *(const double *)a;
    double y = *(const double *)b;
    return (x > y) - (x < y);
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
            column = atoi(argv[++i]);
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

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        fprintf(stderr, "numcsv: cannot open %s: %s\n", path, strerror(errno));
        return 1;
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
        if (errno != 0 || end == text || *end != '\0') {
            free(text);
            continue;
        }
        free(text);
        if (!append_value(&values, value)) {
            fprintf(stderr, "numcsv: out of memory\n");
            free(line);
            fclose(file);
            free(values.items);
            return 1;
        }
    }

    free(line);
    fclose(file);

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
        median = (values.items[values.count / 2 - 1] + values.items[values.count / 2]) / 2.0;
    }

    printf("count: %zu\n", values.count);
    printf("min: %.10g\n", values.items[0]);
    printf("max: %.10g\n", values.items[values.count - 1]);
    printf("mean: %.10g\n", mean);
    printf("median: %.10g\n", median);

    free(values.items);
    return 0;
}
