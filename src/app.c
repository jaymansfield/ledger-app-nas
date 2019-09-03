/*******************************************************************************
*   Ledger Blue
*   (c) 2016 Ledger
*   (c) 2019 Nebulas
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

#include "app.h"
#include "view.h"
#include "lib/transaction.h"
#include "signature.h"

#include <cx.h>
#include <os_io_seproxyhal.h>
#include <os.h>

#include <string.h>

/*
extern unsigned long _stack;

#define STACK_CANARY (*((volatile uint32_t*) &_stack))

void init_canary() {
    STACK_CANARY = 0xDEADBEEF;
}

void check_canary() {
    if (STACK_CANARY != 0xDEADBEEF)
    {
        PRINTF("OUT OF BOUNDS!\n");
        THROW(EXCEPTION_OVERFLOW);
    }
}*/

unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

void app_init() {
    io_seproxyhal_init();

    USB_power(0);
    USB_power(1);

    view_idle(0);
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

unsigned char io_event(unsigned char channel) {
    switch (G_io_seproxyhal_spi_buffer[0]) {
        case SEPROXYHAL_TAG_FINGER_EVENT: //
            UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
            break;

        case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT: // for Nano S
            UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
            break;

        case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
            if (!UX_DISPLAYED())
                UX_DISPLAYED_EVENT();
            break;

        case SEPROXYHAL_TAG_TICKER_EVENT: { //
            UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {
                    if (UX_ALLOWED) {
                        UX_REDISPLAY();
                    }
            });
            break;
        }

            // unknown events are acknowledged
        default:
            UX_DEFAULT_EVENT();
            break;
    }

    // close the event if not done previously (by a display or whatever)
    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }
    return 1; // DO NOT reset the current APDU transport
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

bool extractBip32(uint8_t *depth, uint32_t path[10], uint32_t rx, uint32_t offset) {
    if (rx < offset + 1) {
        return 0;
    }

    *depth = G_io_apdu_buffer[offset];
    const uint16_t req_offset = 4 * *depth + 1 + offset;

    if (rx < req_offset || *depth > 10) {
        return 0;
    }
    memcpy(path, G_io_apdu_buffer + offset + 1, *depth * 4);
    return 1;
}

bool process_chunk(volatile uint32_t *tx, uint32_t rx, bool getBip32) {
    int packageIndex = G_io_apdu_buffer[OFFSET_PCK_INDEX];
    int packageCount = G_io_apdu_buffer[OFFSET_PCK_COUNT];

    uint16_t offset = OFFSET_DATA;
    if (rx < offset) {
        THROW(APDU_CODE_DATA_INVALID);
    }

    if (packageIndex == 1) {
        transaction_initialize();
        transaction_reset();
        if (getBip32) {
            if (!extractBip32(&bip32_depth, bip32_path, rx, OFFSET_DATA)) {
                THROW(APDU_CODE_DATA_INVALID);
            }
            return packageIndex == packageCount;
        }
    }
    if (transaction_append(&(G_io_apdu_buffer[offset]), rx - offset) != rx - offset) {
        THROW(APDU_CODE_OUTPUT_BUFFER_TOO_SMALL);
    }
    return packageIndex == packageCount;
}

int getTxData(
        char *title, int max_title_length,
        char *key, int max_key_length,
        char *value, int max_value_length,
        int page_index,
        int chunk_index,
        int *page_count_out,
        int *chunk_count_out) {

    *page_count_out = 7;

    snprintf(title, max_title_length, "Txn Details %02d/%02d", page_index + 1, *page_count_out);

    // The API is different so we need to temporarily send chunk_index => chunk_count_out
    *chunk_count_out = transaction_get_display_key_value(
            key, max_key_length,
            value, max_value_length,
            page_index,
            chunk_index);

    return 0;
}

int getAddrData(
        char *title, int max_title_length,
        char *key, int max_key_length,
        char *value, int max_value_length,
        int page_index,
        int chunk_index,
        int *page_count_out,
        int *chunk_count_out) {

    *page_count_out = 1;
    *chunk_count_out = 1;

    snprintf(title, max_title_length, "Export Public Key");
    snprintf(key, max_key_length, "Wallet Address");

    //check_canary();
    get_address(value);
    //check_canary();

    return 0;
}

void addr_accept() {
    cx_ecfp_public_key_t publicKey;
    getPubKey(&publicKey);

    os_memmove(G_io_apdu_buffer, publicKey.W, 65);
    int pos = 65;

    set_code(G_io_apdu_buffer, pos, APDU_CODE_OK);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, pos + 2);
    view_idle(0);
}

void addr_reject() {
    set_code(G_io_apdu_buffer, 0, APDU_CODE_COMMAND_NOT_ALLOWED);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    view_idle(0);
}

void reject_transaction() {
    set_code(G_io_apdu_buffer, 0, APDU_CODE_COMMAND_NOT_ALLOWED);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    view_idle(0);
}

