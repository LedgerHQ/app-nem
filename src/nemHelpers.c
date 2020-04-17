/*******************************************************************************
*   NEM Wallet
*   (c) 2017 Ledger
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
#include "os.h"
#include "cx.h"
#include "base32.h"
#include <ctype.h>
#include <inttypes.h>
#include "nemHelpers.h"
#define MAX_SAFE_INTEGER 9007199254740991

static const uint8_t AMOUNT_MAX_SIZE = 17;

uint8_t readNetworkIdFromBip32path(uint32_t bip32Path[]) {
    uint8_t outNetworkId;
    switch(bip32Path[2]) {
        case 0x80000068: 
            outNetworkId = 104; //N: mainnet
            break;
        case 0x80000098:
           outNetworkId = 152; //T
           break;
        case 0x80000060:
            outNetworkId = 96; //M
            break;
        case 0x80000090:
            outNetworkId = 144; //S
            break;
        default:
            THROW(0x6a80);
    }
    return outNetworkId;
}

//todo nonprintable ch + utf8
void uint2Ascii(uint8_t *inBytes, uint8_t len, char *out){
    char *tmpCh = (char *)inBytes;
    for (uint8_t j=0; j<len; j++){
        out[j] = tmpCh[j];
    }
    out[len] = '\0';
}

uint8_t *reverseBytes(uint8_t *sourceArray, uint16_t len){
    uint8_t outArray[len];
    for (uint8_t j=0; j<len; j++) {
        outArray[j] = sourceArray[len - j -1];
    }
    return outArray;
}

void print_amount(uint64_t amount, uint8_t divisibility, char *asset, char *out) {
    char buffer[AMOUNT_MAX_SIZE];
    uint64_t dVal = amount;
    int i, j;

    // If the amount can't be represented safely in JavaScript, signal an error
    //if (MAX_SAFE_INTEGER < amount) THROW(0x6a80);

    memset(buffer, 0, AMOUNT_MAX_SIZE);
    for (i = 0; dVal > 0 || i < 7; i++) {
        if (dVal > 0) {
            buffer[i] = (dVal % 10) + '0';
            dVal /= 10;
        } else {
            buffer[i] = '0';
        }
        if (i == divisibility - 1) { // divisibility
            i += 1;
            buffer[i] = '.';
            if (dVal == 0) {
                i += 1;
                buffer[i] = '0'; 
            }           
        }
        if (i >= AMOUNT_MAX_SIZE) {
            THROW(0x6700);
        }
    }
    // reverse order
    for (i -= 1, j = 0; i >= 0 && j < AMOUNT_MAX_SIZE-1; i--, j++) {
        out[j] = buffer[i];
    }
    // strip trailing 0s
    for (j -= 1; j > 0; j--) {
        if (out[j] != '0') break;
    }
    j += 1;

    // strip trailing .
    if (out[j-1] == '.') j -= 1;

    if (asset) {
        // qualify amount
        out[j++] = ' ';
        strcpy(out + j, asset);
        out[j+strlen(asset)] = '\0';
    } else {
        out[j] = '\0';
    }

}

uint16_t getUint16(uint8_t *buffer) {
    return ((uint16_t)buffer[1]) | ((uint16_t)buffer[0] << 8);
}

uint32_t getUint32(uint8_t *data) {
    return ((uint32_t)data[3]) | ((uint32_t)data[2] << 8) | ((uint32_t)data[1] << 16) |
             ((uint32_t)data[0] << 24);
}

uint64_t getUint64(uint8_t *data) {
    return ((uint64_t)data[7]) | ((uint64_t)data[6] << 8) | ((uint64_t)data[5] << 16) |
             ((uint64_t)data[4] << 24) | ((uint64_t)data[3] << 32) | ((uint64_t)data[2] << 40) |
             ((uint64_t)data[1] << 48) | ((uint64_t)data[0] << 56);
}

void to_nem_public_key_and_address(cx_ecfp_public_key_t *inPublicKey, uint8_t inNetworkId, unsigned int inAlgo, uint8_t *outNemPublicKey, unsigned char *outNemAddress) {
    uint8_t i;
    for (i=0; i<32; i++) {
        outNemPublicKey[i] = inPublicKey->W[64 - i];
    }

    if ((inPublicKey->W[32] & 1) != 0) {
        outNemPublicKey[31] |= 0x80;
    }    

    cx_sha3_t hash1;
    cx_sha3_t temphash;
    
    cx_sha3_init(&hash1, 256);
    cx_sha3_init(&temphash, 256);
    
    unsigned char buffer1[32];
    cx_hash(&hash1.header, CX_LAST, outNemPublicKey, 32, buffer1);
    unsigned char buffer2[20];
    cx_ripemd160_t hash2;
    cx_ripemd160_init(&hash2);
    cx_hash(&hash2.header, CX_LAST, buffer1, 32, buffer2);
    unsigned char rawAddress[50];
    //step1: add network prefix char
    rawAddress[0] = inNetworkId;   //104,,,,,
    //step2: add ripemd160 hash
    os_memmove(rawAddress + 1, buffer2, sizeof(buffer2));
    
    unsigned char buffer3[32];
    cx_hash(&temphash.header, CX_LAST, rawAddress, 21, buffer3);
    //step3: add checksum
    os_memmove(rawAddress + 21, buffer3, 4);
    base32_encode(rawAddress, sizeof(rawAddress), outNemAddress, 40);
}

void public_key_to_address(uint8_t inNetworkId, uint8_t *inNemPublicKey, unsigned char *outNemAddress) {
    cx_sha3_t hash1;
    cx_sha3_t temphash;
    cx_keccak_init(&hash1, 256);
    cx_keccak_init(&temphash, 256);

    unsigned char buffer1[32];
    cx_hash(&hash1.header, CX_LAST, inNemPublicKey, 32, buffer1);
    unsigned char buffer2[20];
    cx_ripemd160_t hash2;
    cx_ripemd160_init(&hash2);
    cx_hash(&hash2.header, CX_LAST, buffer1, 32, buffer2);
    unsigned char rawAddress[50];
    //step1: add network prefix char
    rawAddress[0] = inNetworkId;   //104,,,,,
    //step2: add ripemd160 hash
    os_memmove(rawAddress + 1, buffer2, sizeof(buffer2));
    
    unsigned char buffer3[32];
    cx_hash(&temphash.header, CX_LAST, rawAddress, 21, buffer3);
    //step3: add checksum
    os_memmove(rawAddress + 21, buffer3, 4);
    base32_encode(rawAddress, sizeof(rawAddress), outNemAddress, 40);
}


unsigned int get_apdu_buffer_length() {
	unsigned int len0 = G_io_apdu_buffer[APDU_BODY_LENGTH_OFFSET];
	return len0;
}

void clean_raw_tx(unsigned char *raw_tx) {
    uint16_t i;
    for (i = 0; i < MAX_TX_RAW_LENGTH; i++) {
        raw_tx[i] = 0;
    }
}

int compare_strings (char str1[], char str2[]) {
    int index = 0;
 
    while (str1[index] == str2[index]) {
        if (str1[index] == '\0' || str2[index] == '\0')
            break;
        index++;
    }
    
    if (str1[index] == '\0' && str2[index] == '\0')
        return 0;
    else
        return -1;
}

int string_length(char str[]) {
    int index = 0;
 
    while (str[index] != '\0') {
        str++;
    }
 
    return index;
}

/** Convert 1 hex byte to 2 characters */
char hex2Ascii(uint8_t input){
    return input > 9 ? (char)(input + 87) : (char)(input + 48);
}

