#include <malloc.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>

#include "cmocka.h"

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
    assert_non_null(f);

    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
    fseek(f, 0, SEEK_SET);

    data = malloc(filesize);
    assert_non_null(data);
    assert_int_equal(fread(data, 1, filesize, f), filesize);
    *size = filesize;
    fclose(f);
    return data;
}

static void check_transaction_results(const char *filename, int num_fields, const result_entry_t *expected) {
    parse_context_t context = {0};
    char field_name[MAX_FIELDNAME_LEN];
    char field_value[MAX_FIELD_LEN];

    size_t tx_length;
    uint8_t * const tx_data = load_transaction_data(filename, &tx_length);
    assert_non_null(tx_data);

    context.data = tx_data;
    context.length = tx_length;

    assert_int_equal(parse_txn_context(&context), 0);
    assert_int_equal(context.result.numFields, num_fields);

    for (int i = 0; i < context.result.numFields; i++) {
        const field_t *field = &context.result.fields[i];
        resolve_fieldname(field, field_name);
        format_field(field, field_value);
        assert_string_equal(expected[i].field_name, field_name);
        assert_string_equal(expected[i].field_value, field_value);
    }
    free(tx_data);
    return;
}

static void test_parse_transfer_transaction(void **state) {
    (void) state;

    const result_entry_t expected[5] = {
        {"Transaction Type", "Transfer TX"},
        {"Recipient", "TBE56Z7MLQZ4S755JZL46VRYM7OD37SLPGFZPO5O"},
        {"Amount", "5 XEM"},
        {"Message", "ttest"},
        {"Fee", "0.1 XEM"}
    };

    check_transaction_results("../testcases/transfer_transaction.raw", sizeof(expected) / sizeof(expected[0]), expected);
}

static void test_parse_transfer_transaction_hex_message(void **state) {
    (void) state;

    const result_entry_t expected[5] = {
        {"Transaction Type", "Transfer TX"},
        {"Recipient", "TB7IB6DSJKWBVQEK7PD7TWO66ECW5LY6SISM2CJJ"},
        {"Amount", "10 XEM"},
        {"Message", "0123456789abcdef"},
        {"Fee", "0.1 XEM"}
    };

    check_transaction_results("../testcases/transfer_transaction_hex_message.raw", sizeof(expected) / sizeof(expected[0]), expected);
}

static void test_parse_transfer_transaction_encrypted_message(void **state) {
    (void) state;

    const result_entry_t expected[5] = {
        {"Transaction Type", "Transfer TX"},
        {"Recipient", "TB7IB6DSJKWBVQEK7PD7TWO66ECW5LY6SISM2CJJ"},
        {"Amount", "0 XEM"},
        {"Message", "<encrypted msg>"},
        {"Fee", "0.2 XEM"}
    };

    check_transaction_results("../testcases/transfer_transaction_encrypted_message.raw", sizeof(expected) / sizeof(expected[0]), expected);
}

static void test_parse_transfer_transaction_multi_mosaics(void **state) {
    (void) state;

    const result_entry_t expected[9] = {
        {"Transaction Type", "Transfer TX"},
        {"Recipient", "TB7IB6DSJKWBVQEK7PD7TWO66ECW5LY6SISM2CJJ"},
        {"Message", "Test message"},
        {"Fee", "0.15 XEM"},
        {"Mosaics", "Found 2"},
        {"Amount", "1 XEM"},
        {"Unknown Mosaic", "Divisibility and levy cannot be shown"},
        {"Namespace", "testnet: token"},
        {"Micro Units", "1"}
    };

    check_transaction_results("../testcases/transfer_transaction_multi_mosaics.raw", sizeof(expected) / sizeof(expected[0]), expected);
}

static void test_parse_transfer_transaction_multi_mosaics_2(void **state) {
    (void) state;

    const result_entry_t expected[9] = {
        {"Transaction Type", "Transfer TX"},
        {"Recipient", "TA545ICAVNEUDFUBIHO3CEJBSVIZ7YYHFFX5LQPT"},
        {"Message", "Mosaics transaction"},
        {"Fee", "0.15 XEM"},
        {"Mosaics", "Found 2"},
        {"Amount", "10 XEM"},
        {"Unknown Mosaic", "Divisibility and levy cannot be shown"},
        {"Namespace", "xarleecm.zodiac: gemini"},
        {"Micro Units", "10"}
    };

    check_transaction_results("../testcases/transfer_transaction_multi_mosaics_2.raw", sizeof(expected) / sizeof(expected[0]), expected);
}

