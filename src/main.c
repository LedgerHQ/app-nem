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
#include "ux.h"
#include "os_io_seproxyhal.h"
#include "base32.h"
#include "nemHelpers.h"
#include "nem_tx.h"
#include "glyphs.h"

#include <stdbool.h>
#include <string.h>

ux_state_t G_ux;
bolos_ux_params_t G_ux_params;

unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

uint8_t set_result_get_publicKey(void);

#define CLA 0xE0
#define INS_GET_PUBLIC_KEY 0x02
#define INS_SIGN 0x04
#define INS_GET_APP_CONFIGURATION 0x06
#define P1_CONFIRM 0x01
#define P1_FIRST 0x00
#define P1_MORE 0x80
#define P1_LAST 0x90

#define OFFSET_CLA 0
#define OFFSET_INS 1
#define OFFSET_P1 2
#define OFFSET_P2 3
#define OFFSET_LC 4
#define OFFSET_CDATA 5

/** notification to restart the hash */
unsigned char hashTainted;

/** raw transaction data. */
unsigned char raw_tx[MAX_TX_RAW_LENGTH];

/** current index into raw transaction. */
unsigned int raw_tx_ix;

/** current length of raw transaction. */
unsigned int raw_tx_len;

typedef struct txContent_t {
    uint16_t txType;
} txContent_t;

typedef struct publicKeyContext_t {
    cx_ecfp_public_key_t publicKey;
    uint8_t networkId;
    uint8_t algo;    
    uint8_t nemPublicKey[32];
    char address[40];
} publicKeyContext_t;

typedef struct transactionContext_t {  
    uint8_t pathLength;
    uint8_t networkId;
    uint8_t algo;
    uint32_t bip32Path[MAX_BIP32_PATH];
    uint32_t rawTxLength;
} transactionContext_t;

union {
    publicKeyContext_t publicKeyContext;
    transactionContext_t transactionContext;
} tmpCtx;
txContent_t txContent;


static char txTypeName[30];
static char fullAddress[40 + 1];

//Registers save information to show on the top line of screen
static char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH];
//Registers save information to show on the bottom line of screen
static char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH];

void ui_idle(void);

void sendResponse(uint8_t tx, bool approve) {
    G_io_apdu_buffer[tx++] = approve? 0x90 : 0x69;
    G_io_apdu_buffer[tx++] = approve? 0x00 : 0x85;
    // Send back the response, do not restart the event loop

    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
    // Display back the original UX
    ui_idle();
}

uint8_t set_result_get_publicKey() {
    uint32_t tx = 0;
    uint32_t addressLength = sizeof(tmpCtx.publicKeyContext.address);

    //address
    G_io_apdu_buffer[tx++] = addressLength;
    memmove(G_io_apdu_buffer + tx, tmpCtx.publicKeyContext.address, addressLength);
    tx += addressLength;

    //publicKey
    G_io_apdu_buffer[tx++] = 32;
    memmove(G_io_apdu_buffer + tx, tmpCtx.publicKeyContext.nemPublicKey, 32);
    tx += 32;

    return tx;
}

UX_STEP_NOCB(
    ux_display_public_flow_1_step,
    bb,
    {
        .line1 = "Export",
        .line2 = "NEM Account"
    });

UX_STEP_NOCB(
    ux_display_public_flow_5_step,
    bnnn_paging,
    {
        .title = "Address",
        .text = fullAddress,
    });

UX_STEP_VALID(
    ux_display_public_flow_6_step,
    pb,
    sendResponse(set_result_get_publicKey(), true),
    {
        &C_icon_validate_14,
        "Approve",
    });
UX_STEP_VALID(
    ux_display_public_flow_7_step,
    pb,
    sendResponse(0, false),
    {
        &C_icon_crossmark,
        "Reject",
    });

UX_FLOW(ux_display_public_flow,
    &ux_display_public_flow_1_step,
    &ux_display_public_flow_5_step,
    &ux_display_public_flow_6_step,
    &ux_display_public_flow_7_step
);

unsigned int ux_step_count;

UX_STEP_NOCB(
    ux_idle_flow_1_step,
    pnn,
    {
        &C_icon_NEM,
        "Welcome to",
        "NEM wallet",
    });
UX_STEP_NOCB(
    ux_idle_flow_2_step,
    bn,
    {
        "Version",
        APPVERSION,
    });