/** Convert hex string to character string 
    outLen = inLen*2 + 1 */
void hex2String(uint8_t *inBytes, uint8_t inLen, char *out) {
    uint8_t index;

    for (index = 0; index < inLen; index++) {
        out[2*index] = hex2Ascii((inBytes[index] & 0xf0) >> 4);
        out[2*index + 1] = hex2Ascii(inBytes[index] & 0x0f);
    }
    out[2*inLen] = '\0';
}

void parse_transfer_tx (unsigned char raw_tx[],
    unsigned int* ux_step_count, 
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char fullAddress[NEM_ADDRESS_LENGTH],
    bool isMultisig) {
    
    //Recipient Address
    uint16_t recipientAddressIndex;
    uint8_t tmpAddress[41];

    //Message
    uint16_t msgSizeIndex;
    uint16_t msgSize;
    uint16_t msgTypeIndex;
    uint8_t msgType;
    uint16_t msgIndex;
    char msg[MAX_PRINT_MESSAGE_LENGTH + 1];

    //Fee
    uint64_t fee;

    //Mosaics
    uint16_t numMosaicIndex;
    uint8_t numMosaic;
    uint16_t offset;
    uint32_t lowMosaicId;
    uint32_t highMosaicId;
    uint64_t amount;
    uint8_t index;

    *ux_step_count = 5;

    //Recipient Address
    SPRINTF(detailName[0], "%s", "Recipient");
    recipientAddressIndex = isMultisig ? 2+2 :2+2+8+8;
    base32_encode(&raw_tx[recipientAddressIndex], 25, &tmpAddress, 40);
    tmpAddress[40] = '\0';
    os_memset(fullAddress, 0, sizeof(fullAddress));
    os_memmove((void *)fullAddress, tmpAddress, 41);

    //Fee
    if (!isMultisig) {
        fee = getUint64(reverseBytes(&raw_tx[2+2], 8));
        SPRINTF(detailName[1], "%s", "Fee");
        print_amount(fee, 6, "xym", &extraInfo[0]);
    }

    //Mosaic
    SPRINTF(detailName[3], "%s", "Mosaic");
    numMosaicIndex = isMultisig ? 2+2+25: 2+2+8+8+25;
    numMosaic = raw_tx[numMosaicIndex];
    SPRINTF(extraInfo[2], "<find %d mosaics>", numMosaic);

    offset  = isMultisig ? 2+2+25+1+2+4 : 2+2+8+8+25+1+2+4;

    for (index = 0; index < numMosaic; index++) {
        *ux_step_count = *ux_step_count + 1;
        //Mosaic ID
        lowMosaicId = getUint32(reverseBytes(&raw_tx[offset], 4));
        highMosaicId = getUint32(reverseBytes(&raw_tx[offset+4], 4));
        //Quantity
        offset +=8;
        amount = getUint64(reverseBytes(&raw_tx[offset], 8));
        offset +=8;

        if ((highMosaicId == 0x747B276C) && (lowMosaicId == 0x30626442)) {
            //xymbol.xym
            SPRINTF(detailName[4+index], "XYM");
            print_amount(amount, 6, "xym", &extraInfo[3+index]); // mosaicDivisibility = 6
        } else {
            //unkown mosaic
            SPRINTF(extraInfo[3+index], "%x%x", highMosaicId, lowMosaicId);
            print_amount(amount*10, 1, "raw", &detailName[4+index]); // mosaicDivisibility = 0
        }
    }

    //Message
    SPRINTF(detailName[2], "%s", "Message");
    msgSizeIndex = isMultisig ? 2+2+25+1: 2+2+8+8+25+1;
    msgSize = getUint16(reverseBytes(&raw_tx[msgSizeIndex], 2));
    msgTypeIndex = isMultisig ? 2+2+25+1+2+4+numMosaic*16: 2+2+8+8+25+1+2+4+numMosaic*16;
    msgIndex = msgTypeIndex + 1;
    msgType = raw_tx[msgTypeIndex];
    if (msgSize <= 1) {
        SPRINTF(extraInfo[1], "%s\0", "<empty msg>");
    } else {
        if(msgType == 0x00) {
            if (msgSize > MAX_PRINT_MESSAGE_LENGTH) {
                uint2Ascii(&raw_tx[msgIndex], MAX_PRINT_MESSAGE_LENGTH, msg);
                SPRINTF(extraInfo[1], "%s...\0", msg);
            } else {
                uint2Ascii(&raw_tx[msgIndex], msgSize, msg);
                SPRINTF(extraInfo[1], "%s\0", msg);
            }
        } else if (msgType == 0x01) {
            SPRINTF(extraInfo[1], "%s\0", "<encrypted msg>");
        }        
    }
}