void sign_transaction() {
    PRINTF("sign_transaction - start\n");

    //check_canary();
    cx_ecfp_private_key_t privateKey;

    {
        cx_ecfp_public_key_t publicKey;
        uint8_t privateKeyData[32];

        os_perso_derive_node_bip32(CX_CURVE_256K1, bip32_path, bip32_depth, privateKeyData, NULL);

        //uint32_t bip32Path[] = {44 | 0x80000000, 2718 | 0x80000000, 0 | 0x80000000, 0x80000000, 0x80000000};
        //os_perso_derive_node_bip32_seed_key(HDW_NORMAL, CX_CURVE_256K1, bip32Path, 5, privateKeyData, NULL, NULL, 0);

        PRINTF("GET KEY\n");
        keys_secp256k1(&publicKey, &privateKey, privateKeyData);

        PRINTF("GOT KEY  %d\n", IO_APDU_BUFFER_SIZE);
        memset(privateKeyData, 0, 32);
    }

    unsigned int length = 0;

    {
        uint8_t hash[32];
        transaction_get_hash(hash);

        length = sign_secp256k1(hash, &privateKey);
    }

    //check_canary();
    set_code(G_io_apdu_buffer, length, APDU_CODE_OK);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, length + 2);
    view_idle(0);
}

void app_main() {
    volatile unsigned int rx = 0;
    volatile unsigned int tx = 0;
    volatile unsigned int flags = 0;

    //init_canary();

    for (;;) {
        volatile uint16_t sw = 0;

        BEGIN_TRY;
        {
            TRY;
            {
                rx = tx;
                tx = 0; // ensure no race in catch_other if io_exchange throws
                        // an error
                rx = io_exchange(CHANNEL_APDU | flags, rx);
                flags = 0;

                // no apdu received, well, reset the session, and reset the
                // bootloader configuration
                if (rx == 0) {
                    THROW(0x6982);
                }

                if (G_io_apdu_buffer[0] != 0x6e) {
                    THROW(0x6E00);
                }

                switch (G_io_apdu_buffer[1]) {
                    case 0x00: { // get app info
                        PRINTF("START - GET APP INFO\n");
                        G_io_apdu_buffer[0] = 0;
                        G_io_apdu_buffer[1] = LEDGER_MAJOR_VERSION;
                        G_io_apdu_buffer[2] = LEDGER_MINOR_VERSION;
                        G_io_apdu_buffer[3] = LEDGER_PATCH_VERSION;
                        tx += 4;
                        PRINTF("END - GET APP INFO\n");
                        THROW(APDU_CODE_OK);
                        break;
                    }

                    case 0x01: { // get public key
                        PRINTF("START - GET PUB KEY\n");
                        if (!extractBip32(&bip32_depth, bip32_path, rx, OFFSET_DATA)) {
                            THROW(APDU_CODE_DATA_INVALID);
                        }

                        PRINTF("BIP32_PATH:%.*H\n", 10, bip32_path);
                        PRINTF("BIP32_DEPTH:%d\n", bip32_depth);

                        cx_ecfp_public_key_t publicKey;
                        getPubKey(&publicKey);

                        os_memmove(G_io_apdu_buffer, publicKey.W, 65);
                        tx += 65;

                        PRINTF("END - GET PUB KEY\n");
                        THROW(APDU_CODE_OK);
                        break;
                    }

                    case 0x03: { // get public key (requires user confirmation)
                        PRINTF("START - VERIFY PUB KEY\n");
                        if (!extractBip32(&bip32_depth, bip32_path, rx, OFFSET_DATA)) {
                            THROW(APDU_CODE_DATA_INVALID);
                        }

                        PRINTF("BIP32_PATH:%.*H\n", 10, bip32_path);
                        PRINTF("BIP32_DEPTH:%d\n", bip32_depth);

                        view_set_event_handlers(&getAddrData, &addr_accept, &addr_reject);
                        view_addr_show(0);

                        flags |= IO_ASYNCH_REPLY;
                        break;
                    }

                    case 0x02: { // sign transaction
                        if (!process_chunk(tx, rx, true))
                            THROW(APDU_CODE_OK);

                        const char *error_msg = transaction_parse();
                        if (error_msg != NULL) {
                            PRINTF("%s\n", error_msg);
                            int error_msg_length = strlen(error_msg);
                            os_memmove(G_io_apdu_buffer, error_msg, error_msg_length);
                            tx += (error_msg_length);
                            THROW(APDU_CODE_BAD_KEY_HANDLE);
                        }

                        view_set_event_handlers(&getTxData, &sign_transaction, &reject_transaction);
                        view_tx_show(0);

                        flags |= IO_ASYNCH_REPLY;
                        break;
                    }

                    default:
                        THROW(0x6D00);
                        break;
                }
            }
            CATCH_OTHER(e);
            {
                switch (e & 0xF000) {
                    case 0x6000:
                    case 0x9000:
                        sw = e;
                        break;
                    default:
                        sw = 0x6800 | (e & 0x7FF);
                        break;
                }
                G_io_apdu_buffer[tx] = sw >> 8;
                G_io_apdu_buffer[tx + 1] = sw;
                tx += 2;
            }
            FINALLY;
            {}
        }
        END_TRY;
    }

//return_to_dashboard:
    return;
}