UX_STEP_VALID(
    ux_idle_flow_3_step,
    pb,
    os_sched_exit(0),
    {
        &C_icon_dashboard,
        "Quit app",
    });
UX_FLOW(ux_idle_flow,
    &ux_idle_flow_1_step,
    &ux_idle_flow_2_step,
    &ux_idle_flow_3_step,
    FLOW_END_STEP
);

// Display transaction summary
#define TITLE_SIZE 32
#define TEXT_BUFFER_LENGTH 64

char G_transaction_summary_title[TITLE_SIZE];
char G_transaction_summary_text[TEXT_BUFFER_LENGTH];

int transaction_summary_display_item(size_t item_index) {
    if (item_index == 0) {
        os_sched_exit(-1);
    }

    if (item_index == 1) {
        strcpy(G_transaction_summary_title, detailName[0]);
        strcpy(G_transaction_summary_text, fullAddress);
    } else {
        strcpy(G_transaction_summary_title, detailName[item_index - 1]);
        strcpy(G_transaction_summary_text, extraInfo[item_index - 2]);
    }
    return 0;
}

static uint8_t set_result_sign_message() {
    uint8_t privateKeyData[32];
    cx_ecfp_private_key_t privateKey;
    cx_ecfp_public_key_t publicKey;
    uint32_t tx = 0;

    os_perso_derive_node_bip32(CX_CURVE_256K1, tmpCtx.transactionContext.bip32Path, tmpCtx.transactionContext.pathLength, privateKeyData, NULL);    
    cx_ecfp_init_public_key(CX_CURVE_Ed25519, NULL, 0, &publicKey);

    if (tmpCtx.transactionContext.algo == CX_KECCAK) {
        uint8_t privateKeyDataR[32];
        uint8_t j;
        for (j=0; j<32; j++) {
            privateKeyDataR[j] = privateKeyData[31 - j];
        }

        cx_ecfp_init_private_key(CX_CURVE_Ed25519, privateKeyDataR, 32, &privateKey);
        memset(privateKeyDataR, 0, sizeof(privateKeyDataR));
    }else if (tmpCtx.transactionContext.algo == CX_SHA3) {
        cx_ecfp_init_private_key(CX_CURVE_Ed25519, privateKeyData, 32, &privateKey);
    }else{
        THROW(0x6b00);
    }
    
    //signature 128
    G_io_apdu_buffer[tx++] = 128;
    tx = cx_eddsa_sign(&privateKey, 
                       CX_LAST, 
                       tmpCtx.transactionContext.algo, 
                       raw_tx + 21,
                       tmpCtx.transactionContext.rawTxLength, 
                       NULL, 
                       0, 
                       G_io_apdu_buffer, 
                       IO_APDU_BUFFER_SIZE,
                       NULL);
    cx_ecfp_generate_pair2(CX_CURVE_Ed25519, &publicKey, &privateKey, 1, tmpCtx.transactionContext.algo);

    //public 64
    memset(&privateKey, 0, sizeof(privateKey));
    memset(privateKeyData, 0, sizeof(privateKeyData));

  G_io_apdu_buffer[tx++] = 0x90;
  G_io_apdu_buffer[tx++] = 0x00;
  tx = 64;
  return tx;
}

#define MAX_FLOW_STEPS 10
ux_flow_step_t const * flow_steps[MAX_FLOW_STEPS];

UX_STEP_NOCB(
    ux_summary_step,
    bb,
    {
        .line1 = "Confirm",
        .line2 = txTypeName,
    }
);
UX_STEP_NOCB_INIT(
    ux_details_step,
    bn_paging,
    {
        size_t step_index = G_ux.flow_stack[stack_slot].index;
        if (transaction_summary_display_item(step_index)) {
            THROW(0x6984);
        }
    },
    {
        .title = G_transaction_summary_title,
        .text = G_transaction_summary_text,
    }
);
UX_STEP_VALID(
    ux_approve_step,
    pb,
    sendResponse(set_result_sign_message(), true),
    {
        &C_icon_validate_14,
        "Approve",
    });
UX_STEP_VALID(
    ux_reject_step,
    pb,
    sendResponse(0, false),
    {
        &C_icon_crossmark,
        "Reject",
    });

