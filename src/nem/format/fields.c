/*******************************************************************************
*   NEM Wallet
*    (c) 2020 FDS
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/
#include "fields.h"
#include "common.h"
#include "limitations.h"
#include "readers.h"

void resolve_fieldname(field_t *field, char* dst) {
    if (field->dataType == STI_UINT32) {
        switch (field->id) {
            CASE_FIELDNAME(NEM_UINT32_TRANSACTION_TYPE, "Transaction Type")
            CASE_FIELDNAME(NEM_UINT32_INNER_TRANSACTION_TYPE, "Inner TX Type")
            CASE_FIELDNAME(NEM_UINT32_MOSAIC_COUNT, "Mosaics")
            CASE_FIELDNAME(NEM_UINT32_IT_MODE, "Importance Mode")
            CASE_FIELDNAME(NEM_UINT32_AM_COSIGNATORY_NUM, "Cosignatory Num")
            CASE_FIELDNAME(NEM_UINT32_AM_MODICATION_TYPE, "Mod. Type")
            CASE_FIELDNAME(NEM_UINT32_AM_RELATIVE_CHANGE, "Relative Change")
            CASE_FIELDNAME(NEM_UINT32_LEVY_FEE_TYPE, "Levy Fee Type")
        }
    }

    if (field->dataType == STI_UINT64) {
        switch (field->id) {
            CASE_FIELDNAME(NEM_UINT64_DURATION, "Duration")
        }
    }

    if (field->dataType == STI_HASH256) {
        switch (field->id) {
            CASE_FIELDNAME(NEM_HASH256, "SHA3 Tx Hash")
            CASE_FIELDNAME(NEM_PUBLICKEY_IT_REMOTE, "Rmt. Public Key")
            CASE_FIELDNAME(NEM_PUBLICKEY_AM_COSIGNATORY, "Cosignatory PbK")
        }
    }

    if (field->dataType == STI_ADDRESS) {
        switch (field->id) {
            CASE_FIELDNAME(NEM_STR_RECIPIENT_ADDRESS, "Recipient")
            CASE_FIELDNAME(NEM_STR_MULTISIG_ADDRESS, "Multisig Address")
            CASE_FIELDNAME(NEM_STR_SINK_ADDRESS, "Sink Address")
            CASE_FIELDNAME(NEM_STR_LEVY_ADDRESS, "Levy Address")
        }
    }

    if (field->dataType == STI_PROPERTY) {
        switch (field->id) {
            case NEM_STR_PROPERTY:
            {
                // field->data = len name, name, len value, value (ignore field->length)
                sprintf_ascii(dst, MAX_FIELDNAME_LEN, field->data + sizeof(uint32_t), read_uint32(field->data));
                return ;
            }
        }
    }

    if (field->dataType == STI_MOSAIC_CURRENCY) {
        switch (field->id) {
            CASE_FIELDNAME(NEM_MOSAIC_AMOUNT, "Amount")
            CASE_FIELDNAME(NEM_MOSAIC_UNITS, "Micro Units")
            case NEM_MOSAIC_SUPPLY_DELTA:
            {
                uint32_t supplyType = read_uint32(field->data);
                if (supplyType == 1) {
                    snprintf(dst, MAX_FIELDNAME_LEN, "Create Supply");
                } else if (supplyType == 2) {
                    snprintf(dst, MAX_FIELDNAME_LEN, "Delete Supply");
                }
                return ;
            }
        }
    }

    if (field->dataType == STI_NEM) {
        switch (field->id) {
            CASE_FIELDNAME(NEM_UINT64_TXN_FEE, "Fee")
            CASE_FIELDNAME(NEM_UINT64_RENTAL_FEE, "Rental Fee")
            CASE_FIELDNAME(NEM_MOSAIC_AMOUNT, "Amount")
            CASE_FIELDNAME(NEM_UINT64_LEVY_FEE, "Levy Fee")
            CASE_FIELDNAME(NEM_UINT64_MULTISIG_FEE, "Multisig Fee")
        }
    }

    if (field->dataType == STI_MESSAGE) {
        switch (field->id) {
            CASE_FIELDNAME(NEM_STR_TXN_MESSAGE, "Message")
            CASE_FIELDNAME(NEM_STR_ENC_MESSAGE, "Message")
        }
    }

    if (field->dataType == STI_STR) {
        switch (field->id) {
            CASE_FIELDNAME(NEM_MOSAIC_UNKNOWN_TYPE, "Unknown Mosaic")
            CASE_FIELDNAME(NEM_STR_TRANSFER_MOSAIC, "Namespace")
            CASE_FIELDNAME(NEM_STR_NAMESPACE, "Namespace")
            CASE_FIELDNAME(NEM_STR_PARENT_NAMESPACE, "Parent Name")
            CASE_FIELDNAME(NEM_STR_ROOT_NAMESPACE, "Create new root")
            CASE_FIELDNAME(NEM_STR_MOSAIC, "Mosaic Name")
            CASE_FIELDNAME(NEM_STR_DESCRIPTION, "Description")
            CASE_FIELDNAME(NEM_STR_LEVY_MOSAIC, "Levy Mosaic")
        }
    }

    // Default case
    snprintf(dst, MAX_FIELDNAME_LEN, "Unknown Field");
}
