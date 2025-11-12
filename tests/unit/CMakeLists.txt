cmake_minimum_required(VERSION 3.10)

project(NemTests)

set(SRC_DIR "../../src")

add_executable(test_transaction_parser
    test_transaction_parser.c
    ${SRC_DIR}/nem/nem_helpers.c
    ${SRC_DIR}/nem/parse/nem_parse.c
    ${SRC_DIR}/nem/format/fields.c
    ${SRC_DIR}/nem/format/format.c
    ${SRC_DIR}/nem/format/printers.c
    ${SRC_DIR}/nem/format/readers.c
    ${SRC_DIR}/base32.c
)

target_compile_options(test_transaction_parser PRIVATE -Wall -Wextra -pedantic -Werror)
add_compile_definitions(test_transaction_parser PRIVATE FUZZ)
target_include_directories(test_transaction_parser PRIVATE . ${SRC_DIR} ${SRC_DIR}/nem)