void ui_idle(void) {
  // reserve a display stack slot if none yet
  if(G_ux.stack_count == 0) {
    ux_stack_push();
  }
  ux_flow_init(0, ux_idle_flow, NULL);
}

unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len) {
    switch (channel & ~(IO_FLAGS)) {
    case CHANNEL_KEYBOARD:
        break;

    // multiplexed io exchange over a SPI channel and TLV encapsulated protocol
    case CHANNEL_SPI:
        if (tx_len) {
            io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);

            if (channel & IO_RESET_AFTER_REPLIED) {
                reset();
            }
            return 0; // nothing received from the master so far (it's a tx
                      // transaction)
        } else {
            return io_seproxyhal_spi_recv(G_io_apdu_buffer,
                                          sizeof(G_io_apdu_buffer), 0);
        }

    default:
        THROW(INVALID_PARAMETER);
    }
    return 0;
}

void handleGetPublicKey(uint8_t p1, uint8_t p2, uint8_t *dataBuffer,
                        uint16_t dataLength, unsigned int *flags,
                        size_t *tx) {
    UNUSED(dataLength);
    uint8_t privateKeyData[32];
    uint32_t bip32Path[MAX_BIP32_PATH];
    uint32_t i;
    uint8_t bip32PathLength = *(dataBuffer++);
    cx_ecfp_private_key_t privateKey;

    // bip32PathLength should be 5
    if (bip32PathLength != MAX_BIP32_PATH) {
        THROW(0x6a80);
    }

    if (p1 != P1_CONFIRM) {
        THROW(0x6B00);
    }

    for (i = 0; i < bip32PathLength; i++) {
        bip32Path[i] = (dataBuffer[0] << 24) | (dataBuffer[1] << 16) |
                       (dataBuffer[2] << 8) | (dataBuffer[3]);
        dataBuffer += 4;
    }

    tmpCtx.publicKeyContext.networkId = readNetworkIdFromBip32path(bip32Path);
    if (tmpCtx.publicKeyContext.networkId == NEM_MAINNET || tmpCtx.publicKeyContext.networkId == NEM_TESTNET) {
        tmpCtx.publicKeyContext.algo = CX_KECCAK;
    } else {
        tmpCtx.publicKeyContext.algo = CX_SHA3;
    }
    
    os_perso_derive_node_bip32(CX_CURVE_256K1, bip32Path, bip32PathLength, privateKeyData, NULL);

    if (tmpCtx.publicKeyContext.algo == CX_SHA3) {
        cx_ecfp_init_private_key(CX_CURVE_Ed25519, privateKeyData, 32, &privateKey);
    }else if (tmpCtx.publicKeyContext.algo == CX_KECCAK) { //CX_KECCAK
        //reverse privateKey
        uint8_t privateKeyDataR[32];
        uint8_t j;
        for (j=0; j<32; j++) {
            privateKeyDataR[j] = privateKeyData[31 - j];
        }
        cx_ecfp_init_private_key(CX_CURVE_Ed25519, privateKeyDataR, 32, &privateKey);
        memset(privateKeyDataR, 0, sizeof(privateKeyDataR));
    }else{ 
        THROW(0x6a80);
    }
    cx_ecfp_generate_pair2(CX_CURVE_Ed25519, &tmpCtx.publicKeyContext.publicKey, &privateKey, 1, tmpCtx.publicKeyContext.algo);

    memset(privateKeyData, 0, sizeof(privateKeyData));
    memset(&privateKey, 0, sizeof(privateKey));

    to_nem_public_key_and_address(
                                  &tmpCtx.publicKeyContext.publicKey, 
                                  tmpCtx.publicKeyContext.networkId, 
                                  tmpCtx.publicKeyContext.algo, 
                                  tmpCtx.publicKeyContext.nemPublicKey,
                                  tmpCtx.publicKeyContext.address
                                  );

    memset(fullAddress, 0, sizeof(fullAddress));
    memmove((void *)fullAddress, tmpCtx.publicKeyContext.address, 40);

    // prepare for a UI based reply//
    ux_flow_init(0, ux_display_public_flow, NULL);
    *flags |= IO_ASYNCH_REPLY;
}


