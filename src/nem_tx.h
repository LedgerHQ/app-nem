#pragma once

#include <stdbool.h>
#include <stdint.h>

#define MAX_PRINT_MESSAGE_LENGTH 11
#define MAX_PRINT_DETAIL_NAME_LENGTH 15
#define MAX_PRINT_EXTRA_INFOR_LENGTH 17
#define MAX_PRINT_DETAIL_NAME_SCREEN 11
#define MAX_PRINT_EXTRA_INFO_SCREEN 10
#define NEM_ADDRESS_LENGTH 41

static const uint16_t NEMV1_TRANSFER = 0x101;
static const uint16_t NEMV1_IMPORTANCE_TRANSFER = 0x801;
static const uint16_t NEMV1_MULTISIG_MODIFICATION = 0x1001;
static const uint16_t NEMV1_MULTISIG_SIGNATURE = 0x1002;
static const uint16_t NEMV1_MULTISIG_TRANSACTION = 0x1004;
static const uint16_t NEMV1_PROVISION_NAMESPACE = 0x2001;
static const uint16_t NEMV1_MOSAIC_DEFINITION = 0x4001;
static const uint16_t NEMV1_MOSAIC_SUPPLY_CHANGE = 0x4002;
static const uint16_t NEMV1_MOSAIC_SUPPLY = 0x4002;

int parse_transfer_tx (const uint8_t *raw_tx,
    unsigned int* ux_step_count, 
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char fullAddress[NEM_ADDRESS_LENGTH],
    bool isMultisig
);

int parse_mosaic_definition_tx (const uint8_t *raw_tx,
    unsigned int* ux_step_count, 
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char fullAddress[NEM_ADDRESS_LENGTH],
    bool isMultisig
);

int parse_mosaic_supply_change_tx (const uint8_t *raw_tx,
    unsigned int* ux_step_count, 
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char fullAddress[NEM_ADDRESS_LENGTH],
    bool isMultisig
);

int parse_provision_namespace_tx (const uint8_t *raw_tx,
    unsigned int* ux_step_count, 
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char fullAddress[NEM_ADDRESS_LENGTH],
    bool isMultisig
);

int parse_aggregate_modification_tx (const uint8_t *raw_tx,
    unsigned int* ux_step_count,
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char fullAddress[NEM_ADDRESS_LENGTH],
    bool isMultisig,
    uint8_t networkId
);

int parse_multisig_tx (const uint8_t *raw_tx,
    unsigned int* ux_step_count, 
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char fullAddress[NEM_ADDRESS_LENGTH],
    uint8_t networkId
);

int parse_multisig_signature_tx (const uint8_t *raw_tx,
    unsigned int* ux_step_count,
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char fullAddress[NEM_ADDRESS_LENGTH]
);
