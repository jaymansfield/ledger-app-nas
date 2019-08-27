/*******************************************************************************
*   (c) 2018 ZondaX GmbH
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

#include "transaction.h"
#include "../view.h"
#include "apdu_codes.h"
#include "json_parser.h"
#include "transaction_parser.h"
#include "buffering.h"

#include "xrpBase58.h"

// Ram
#define RAM_BUFFER_SIZE 416
uint8_t ram_buffer[RAM_BUFFER_SIZE];

// Flash
#define FLASH_BUFFER_SIZE 3072
typedef struct {
    uint8_t buffer[FLASH_BUFFER_SIZE];
} storage_t;

const storage_t N_appdata_impl __attribute__ ((aligned(64)));
#define N_appdata (*(volatile storage_t *)PIC(&N_appdata_impl))

parsed_json_t parsed_transaction;
hash_context hash_ctx;

void update_ram(buffer_state_t *buffer, uint8_t *data, int size) {
    os_memmove(buffer->data + buffer->pos, data, size);
}

void update_flash(buffer_state_t *buffer, uint8_t *data, int size) {
    nvm_write((void *) buffer->data + buffer->pos, data, size);
}

void transaction_initialize() {
    append_buffer_delegate update_ram_delegate = &update_ram;
    append_buffer_delegate update_flash_delegate = &update_flash;

    buffering_init(
        ram_buffer,
        sizeof(ram_buffer),
        update_ram_delegate,
        N_appdata.buffer,
        sizeof(N_appdata.buffer),
        update_flash_delegate
    );
}

void transaction_reset() {
    buffering_reset();
}

uint32_t transaction_append(unsigned char *buffer, uint32_t length) {
    return buffering_append(buffer, length);
}

uint32_t transaction_get_buffer_length() {
    return buffering_get_buffer()->pos;
}

uint8_t *transaction_get_buffer() {
    return buffering_get_buffer()->data;
}

const char* transaction_parse() {
    const char *transaction_buffer = (const char *) transaction_get_buffer();
    const char* error_msg = json_parse_s(&parsed_transaction, transaction_buffer, transaction_get_buffer_length());
    if (error_msg != NULL) {
        return error_msg;
    }
    error_msg = json_validate(&parsed_transaction, transaction_buffer);
    if (error_msg != NULL) {
        return error_msg;
    }

    parsing_context_t context;
    context.tx = transaction_buffer;
    context.parsed_tx = &parsed_transaction;

    set_parsing_context(context);
    set_copy_delegate(&os_memmove);
    return NULL;
}

uint32_t char_to_int32(const unsigned char *text)
{
    uint32_t number=0;

    for(; *text; text++)
    {
        char digit=*text-'0';
        number=(number*10)+digit;
    }

    return number;
}

uint64_t char_to_int64(const unsigned char *text)
{
    uint64_t number=0;

    for(; *text; text++)
    {
        char digit=*text-'0';
        number=(number*10)+digit;
    }

    return number;
}

void writeHexAmount32(int32_t amount, unsigned char *buffer, int offset) {
	*(buffer + 3 + offset) = ((amount ) & 0xff);
	*(buffer + 2 + offset) = ((amount >> 8) & 0xff);
	*(buffer + 1 + offset) = ((amount >> 16) & 0xff);
	*(buffer + offset) = ((amount >> 24) & 0xff);
}

void writeHexAmountBE(int64_t amount, unsigned char *buffer, int offset) {
	*(buffer + 7 + offset) = (amount & 0xff);
	*(buffer + 6 + offset) = ((amount >> 8) & 0xff);
	*(buffer + 5 + offset) = ((amount >> 16) & 0xff);
	*(buffer + 4 + offset) = ((amount >> 24) & 0xff);
	*(buffer + 3 + offset) = ((amount >> 32) & 0xff);
	*(buffer + 2 + offset) = ((amount >> 40) & 0xff);
	*(buffer + 1 + offset) = ((amount >> 48) & 0xff);
	*(buffer + offset) = ((amount >> 56) & 0xff);
}

uint8_t hexStringToByteArray(unsigned char* string, int slength, uint8_t* byteArray) {
    if(string == NULL)
       return 0;

    size_t index = 0;

    unsigned char *quotePtr;
    for (quotePtr = string; *quotePtr; quotePtr++){
        char c = *quotePtr;
        int value = 0;
        if(c >= '0' && c <= '9')
          value = (c - '0');
        else if (c >= 'A' && c <= 'F')
          value = (10 + (c - 'A'));
        else if (c >= 'a' && c <= 'f')
          value = (10 + (c - 'a'));

        byteArray[(index/2)] += value << (((index + 1) % 2) * 4);
        index++;
    }
    return 1;
}

int get_json_element(unsigned char *key, int key_length, int token_index, const char *transaction_buffer) {
    int key_size = parsed_transaction.Tokens[token_index].end - parsed_transaction.Tokens[token_index].start;
    const char *address_ptr = transaction_buffer + parsed_transaction.Tokens[token_index].start;

    if (key_size >= key_length) { // don't allow overflows
        key_size = key_length - 1;
    }
    memcpy(key, address_ptr, key_size);
    key[key_size] = '\0';
    return key_size;
}

void transaction_get_hash(uint8_t *output) {
    memset(&hash_ctx, 0, sizeof(hash_ctx));

    //cx_sha3_t sha;
    cx_sha3_init(&hash_ctx.sha, 256);

    PRINTF("GENERATING HASH\n");

    const char *transaction_buffer = (const char *) transaction_get_buffer();

    int token_index = 0;
    unsigned char address_temp[36];

    /////////////////////////////////////////////
    // from address

    os_memset(address_temp, 0, sizeof(address_temp));

    token_index = object_get_value(ROOT_TOKEN_INDEX, "f", &parsed_transaction, transaction_buffer);
    get_json_element(address_temp, sizeof(address_temp), token_index, transaction_buffer);
    //PRINTF("ADDRESS_FROM:%s\n", address_temp);

    unsigned char address_decoded[27];
    unsigned char outLen = 27;

    xrp_decode_base58((unsigned char *)address_temp, 35, (unsigned char *)address_decoded, outLen);
    PRINTF("ADDRESS_DECODED: %.*h\n", 26, address_decoded);

    cx_hash(&hash_ctx.sha.header, 0, address_decoded, 26, NULL, 0);

    os_memset(address_temp, 0, sizeof(address_temp));
    os_memset(address_decoded, 0, sizeof(address_decoded));

    /////////////////////////////////////////////
    // to address

    token_index = object_get_value(ROOT_TOKEN_INDEX, "a", &parsed_transaction, transaction_buffer);
    get_json_element(address_temp, sizeof(address_temp), token_index, transaction_buffer);
    //PRINTF("ADDRESS_TO:%s\n", address_temp);

    xrp_decode_base58((unsigned char *)address_temp, 35, (unsigned char *)address_decoded, outLen);
    PRINTF("ADDRESS_DECODED: %.*h\n", 26, address_decoded);

    cx_hash(&hash_ctx.sha.header, 0, address_decoded, 26, NULL, 0);

    os_memset(address_temp, 0, sizeof(address_temp));
    os_memset(address_decoded, 0, sizeof(address_decoded));

    /////////////////////////////////////////////
    // value

    token_index = object_get_value(ROOT_TOKEN_INDEX, "v", &parsed_transaction, transaction_buffer);
    get_json_element(address_temp, sizeof(address_temp), token_index, transaction_buffer);
    //PRINTF("VALUE:%s\n", address_temp);

    uint64_t uint64 = char_to_int64(address_temp);

    unsigned char value_string128[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    writeHexAmountBE(uint64, (unsigned char *)value_string128, 8);
    PRINTF("VAL HEX:         %.*h\n", 16, value_string128);

    cx_hash(&hash_ctx.sha.header, 0, value_string128, 16, NULL, 0);

    os_memset(address_temp, 0, sizeof(address_temp));
    os_memset(value_string128, 0, sizeof(value_string128));

    /////////////////////////////////////////////
    // nonce

    token_index = object_get_value(ROOT_TOKEN_INDEX, "n", &parsed_transaction, transaction_buffer);
    get_json_element(address_temp, sizeof(address_temp), token_index, transaction_buffer);
    //PRINTF("NONCE:%s\n", address_temp);

    uint64 = char_to_int64(address_temp);

    unsigned char value_string64[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    writeHexAmountBE(uint64, (unsigned char *)value_string64, 0);

    PRINTF("NONCE HEX:       %.*h\n", 8, value_string64);

    cx_hash(&hash_ctx.sha.header, 0, value_string64, 8, NULL, 0);

    os_memset(address_temp, 0, sizeof(address_temp));
    os_memset(value_string64, 0, sizeof(value_string64));

    /////////////////////////////////////////////
    // timestamp

    token_index = object_get_value(ROOT_TOKEN_INDEX, "t", &parsed_transaction, transaction_buffer);
    get_json_element(address_temp, sizeof(address_temp), token_index, transaction_buffer);
    //PRINTF("TIMESTAMP:%s\n", address_temp);

    uint64 = char_to_int64(address_temp);

    writeHexAmountBE(uint64, (unsigned char *)value_string64, 0);
    PRINTF("TIMESTAMP HEX:   %.*h\n", 8, value_string64);

    cx_hash(&hash_ctx.sha.header, 0, value_string64, 8, NULL, 0);

    os_memset(address_temp, 0, sizeof(address_temp));
    os_memset(value_string64, 0, sizeof(value_string64));

    /////////////////////////////////////////////
    // data

    token_index = object_get_value(0, "d", &parsed_transaction, transaction_buffer);

    int key_size = parsed_transaction.Tokens[token_index].end - parsed_transaction.Tokens[token_index].start;

    unsigned char buffer_temp[key_size + 1];

    get_json_element(buffer_temp, sizeof(buffer_temp), token_index, transaction_buffer);

    const size_t numdigits = key_size / 2;

    uint8_t bufferArray[numdigits];
    memset(bufferArray, 0x00, numdigits);

    hexStringToByteArray((unsigned char*)buffer_temp, key_size, bufferArray);

    PRINTF("DATA HEX:        %.*h\n", numdigits, bufferArray);
    //0a0662696e617279

    cx_hash(&hash_ctx.sha.header, 0, bufferArray, numdigits, NULL, 0);

    os_memset(bufferArray, 0, sizeof(bufferArray));
    os_memset(buffer_temp, 0, sizeof(buffer_temp));

    /////////////////////////////////////////////
    // chainID

    token_index = object_get_value(ROOT_TOKEN_INDEX, "c", &parsed_transaction, transaction_buffer);
    get_json_element(address_temp, sizeof(address_temp), token_index, transaction_buffer);
    //PRINTF("CHAIN ID:%s\n", address_temp);

    uint32_t uint32 = char_to_int32(address_temp);

    unsigned char value_string32[4] = {0, 0, 0, 0};
    writeHexAmount32(uint32, (unsigned char *)value_string32, 0);
    PRINTF("chainID HEX:     %.*h\n", 4, value_string32);

    cx_hash(&hash_ctx.sha.header, 0, value_string32, 4, NULL, 0);

    os_memset(address_temp, 0, sizeof(address_temp));
    os_memset(value_string32, 0, sizeof(value_string32));

    /////////////////////////////////////////////
    // gasPrice

    token_index = object_get_value(ROOT_TOKEN_INDEX, "p", &parsed_transaction, transaction_buffer);
    get_json_element(address_temp, sizeof(address_temp), token_index, transaction_buffer);
    //PRINTF("GAS PRICE:%s\n", address_temp);

    uint64 = char_to_int64(address_temp);

    writeHexAmountBE(uint64, (unsigned char *)value_string128, 8);
    PRINTF("GAS PRICE HEX:   %.*h\n", 16, value_string128);

    cx_hash(&hash_ctx.sha.header, 0, value_string128, 16, NULL, 0);

    os_memset(address_temp, 0, sizeof(address_temp));
    os_memset(value_string128, 0, sizeof(value_string128));

    /////////////////////////////////////////////
    // gasLimit

    token_index = object_get_value(ROOT_TOKEN_INDEX, "l", &parsed_transaction, transaction_buffer);
    get_json_element(address_temp, sizeof(address_temp), token_index, transaction_buffer);
    //PRINTF("GAS LIMIT:%s\n", address_temp);

    uint64 = char_to_int64(address_temp);

    writeHexAmountBE(uint64, (unsigned char *)value_string128, 8);
    PRINTF("GAS LIMIT HEX:   %.*h\n", 16, value_string128);

    cx_hash(&hash_ctx.sha.header, CX_LAST, value_string128, 16, output, 32);

    os_memset(address_temp, 0, sizeof(address_temp));
    os_memset(value_string128, 0, sizeof(value_string128));
}