void display_tx(uint8_t *tx_data, uint16_t dataLength,
                volatile unsigned int *flags, volatile unsigned int *tx ) {
    UNUSED(tx);
    uint32_t i;

    tmpCtx.transactionContext.pathLength = tx_data[0];
    if (tmpCtx.transactionContext.pathLength != MAX_BIP32_PATH) {
        THROW(0x6a80);
    }

    for (i = 0; i < tmpCtx.transactionContext.pathLength; i++) {
        tmpCtx.transactionContext.bip32Path[i] =
            (tx_data[1 + i*4] << 24) | (tx_data[2 + i*4] << 16) |
            (tx_data[3 + i*4] << 8) | (tx_data[4 + i*4]);
    }

    tmpCtx.transactionContext.networkId = readNetworkIdFromBip32path(tmpCtx.transactionContext.bip32Path);
    if (tmpCtx.transactionContext.networkId == NEM_MAINNET || tmpCtx.transactionContext.networkId == NEM_TESTNET) {
        tmpCtx.transactionContext.algo = CX_KECCAK;
    } else {
        tmpCtx.transactionContext.algo = CX_SHA3;
    }
    
    // Load dataLength of tx
    tmpCtx.transactionContext.rawTxLength = dataLength - 21; 
    
    //NEM_MAINNET || NEM_TESTNET
    //txType
    uint32_t txType = get_uint32_le(&tx_data[21]);
    txContent.txType = (uint16_t)txType;

    // uint32_t txVersion = get_uint32_le(&tx_data[21+4]);

    //Distance index: use for calculating the inner index of multisig tx
    uint8_t disIndex; 

    int err = -1;
    switch(txContent.txType){
        case NEMV1_TRANSFER: //Transfer 
            disIndex = 21; 
            SPRINTF(txTypeName, "%s", "Transfer TX");
            err = parse_transfer_tx (tx_data + disIndex,
                tmpCtx.transactionContext.rawTxLength,
                &ux_step_count, 
                detailName,
                extraInfo,
                fullAddress,
                false
            ); 
            break;
        case NEMV1_MULTISIG_MODIFICATION:
            disIndex = 21;
            SPRINTF(txTypeName, "%s", "Convert to Multisig");
            err = parse_aggregate_modification_tx (tx_data + disIndex,
                tmpCtx.transactionContext.rawTxLength,
                &ux_step_count, 
                detailName,
                extraInfo,
                fullAddress,
                false,
                tmpCtx.transactionContext.networkId
            ); 
            break;
        case NEMV1_MULTISIG_SIGNATURE:
            SPRINTF(txTypeName, "%s", "Mulisig signature");
            disIndex = 21;
            err = parse_multisig_signature_tx (tx_data + disIndex,
                tmpCtx.transactionContext.rawTxLength,
                &ux_step_count, 
                detailName,
                extraInfo,
                fullAddress
            );
            break;
        case NEMV1_MULTISIG_TRANSACTION:
            SPRINTF(txTypeName, "%s", "Mulisig TX");
            disIndex = 21+4+4+4+4+32+8+4+4;
            err = parse_multisig_tx (tx_data + disIndex,
                tmpCtx.transactionContext.rawTxLength - (4+4+4+4+32+8+4+4),
                &ux_step_count, 
                detailName,
                extraInfo,
                fullAddress,
                tmpCtx.transactionContext.networkId
            );
            break;
        case NEMV1_PROVISION_NAMESPACE:
            disIndex = 21;
            SPRINTF(txTypeName, "%s", "Namespace TX");
            err = parse_provision_namespace_tx (tx_data + disIndex,
                tmpCtx.transactionContext.rawTxLength,
                &ux_step_count, 
                detailName,
                extraInfo,
                fullAddress,
                false
            );
            break;                
        case NEMV1_MOSAIC_DEFINITION:
            disIndex = 21;
            SPRINTF(txTypeName, "%s", "Create Mosaic");
            err = parse_mosaic_definition_tx (tx_data + disIndex,
                tmpCtx.transactionContext.rawTxLength,
                &ux_step_count, 
                detailName,
                extraInfo,
                fullAddress,
                false
            );
            break; 
        case NEMV1_MOSAIC_SUPPLY:
            disIndex = 21;
            SPRINTF(txTypeName, "%s", "Mosaic Supply");
            err = parse_mosaic_supply_tx (tx_data + disIndex,
                tmpCtx.transactionContext.rawTxLength,
                &ux_step_count, 
                detailName,
                extraInfo,
                fullAddress,
                false
            );
            break;         
        default:
            SPRINTF(txTypeName, "tx type %d", txContent.txType); 
            break;    
    }
    if (err) {
        THROW(0x6700);
    }

    size_t num_flow_steps = 0;
    flow_steps[num_flow_steps++] = &ux_summary_step;
    for (size_t i = 0; i < ux_step_count - 1; i++) {
        flow_steps[num_flow_steps++] = &ux_details_step;
    }

    flow_steps[num_flow_steps++] = &ux_approve_step;
    flow_steps[num_flow_steps++] = &ux_reject_step;
    flow_steps[num_flow_steps++] = FLOW_END_STEP;

    ux_flow_init(0, flow_steps, NULL);
    *flags |= IO_ASYNCH_REPLY;
}