void parse_mosaic_definition_tx (unsigned char raw_tx[],
    unsigned int* ux_step_count, 
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char extraInfo_0[NEM_ADDRESS_LENGTH],
    bool isMultisig) {

    //Supply amount
    uint16_t supplyAmountIndex;
    uint64_t supplyAmount;

    //Divisibility
    uint16_t divisibilityIndex;
    uint8_t divisibility;

    //Duration
    uint16_t blockDurationIndex;
    uint64_t blockDuration;
    uint8_t day;
    uint8_t hour;
    uint8_t min;

    //MosaicFlags
    uint16_t mosaicFlagsIndex;
    uint8_t mosaicFlags;

    *ux_step_count = 8;

    //Supply amount
    SPRINTF(detailName[0], "%s", "Supply amount");
    supplyAmountIndex = 8+8+4+1+1+50+8;
    supplyAmount = getUint64(reverseBytes(&raw_tx[supplyAmountIndex], 8));
    os_memset(extraInfo_0, 0, sizeof(extraInfo_0));
    print_amount(supplyAmount*10, 1, "\0", extraInfo_0);

    //Divisibility
    SPRINTF(detailName[1], "%s", "Divisibility");
    divisibilityIndex = 8+8+4+1;
    divisibility = raw_tx[divisibilityIndex];
    SPRINTF(extraInfo[0], "%d", divisibility);

    //Duration
    SPRINTF(detailName[2], "%s", "Duration");
    blockDurationIndex = 8;
    blockDuration = getUint64(reverseBytes(&raw_tx[blockDurationIndex], 8));
    if (blockDuration <= 0) {
        SPRINTF(extraInfo[1], "%s", "Unlimited");
    } else {
        day = blockDuration / 5760;
        hour = (blockDuration % 5760) / 240;
        min = (blockDuration % 240) / 4;
        SPRINTF(extraInfo[1], "%d%s%d%s%d%s", day, "d ", hour, "h ", min, "m");
    }

    //Mosaic Flags
    mosaicFlagsIndex = 8+8+4;
    mosaicFlags = raw_tx[mosaicFlagsIndex];

    //Transmittable
    SPRINTF(detailName[3], "%s", "Transferable");
    if (mosaicFlags & 0x02 ) {
        SPRINTF(extraInfo[2], "%s", "Yes");
    } else {
        SPRINTF(extraInfo[2], "%s", "No");
    }

    //Supply multable
    SPRINTF(detailName[4], "%s", "SupplyMultable");
    if (mosaicFlags & 0x01) {
        SPRINTF(extraInfo[3], "%s", "Yes");
    } else {
        SPRINTF(extraInfo[3], "%s", "No");
    }

    //Restrictable
    SPRINTF(detailName[5], "%s", "Restrictable");
    if (mosaicFlags & 0x04) {
        SPRINTF(extraInfo[4], "%s", "Yes");
    } else {
        SPRINTF(extraInfo[4], "%s", "No");
    }
}

