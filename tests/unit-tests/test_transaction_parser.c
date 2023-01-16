#include <malloc.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

#include "parse/nem_parse.h"
#include "format/format.h"
#include "apdu/global.h"  // FIXME: transaction_context_t should be defined elsewhere

transaction_context_t transactionContext;

typedef struct {
    const char *field_name;
    const char *field_value;
} result_entry_t;

static uint8_t *load_transaction_data(const char *filename, size_t *size) {
    uint8_t *data;

    FILE *f = fopen(filename, "rb");
    if (f == NULL) {
        fprintf(stderr, "File opening failed %s\n", filename);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    size_t filesize = ftell(f);
    fseek(f, 0, SEEK_SET);

    data = malloc(filesize);
    if (data == NULL) {
        fprintf(stderr, "Malloc failed %ld\n", filesize);
        exit(1);
    }
    if (fread(data, 1, filesize, f) != filesize) {
        fprintf(stderr, "File read failed\n");
        exit(1);
    }

    *size = filesize;
    fclose(f);
    return data;
}

static void check_transaction_results(const char *filename) {
    parse_context_t context = {0};
    char field_name[MAX_FIELDNAME_LEN];
    char field_value[MAX_FIELD_LEN];

    size_t tx_length;
    uint8_t * const tx_data = load_transaction_data(filename, &tx_length);
    if (tx_data == NULL) {
        fprintf(stderr, "Loading failed %s\n", filename);
        exit(1);
    }

    context.data = tx_data;
    context.length = tx_length;

    int res = parse_txn_context(&context);
    if (res != 0) {
        fprintf(stderr, "Parsing returned %d\n", res);
        exit(1);
    }

    for (int i = 0; i < context.result.numFields; i++) {
        const field_t *field = &context.result.fields[i];
        resolve_fieldname(field, field_name);
        format_field(field, field_value);
        printf("%s::%s\n", field_name, field_value);
    }

    free(tx_data);
    return;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage:./test_transaction_parser.c <transaction_bytes_file>\n");
        exit(1);
    }

    check_transaction_results(argv[1]);

    return 0;
}