void handleGetAppConfiguration(uint8_t p1,
                               uint8_t p2,
                               const uint8_t *data,
                               size_t length,
                               unsigned int *flags,
                               size_t *tx);

void handleSign(uint8_t p1, uint8_t p2, const uint8_t *data, size_t length, unsigned int *flags, size_t *tx) {
    // check the third byte (0x02) for the instruction subtype.
    if (p1 == P1_FIRST || p1 == P1_LAST) {
      clean_raw_tx(raw_tx);
      hashTainted = 1;
    }

    // if this is the first transaction part, reset the hash and all the other temporary variables.
    if (hashTainted) {
        hashTainted = 0;
        raw_tx_ix = 0;
        raw_tx_len = 0;
    }

    // move the contents of the buffer into raw_tx, and update raw_tx_ix to the end of the buffer, 
    // to be ready for the next part of the tx.
    unsigned char * out = raw_tx + raw_tx_ix;
    if (raw_tx_ix + length > MAX_TX_RAW_LENGTH) {
        hashTainted = 1;
        THROW(0x6D08);
    }
    memmove(out, data, length);
    raw_tx_ix += length;

    // if this is the last part of the transaction, parse the transaction into human readable text, and display it.
    if (p1 == P1_MORE || p1 == P1_LAST) {
        raw_tx_len = raw_tx_ix;
        raw_tx_ix = 0;

        // parse the transaction into human readable text.
        display_tx(raw_tx, raw_tx_len, flags, tx);
    } else {
        // continue reading the tx
        THROW(0x9000);  
    }
}

void handleGetAppConfiguration(uint8_t p1,
                               uint8_t p2,
                               const uint8_t *data,
                               size_t length,
                               unsigned int *flags,
                               size_t *tx) {
  UNUSED(p1);
  UNUSED(p2);
  UNUSED(data);
  UNUSED(length);
  UNUSED(flags);
  G_io_apdu_buffer[0] = 0x00;
  G_io_apdu_buffer[1] = LEDGER_MAJOR_VERSION;
  G_io_apdu_buffer[2] = LEDGER_MINOR_VERSION;
  G_io_apdu_buffer[3] = LEDGER_PATCH_VERSION;
  *tx = 4;
  THROW(0x9000);
}

int handle_apdu(unsigned int *flags, size_t rx, size_t *tx) {
  // if the buffer doesn't start with the magic byte, return an error.
  if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
    hashTainted = 1;
    THROW(0x6E00);
  }
  if (rx < OFFSET_CDATA || (rx != G_io_apdu_buffer[OFFSET_LC] + OFFSET_CDATA)) {
    hashTainted = 1;
    THROW(0x6E00);
  }

  // check the second byte (0x01) for the instruction.
  switch (G_io_apdu_buffer[OFFSET_INS]) {

  case INS_GET_PUBLIC_KEY:
    handleGetPublicKey(G_io_apdu_buffer[OFFSET_P1],
                       G_io_apdu_buffer[OFFSET_P2],
                       G_io_apdu_buffer + OFFSET_CDATA,
                       G_io_apdu_buffer[OFFSET_LC], flags, tx);
    break;

    //Sign a transaction
  case INS_SIGN:
    handleSign(G_io_apdu_buffer[OFFSET_P1],
               G_io_apdu_buffer[OFFSET_P2],
               G_io_apdu_buffer + OFFSET_CDATA,
               G_io_apdu_buffer[OFFSET_LC],
               flags,
               tx);
    break;

  case INS_GET_APP_CONFIGURATION:
    handleGetAppConfiguration(G_io_apdu_buffer[OFFSET_P1],
                              G_io_apdu_buffer[OFFSET_P2],
                              G_io_apdu_buffer + OFFSET_CDATA,
                              G_io_apdu_buffer[OFFSET_LC],
                              flags,
                              tx);
    break;

  default:THROW(0x6D00);
  }
  return 0;
}