void parse_mosaic_supply_change_tx (unsigned char raw_tx[],
    unsigned int* ux_step_count, 
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char fullAddress[NEM_ADDRESS_LENGTH],
    bool isMultisig) {

    //Mosaic ID
    uint16_t mosaicIdIndex;
    uint32_t lowMosaicId;
    uint32_t highMosaicId;

    //Quantity
    uint16_t quantityIndex;
    uint64_t quantity;

    //Supply type
    uint16_t supplyTypeIndex;
    uint8_t supplyType;

    //Fee
    uint64_t fee;

    *ux_step_count = 4;
    //Mosaic ID
    mosaicIdIndex = isMultisig ? 2+2: 2+2+8+8;
    SPRINTF(detailName[1], "%s", "Mosaic ID");
    lowMosaicId = getUint32(reverseBytes(&raw_tx[mosaicIdIndex], 4));
    highMosaicId = getUint32(reverseBytes(&raw_tx[mosaicIdIndex+4], 4));
    SPRINTF(extraInfo[0], "%x%x", highMosaicId, lowMosaicId);
    //Supply Quantity
    quantityIndex = mosaicIdIndex + 8;
    quantity = getUint64(reverseBytes(&raw_tx[28], 8));
    print_amount(quantity*10, 1, "\0", fullAddress);
    //Supply type
    supplyTypeIndex = quantityIndex + 8;
    supplyType = raw_tx[supplyTypeIndex];
    if (supplyType == 0x01) {   //Increase supply
        SPRINTF(detailName[0], "%s", "Increase");
    } else { //Decrease supply 
        SPRINTF(detailName[0], "%s", "Decrease");
    }
    //Fee
    SPRINTF(detailName[2], "%s", "Fee");
    fee = getUint64(reverseBytes(&raw_tx[2+2], 8));
    if (isMultisig) {
        fee += 150000;
    }
    print_amount((uint64_t *)fee, 6, "xym", &extraInfo[1]);
}

