cmake_minimum_required(VERSION 3.10)

project(NemTests)

add_executable(test_transaction_parser
    test_transaction_parser.c
    ../src/nem/nem_helpers.c
    ../src/nem/parse/nem_parse.c
    ../src/nem/format/fields.c
    ../src/nem/format/format.c
    ../src/nem/format/printers.c
    ../src/nem/format/readers.c
    ../src/base32.c
)

target_compile_options(test_transaction_parser PRIVATE -Wall -Wextra -pedantic -Werror)
add_compile_definitions(test_transaction_parser PRIVATE FUZZ)
target_include_directories(test_transaction_parser PRIVATE . ../src ../src/nem)
target_link_libraries(test_transaction_parser PRIVATE cmocka)