void nem_main(void) {
    size_t rx = 0, tx = 0;
    unsigned int flags = 0;

    // DESIGN NOTE: the bootloader ignores the way APDU are fetched. The only
    // goal is to retrieve APDU.
    // When APDU are to be fetched from multiple IOs, like NFC+USB+BLE, make
    // sure the io_event is called with a
    // switch event, before the apdu is replied to the bootloader. This avoid
    // APDU injection faults.
    for (;;) {
        volatile unsigned short sw = 0;

        BEGIN_TRY {
            TRY {
                rx = tx;
                tx = 0; // ensure no race in catch_other if io_exchange throws
                        // an error
                rx = io_exchange(CHANNEL_APDU | flags, rx);
                flags = 0;

                // no apdu received, well, reset the session, and reset the
                // bootloader configuration
                if (rx == 0) {
                    hashTainted = 1;
                    THROW(0x6982);
                }

                handle_apdu(&flags, rx, &tx);
            }
            CATCH_OTHER(e) {
                switch (e & 0xF000) {
                case 0x6000:
                    // Wipe the transaction context and report the exception
                    sw = e;
                    memset(&txContent, 0, sizeof(txContent));
                    break;
                case 0x9000:
                    // All is well
                    sw = e;
                    break;
                default:
                    // Internal error
                    sw = 0x6800 | (e & 0x7FF);
                    break;
                }
                // Unexpected exception => report
                G_io_apdu_buffer[tx] = sw >> 8;
                G_io_apdu_buffer[tx + 1] = sw;
                tx += 2;
            }
            FINALLY {
            }
        }
        END_TRY;
    }

    // return_to_dashboard:
    return;
}

// override point, but nothing more to do
void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t *)element);
}

unsigned char io_event(unsigned char channel) {
    // nothing done with the event, throw an error on the transport layer if
    // needed

    // can't have more than one tag in the reply, not supported yet.
    switch (G_io_seproxyhal_spi_buffer[0]) {
    case SEPROXYHAL_TAG_FINGER_EVENT:
        UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
        break;

    case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
        UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
        break;

    case SEPROXYHAL_TAG_STATUS_EVENT:
        if (G_io_apdu_media == IO_APDU_MEDIA_USB_HID &&
            !(U4BE(G_io_seproxyhal_spi_buffer, 3) &
              SEPROXYHAL_TAG_STATUS_EVENT_FLAG_USB_POWERED)) {
            THROW(EXCEPTION_IO_RESET);
        }
    // no break is intentional
    default:
        UX_DEFAULT_EVENT();
        break;

    case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
        UX_DISPLAYED_EVENT({});
        break;

    case SEPROXYHAL_TAG_TICKER_EVENT:
        UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {});
        break;
    }

    // close the event if not done previously (by a display or whatever)
    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }

    // command has been processed, DO NOT reset the current APDU transport
    return 1;
}

void app_exit(void) {
    BEGIN_TRY_L(exit) {
        TRY_L(exit) {
            os_sched_exit(-1);
        }
        FINALLY_L(exit) {
        }
    }
    END_TRY_L(exit);
}

__attribute__((section(".boot"))) int main(void) {
    // exit critical section
    __asm volatile("cpsie i");

    raw_tx_ix = 0;
	hashTainted = 1;

    // ensure exception will work as planned
    os_boot();

    for (;;) {
	    memset(&txContent, 0, sizeof(txContent));
	
        UX_INIT();
        BEGIN_TRY {
            TRY {
                io_seproxyhal_init();

                USB_power(1);

                ui_idle();
                nem_main();
            }
                CATCH(EXCEPTION_IO_RESET) {
                    // reset IO and UX
                    continue;
                }
                CATCH_ALL {
                    break;
                }
            FINALLY {
            }
        }
        END_TRY;
    }
    app_exit();

    return 0;
}