void parse_provision_namespace_tx (unsigned char raw_tx[],
    unsigned int* ux_step_count, 
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char extraInfo_0[NEM_ADDRESS_LENGTH],
    bool isMultisig) {

    //Fee
    uint16_t feeIndex;
    uint64_t fee;

    //Type
    uint16_t registrationTypeIndex;
    uint8_t registrationType;

    //Duration
    uint16_t blockDurationIndex;
    uint64_t blockDuration;
    uint8_t day;
    uint8_t hour;
    uint8_t min;

    //Parent Namespace
    uint16_t namespaceIdIndex;
    uint64_t namespaceId;

    //Name
    uint16_t nameSizeIndex;
    uint8_t nameSize;
    
    *ux_step_count = 4;
    //Type; 0: Root namespace, 1: Child namespace
    registrationTypeIndex = 20+8+8;
    registrationType = raw_tx[registrationTypeIndex];

    if (registrationType == 1) {
        //Id, Parent namespace identifier is required for subnamespaces.
        SPRINTF(detailName[2], "%s", "Parent ID");
        namespaceIdIndex = isMultisig ? 2+2+8: 2+2+8+8+8;
        uint32_t lowParentId = getUint32(reverseBytes(&raw_tx[namespaceIdIndex], 4));
        uint32_t highParentId = getUint32(reverseBytes(&raw_tx[namespaceIdIndex+4], 4));
        SPRINTF(extraInfo[1], "%x%x", highParentId, lowParentId);
    } else {
        //Duration
        SPRINTF(detailName[2], "%s", "Duration");
        blockDurationIndex = isMultisig ? 2+28: 2+2+8+8;
        blockDuration = getUint64(reverseBytes(&raw_tx[blockDurationIndex], 8));
        day = blockDuration / 5760;
        hour = (blockDuration % 5760) / 240;
        min = (blockDuration % 240) / 4;
        SPRINTF(extraInfo[1], "%d%s%d%s%d%s", day, "d ", hour, "h ", min, "m");
    }
    
    //Name
    SPRINTF(detailName[0], "%s", "Name");
    nameSizeIndex = 2+2+8+8+8+8+1;
    nameSize = raw_tx[nameSizeIndex];
    uint2Ascii(&raw_tx[nameSizeIndex+1], nameSize, extraInfo_0);

    //Fee
    SPRINTF(detailName[1], "%s", "Fee");
    feeIndex = 2+2;
    fee = getUint64(reverseBytes(&raw_tx[feeIndex], 8));
    print_amount(fee, 6, "xym", &extraInfo[0]);
}

void parse_aggregate_complete_tx (
    unsigned char raw_tx[],
    unsigned int* ux_step_count, 
    char txTypeName[30],
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char extraInfo_0[NEM_ADDRESS_LENGTH],
    bool isMultisig) {

    uint64_t fee = getUint64(reverseBytes(&raw_tx[2+2], 8));
    //txType
    uint16_t txTypeIndex = 104+2;
    uint16_t txType = getUint16(reverseBytes(&raw_tx[txTypeIndex], 2));
    os_memset(txTypeName, 0, sizeof(txTypeName));

    *ux_step_count = 1;

    switch(txType){
        case TRANSFER: //Transfer 
            os_memmove((void *)txTypeName, "Multisig TX", 12);
            //Fee
            SPRINTF(detailName[1], "%s", "Fee");
            print_amount(fee, 6, "xym", &extraInfo[0]);
            parse_transfer_tx (
                raw_tx + txTypeIndex - 2,
                ux_step_count, 
                detailName,
                extraInfo,
                extraInfo_0,
                true
            ); 
            break;
        case MOSAIC_DEFINITION:
            os_memmove((void *)txTypeName, "Create Mosaic", 14);
            //Fee
            SPRINTF(detailName[6], "%s", "Fee");
            print_amount(fee, 6, "xym", &extraInfo[5]);
            parse_mosaic_definition_tx (
                raw_tx + txTypeIndex + 2,
                ux_step_count,
                detailName,
                extraInfo,
                extraInfo_0,
                false);
            break;
        default:
            break;
    }
}