static void test_parse_provision_subnamespace(void **state) {
    (void) state;

    const result_entry_t expected[6] = {
        {"Transaction Type", "Provision Namespace TX"},
        {"Namespace", "test_namespace_name"},
        {"Parent Name", "test_nem"},
        {"Sink Address", "TAMESPACEWH4MKFMBCVFERDPOOP4FK7MTDJEYP35"},
        {"Rental Fee", "10 XEM"},
        {"Fee", "0.15 XEM"}
    };

    check_transaction_results("../testcases/provision_subnamespace.raw", sizeof(expected) / sizeof(expected[0]), expected);
}

static void test_parse_mosaic_definition(void **state) {
    (void) state;

    const result_entry_t expected[11] = {
        {"Transaction Type", "Mosaic Definition TX"},
        {"Parent Name", "test_nem"},
        {"Mosaic Name", "mosaic_name"},
        {"Description", "mosaic description"},
        {"divisibility", "2"},
        {"initialSupply", "12"},
        {"supplyMutable", "true"},
        {"transferable", "true"},
        {"Sink Address", "TBMOSAICOD4F54EE5CDMR23CCBGOAM2XSJBR5OLC"},
        {"Rental Fee", "10 XEM"},
        {"Fee", "0.15 XEM"},
    };

    check_transaction_results("../testcases/mosaic_definition.raw", sizeof(expected) / sizeof(expected[0]), expected);
}

static void test_parse_mosaic_definition_with_levy(void **state) {
    (void) state;

    const result_entry_t expected[15] = {
        {"Transaction Type", "Mosaic Definition TX"},
        {"Parent Name", "test_nem"},
        {"Mosaic Name", "mosaic_name"},
        {"Description", "mosaic description"},
        {"divisibility", "2"},
        {"initialSupply", "12"},
        {"supplyMutable", "true"},
        {"transferable", "true"},
        {"Levy Mosaic", "nem: xem"},
        {"Levy Address", "TB7IB6DSJKWBVQEK7PD7TWO66ECW5LY6SISM2CJJ"},
        {"Levy Fee Type", "Absolute"},
        {"Levy Fee", "0.000005 micro"},
        {"Sink Address", "TBMOSAICOD4F54EE5CDMR23CCBGOAM2XSJBR5OLC"},
        {"Rental Fee", "10 XEM"},
        {"Fee", "0.15 XEM"}
    };

    check_transaction_results("../testcases/mosaic_definition_with_levy.raw", sizeof(expected) / sizeof(expected[0]), expected);
}

static void test_parse_multisig_transfer_transaction(void **state) {
    (void) state;

    const result_entry_t expected[7] = {
        {"Transaction Type", "Multisig TX"},
        {"Multisig Fee", "0.15 XEM"},
        {"Inner TX Type", "Transfer TX"},
        {"Recipient", "TB7IB6DSJKWBVQEK7PD7TWO66ECW5LY6SISM2CJJ"},
        {"Amount", "10 XEM"},
        {"Message", "Send a transfer transaction from a multisig account using Ledger"},
        {"Fee", "0.2 XEM"}
    };

    check_transaction_results("../testcases/multisig_transfer_transaction.raw", sizeof(expected) / sizeof(expected[0]), expected);
}

static void test_parse_multisig_provision_namespace(void **state) {
    (void) state;

    const result_entry_t expected[8] = {
        {"Transaction Type", "Multisig TX"},
        {"Multisig Fee", "0.15 XEM"},
        {"Inner TX Type", "Provision Namespace TX"},
        {"Namespace", "test_namespace"},
        {"Create new root", "namespace"},
        {"Sink Address", "TAMESPACEWH4MKFMBCVFERDPOOP4FK7MTDJEYP35"},
        {"Rental Fee", "100 XEM"},
        {"Fee", "0.15 XEM"}
    };

    check_transaction_results("../testcases/multisig_provision_namespace.raw", sizeof(expected) / sizeof(expected[0]), expected);
}

static void test_parse_multisig_mosaic_definition(void **state) {
    (void) state;

    const result_entry_t expected[13] = {
        {"Transaction Type", "Multisig TX"},
        {"Multisig Fee", "0.15 XEM"},
        {"Inner TX Type", "Mosaic Definition TX"},
        {"Parent Name", "test_nem"},
        {"Mosaic Name", "mosaic_name"},
        {"Description", "mosaic description"},
        {"divisibility", "2"},
        {"initialSupply", "12"},
        {"supplyMutable", "true"},
        {"transferable", "true"},
        {"Sink Address", "TBMOSAICOD4F54EE5CDMR23CCBGOAM2XSJBR5OLC"},
        {"Rental Fee", "10 XEM"},
        {"Fee", "0.15 XEM"}
    };

    check_transaction_results("../testcases/multisig_mosaic_definition.raw", sizeof(expected) / sizeof(expected[0]), expected);
}

