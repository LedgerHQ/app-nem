cmake_minimum_required(VERSION 3.10)

project(NemTests)

include(CTest)

set(SRC_DIR "../../src")
set(BOLOS_SDK $ENV{BOLOS_SDK})

include_directories(
    ${SRC_DIR}
    ${SRC_DIR}/apdu
    ${SRC_DIR}/nem
    ${SRC_DIR}/nem/format
    ${SRC_DIR}/nem/parse
    ${BOLOS_SDK}/include
    ${BOLOS_SDK}/lib_standard_app
    ${BOLOS_SDK}/target/nanox/include
)

add_compile_definitions(
    TARGET_NANOX
)

add_compile_options(--coverage)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")

add_executable(test_transaction_parser
    test_transaction_parser.c
    ${SRC_DIR}/nem/nem_helpers.c
    ${SRC_DIR}/nem/parse/nem_parse.c
    ${SRC_DIR}/nem/format/fields.c
    ${SRC_DIR}/nem/format/app_format.c
    ${SRC_DIR}/nem/format/printers.c
    ${SRC_DIR}/base32.c
)

target_compile_options(test_transaction_parser PRIVATE -Wall -Wextra -pedantic -Werror)
add_compile_definitions(test_transaction_parser PRIVATE FUZZ)
target_include_directories(test_transaction_parser PRIVATE . ${SRC_DIR} ${SRC_DIR}/nem)

add_test(NAME unit_tests
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_transaction_parser.py
)