void parse_aggregate_bonded_tx (
    unsigned char raw_tx[],
    unsigned int* ux_step_count, 
    char txTypeName[30],
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char extraInfo_0[NEM_ADDRESS_LENGTH],
    bool isMultisig) {
    *ux_step_count = 1;

    // Fee
    uint64_t fee = getUint64(reverseBytes(&raw_tx[2+2], 8));
    //txType
    uint16_t txTypeIndex = 104+2;
    uint16_t txType = getUint16(reverseBytes(&raw_tx[txTypeIndex], 2));
    os_memset(txTypeName, 0, sizeof(txTypeName));
    switch(txType){
        case TRANSFER: //Transfer 
            os_memmove((void *)txTypeName, "Multisig TX", 12);
            //Fee
            SPRINTF(detailName[1], "%s", "Fee");
            print_amount(fee, 6, "xym", &extraInfo[0]);
            parse_transfer_tx (
                raw_tx + txTypeIndex - 2,
                ux_step_count, 
                detailName,
                extraInfo,
                extraInfo_0,
                true); 
            break;
        case MODIFY_MULTISIG_ACCOUNT:
            os_memmove((void *)txTypeName, "Multisig Modify", 16);
            // Fee
            SPRINTF(detailName[2], "%s", "Fee");            
            print_amount(fee, 6, "xym", &extraInfo[1]);
            parse_multisig_account_modification_tx (
                raw_tx + txTypeIndex - 2,
                ux_step_count, 
                detailName,
                extraInfo,
                extraInfo_0,
                true);
            break;
        default:
            break;
    }
}

void parse_multisig_account_modification_tx (unsigned char raw_tx[],
    unsigned int* ux_step_count,
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char extraInfo_0[NEM_ADDRESS_LENGTH],
    bool isMultisig) {

    //Create or modify a multisig contract.

    //min Removal Delta
    uint16_t minRemovalDeltaIndex;
    int8_t minRemovalDelta;

    //min Approval Delta
    uint16_t minApprovalDeltaIndex;
    int8_t minApprovalDelta;

    //public Key Additions Count
    uint16_t numPublicKeyAdditionsIndex;
    uint8_t numPublicKeyAdditions;
    uint16_t publicKeyAdditionsIndex;

    //public Key Deletions Count
    uint16_t numPublicKeyDeletionsIndex;
    uint8_t numPublicKeyDeletions;
    uint16_t publicKeyDeletionsIndex;
    //cosignatory public key
    char publicKey[65]; 

    //display
    uint8_t displayOffet;

    *ux_step_count = 4;
    //min Removal Delta
    SPRINTF(detailName[0], "%s", "Min Removal");
    minRemovalDeltaIndex = isMultisig ? 2+2: 2+2+8+8;
    minRemovalDelta = raw_tx[minRemovalDeltaIndex];
    os_memset(extraInfo_0, 0, sizeof(extraInfo_0));
    if (minRemovalDelta > 0) {
        print_amount(minRemovalDelta*10, 1, "address(es) added", extraInfo_0);
    } else if (minRemovalDelta < 0) {
        print_amount((~minRemovalDelta+1)*10, 1, "address(es) removed", extraInfo_0);
    } else
    {
        os_memmove((void *)extraInfo_0, "Not change", 41);
    }
    

    //min Approval Delta
    SPRINTF(detailName[1], "%s", "Min Approval");
    minApprovalDeltaIndex = minRemovalDeltaIndex+1;
    minApprovalDelta = raw_tx[minApprovalDeltaIndex];
    if (minApprovalDelta > 0) {
        SPRINTF(extraInfo[0], "%d addr. added", minApprovalDelta);
    } else if (minApprovalDelta < 0) {
        SPRINTF(extraInfo[0], "%d addr. removed", (~minApprovalDelta + 1));
    } else
    {
        SPRINTF(extraInfo[0], "Not change");
    }

    //public Key Additions
    numPublicKeyAdditionsIndex = minApprovalDeltaIndex+1;
    numPublicKeyAdditions = raw_tx[numPublicKeyAdditionsIndex];
    publicKeyAdditionsIndex = numPublicKeyAdditionsIndex+1+1+4;
    displayOffet = 3;
    for (uint8_t index = 0; index < numPublicKeyAdditions; index++) {
        *ux_step_count = *ux_step_count + 1;
        //Top line
        SPRINTF(detailName[index + displayOffet], "%s (%d/%d)", "Add", index+1, numPublicKeyAdditions);
        //Bottom line
        hex2String(&raw_tx[publicKeyAdditionsIndex], 32, publicKey);
        publicKeyAdditionsIndex += 32;
        os_memset(extraInfo[index + displayOffet -1], 0, sizeof(extraInfo[index + displayOffet -1]));
        os_memmove((void *)extraInfo[index + displayOffet -1], publicKey, 6);
        os_memmove((void *)(extraInfo[index + displayOffet -1] + 6), "~", 1);
        os_memmove((void *)(extraInfo[index + displayOffet -1] + 6 + 1), publicKey + 64 - 4, 4);        
    }

    //public Key Deletions Count
    numPublicKeyDeletionsIndex = numPublicKeyAdditionsIndex+1;
    numPublicKeyDeletions = raw_tx[numPublicKeyDeletionsIndex];
    publicKeyDeletionsIndex = publicKeyAdditionsIndex;
    displayOffet = 3 + numPublicKeyAdditions;
    for (uint8_t index = 0; index < numPublicKeyDeletions; index++) {
        *ux_step_count = *ux_step_count + 1;
        //Top line
        SPRINTF(detailName[index + displayOffet], "%s (%d/%d)", "Del", index+1, numPublicKeyDeletions);
        //Bottom line
        hex2String(&raw_tx[publicKeyDeletionsIndex], 32, publicKey);
        publicKeyDeletionsIndex += 32;
        os_memset(extraInfo[index + displayOffet - 1], 0, sizeof(extraInfo[index + displayOffet -1]));
        os_memmove((void *)extraInfo[index + displayOffet -1], publicKey, 6);
        os_memmove((void *)(extraInfo[index + displayOffet -1] + 6), "~", 1);
        os_memmove((void *)(extraInfo[index + displayOffet -1] + 6 + 1), publicKey + 64 - 4, 4);        
    }
}