static void test_parse_multisig_mosaic_definition_with_levy(void **state) {
    (void) state;

    const result_entry_t expected[17] = {
        {"Transaction Type", "Multisig TX"},
        {"Multisig Fee", "0.15 XEM"},
        {"Inner TX Type", "Mosaic Definition TX"},
        {"Parent Name", "test_nem"},
        {"Mosaic Name", "mosaic_create_from_ledger"},
        {"Description", "This mosaic is created by a ledger wallet from a multisig account"},
        {"divisibility", "3"},
        {"initialSupply", "1000"},
        {"supplyMutable", "true"},
        {"transferable", "true"},
        {"Levy Mosaic", "nem: xem"},
        {"Levy Address", "TB7IB6DSJKWBVQEK7PD7TWO66ECW5LY6SISM2CJJ"},
        {"Levy Fee Type", "Absolute"},
        {"Levy Fee", "0.000005 micro"},
        {"Sink Address", "TBMOSAICOD4F54EE5CDMR23CCBGOAM2XSJBR5OLC"},
        {"Rental Fee", "10 XEM"},
        {"Fee", "0.15 XEM"}
    };

    check_transaction_results("../testcases/multisig_mosaic_definition_with_levy.raw", sizeof(expected) / sizeof(expected[0]), expected);
}

static void test_parse_multisig_cosignature_transfer_transaction(void **state) {
    (void) state;

    const result_entry_t expected[9] = {
        {"Transaction Type", "Multi Sig. TX"},
        {"SHA3 Tx Hash", "9298E6A7255F269D88BA5D096341B3C56C3E65A511B72A947097C0CFAB95D4B1"},
        {"Multisig Address", "TA6DD3TAAW7DIOFJKWHNJJZQLTSRWAQ67YKWYQBG"},
        {"Multisig Fee", "0.15 XEM"},
        {"Detail TX Type", "Transfer TX"},
        {"Recipient", "TB7IB6DSJKWBVQEK7PD7TWO66ECW5LY6SISM2CJJ"},
        {"Amount", "0.4 XEM"},
        {"Message", "test message"},
        {"Fee", "0.1 XEM"}
    };

    check_transaction_results("../testcases/multisig_cosignature_transfer_transaction.raw", sizeof(expected) / sizeof(expected[0]), expected);
}

static void test_parse_multisig_cosignature_provision_namespace(void **state) {
    (void) state;

    const result_entry_t expected[10] = {
        {"Transaction Type", "Multi Sig. TX"},
        {"SHA3 Tx Hash", "39D64C1602FA48D49C59A464EF109235472134EA854D1B70284EA32AEC6DD17A"},
        {"Multisig Address", "TA6DD3TAAW7DIOFJKWHNJJZQLTSRWAQ67YKWYQBG"},
        {"Multisig Fee", "0.15 XEM"},
        {"Detail TX Type", "Provision Namespace TX"},
        {"Namespace", "ledger_ns"},
        {"Create new root", "namespace"},
        {"Sink Address", "TAMESPACEWH4MKFMBCVFERDPOOP4FK7MTDJEYP35"},
        {"Rental Fee", "100 XEM"},
        {"Fee", "0.15 XEM"}
    };

    check_transaction_results("../testcases/multisig_cosignature_provision_namespace.raw", sizeof(expected) / sizeof(expected[0]), expected);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_parse_transfer_transaction),
        cmocka_unit_test(test_parse_transfer_transaction_hex_message),
        cmocka_unit_test(test_parse_transfer_transaction_encrypted_message),
        cmocka_unit_test(test_parse_transfer_transaction_multi_mosaics),
        cmocka_unit_test(test_parse_transfer_transaction_multi_mosaics_2),
        cmocka_unit_test(test_parse_provision_subnamespace),
        cmocka_unit_test(test_parse_mosaic_definition),
        cmocka_unit_test(test_parse_mosaic_definition_with_levy),
        cmocka_unit_test(test_parse_multisig_transfer_transaction),
        cmocka_unit_test(test_parse_multisig_provision_namespace),
        cmocka_unit_test(test_parse_multisig_mosaic_definition),
        cmocka_unit_test(test_parse_multisig_mosaic_definition_with_levy),
        cmocka_unit_test(test_parse_multisig_cosignature_transfer_transaction),
        cmocka_unit_test(test_parse_multisig_cosignature_provision_namespace),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