void parse_hash_lock_tx (
    unsigned char raw_tx[],
    unsigned int* ux_step_count, 
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char extraInfo_0[NEM_ADDRESS_LENGTH],
    bool isMultisig
) {
    //Fee
    uint16_t feeIndex = 4;
    uint64_t fee;

    //Quantity
    uint16_t quantityIndex;
    uint64_t quantity;

    //Duration
    uint16_t blockDurationIndex;
    uint64_t blockDuration;
    uint8_t day;
    uint8_t hour;
    uint8_t min;

    *ux_step_count = 5;

    //Description
    SPRINTF(detailName[0], "%s", "Description");
    os_memset(extraInfo_0, 0, sizeof(extraInfo_0));
    os_memmove((void *)extraInfo_0, "Lock funds to prevent spamming", 41);

    //Fee
    SPRINTF(detailName[1], "%s", "Fee");
    fee = getUint64(reverseBytes(&raw_tx[feeIndex], 8));
    print_amount(fee, 6, "xym", &extraInfo[0]);

    //Quantity
    SPRINTF(detailName[2], "%s", "Locked Quan");
    quantityIndex = isMultisig ? 2+2+8: 2+2+8+8+8;
    quantity = getUint64(reverseBytes(&raw_tx[quantityIndex], 8));
    print_amount(quantity, 6, "xym", &extraInfo[1]);

    //Duration
    SPRINTF(detailName[3], "%s", "Duration");
    blockDurationIndex = isMultisig ? 2+2+8+8: 2+2+8+8+8+8;
    blockDuration = getUint64(reverseBytes(&raw_tx[blockDurationIndex], 8));
    if (blockDuration <= 0) {
        SPRINTF(extraInfo[2], "%s", "Unlimited");
    } else {
        day = blockDuration / 5760;
        hour = (blockDuration % 5760) / 240;
        min = (blockDuration % 240) / 4;
        SPRINTF(extraInfo[2], "%d%s%d%s%d%s", day, "d ", hour, "h ", min, "m");
    }
}