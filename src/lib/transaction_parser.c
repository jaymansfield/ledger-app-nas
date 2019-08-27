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

#include <jsmn.h>
#include <stdio.h>
#include "transaction_parser.h"
#include "json_parser.h"
#include "fixed8.h"

#define MAX_RECURSION_DEPTH  3
#define MAX_TREE_LEVEL       2
#define NON_MSG_PAGES_COUNT  7

//---------------------------------------------

const char whitespaces[] = {
    0x20, // space ' '
    0x0c, // form_feed '\f'
    0x0a, // line_feed, '\n'
    0x0d, // carriage_return, '\r'
    0x09, // horizontal_tab, '\t'
    0x0b  // vertical_tab, '\v'
};

//---------------------------------------------

int16_t msgs_total_pages = 0;
int16_t msgs_array_elements = 0;

//---------------------------------------------

copy_delegate copy_fct = NULL;          // Decoupled so we can handle both RAM and NVRAM with similar code
parsing_context_t parsing_context;

void set_copy_delegate(copy_delegate delegate) {
    copy_fct = delegate;
}

void set_parsing_context(parsing_context_t context) {
    parsing_context = context;
}

//--------------------------------------
// Transaction parsing helper functions
//--------------------------------------
int16_t update(char *out, const int16_t out_len, const int16_t token_index, uint16_t chunk_to_display) {

    const int16_t token_start = parsing_context.parsed_tx->Tokens[token_index].start;
    const int16_t token_end = parsing_context.parsed_tx->Tokens[token_index].end;
    const int16_t token_len = token_end - token_start;

    int16_t num_chunks = (token_len / (out_len - 1)) + 1;
    if (token_len > 0 && (token_len % (out_len - 1) == 0))
        num_chunks--;

    out[0] = '\0';  // flush

    if (chunk_to_display < num_chunks) {
        const int16_t chunk_start = token_start + chunk_to_display * (out_len - 1);
        int16_t chunk_len = token_end - chunk_start;

        if (chunk_len < 0) {
            return -1;
        }

        if (chunk_len > out_len - 1) {
            chunk_len = out_len - 1;
        }
        copy_fct(out, parsing_context.tx + chunk_start, chunk_len);
        out[chunk_len] = 0;
    }

    return num_chunks;
}

int16_t transaction_get_display_key_value(char *key, int16_t max_key_length,
                                          char *value, int16_t max_value_length,
                                          int16_t page_index, int16_t chunk_index) {
    const int16_t non_msg_pages_count = NON_MSG_PAGES_COUNT;
    if (page_index >= 0 && page_index < non_msg_pages_count) {


        const char *key_name;
        const char *key_subt;
        switch (page_index) {
            case 0:
                key_name = "f";
                key_subt = "From Address";
                break;
            case 1:
                key_name = "a";
                key_subt = "To Address";
                break;
            case 2:
                key_name = "v";
                key_subt = "Value";
                break;
            case 3:
                key_name = "n";
                key_subt = "Nonce";
                break;
            case 4:
                key_name = "p";
                key_subt = "Gas Price";
                break;
            case 5:
                key_name = "l";
                key_subt = "Gas Limit";
                break;
            case 6:
                key_name = "c";
                key_subt = "Chain ID";
                break;
            default:
                key_name = "???";
                key_subt = "???";
        }
        strcpy(key, key_subt);

        int16_t token_index = object_get_value(ROOT_TOKEN_INDEX,
                                               key_name,
                                               parsing_context.parsed_tx,
                                               parsing_context.tx);

        uint16_t ret = update(value, max_value_length, token_index, chunk_index);

        // reformat value   (1000000000000000000 = 1)
        if(page_index == 2)
        {
            fixed8_str_conv(value, value, '\0');
        }

        // empty values only really ever show up here; msgs are filled
        if (strlen(value) == 0) {
            strcpy(value, "(none)");
        }
        return ret;
    }
    return -1;
}

int8_t is_space(char c) {
    for (unsigned int i = 0; i < sizeof(whitespaces); i++) {
        if (whitespaces[i] == c) {
            return 1;
        }
    }
    return 0;
}

int8_t contains_whitespace(parsed_json_t *parsed_transaction,
                           const char *transaction) {

    int start = 0;
    int last_element_index = parsed_transaction->Tokens[0].end;

    // Starting at token 1 because token 0 contains full tx
    for (int i = 1; i < parsed_transaction->NumberOfTokens; i++) {
        if (parsed_transaction->Tokens[i].type != JSMN_UNDEFINED) {
            int end = parsed_transaction->Tokens[i].start;
            for (int j = start; j < end; j++) {
                if (is_space(transaction[j]) == 1) {
                    return 1;
                }
            }
            start = parsed_transaction->Tokens[i].end + 1;
        } else {
            return 0;
        }
    }
    while (start <= last_element_index && transaction[start] != '\0') {
        if (is_space(transaction[start]) == 1) {
            return 1;
        }
        start++;
    }
    return 0;
}

const char* json_validate(parsed_json_t* parsed_transaction, const char *transaction) {

    if (contains_whitespace(parsed_transaction, transaction) == 1) {
        return "Contains whitespace in the corpus";
    }

    /////////////////////////////////////////////

    int token_index = object_get_value(0, "c", parsed_transaction, transaction);
    if (token_index == -1) {
        return "Missing chainID element";
    }

    int length = parsed_transaction->Tokens[token_index].end -
            parsed_transaction->Tokens[token_index].start;

    if(length > 6)
    {
        return "ChainID should be less then 6 characters";
    }

    /////////////////////////////////////////////

    token_index = object_get_value(0, "a", parsed_transaction, transaction);
    if (token_index == -1) {
        return "Missing to element";
    }

    length = parsed_transaction->Tokens[token_index].end -
            parsed_transaction->Tokens[token_index].start;

    if(length != 35)
    {
        return "To address should be 35 characters";
    }

    /////////////////////////////////////////////

    token_index = object_get_value(0, "f", parsed_transaction, transaction);
    if (token_index == -1) {
        return "Missing from element";
    }

    length = parsed_transaction->Tokens[token_index].end -
            parsed_transaction->Tokens[token_index].start;

    if(length != 35)
    {
        return "From address should be 35 characters";
    }

    /////////////////////////////////////////////

    token_index = object_get_value(0, "t", parsed_transaction, transaction);
    if (token_index == -1) {
        return "Missing timestamp element";
    }

    length = parsed_transaction->Tokens[token_index].end -
            parsed_transaction->Tokens[token_index].start;

    if(length > 10)
    {
        return "Timestamp should be less then 10 characters";
    }

    /////////////////////////////////////////////

    token_index = object_get_value(0, "v", parsed_transaction, transaction);
    if (token_index == -1) {
        return "Missing value element";
    }

    length = parsed_transaction->Tokens[token_index].end -
            parsed_transaction->Tokens[token_index].start;

    if(length > 32)
    {
        return "Value should be less then 32 characters";
    }

    /////////////////////////////////////////////

    token_index = object_get_value(0, "n", parsed_transaction, transaction);
    if (token_index == -1) {
        return "Missing nonce element";
    }

    length = parsed_transaction->Tokens[token_index].end -
            parsed_transaction->Tokens[token_index].start;

    if(length > 12)
    {
        return "Nonce should be less then 12 characters";
    }

    /////////////////////////////////////////////

    token_index = object_get_value(0, "p", parsed_transaction, transaction);
    if (token_index == -1) {
        return "Missing gasPrice element";
    }

    length = parsed_transaction->Tokens[token_index].end -
            parsed_transaction->Tokens[token_index].start;

    if(length > 16)
    {
        return "GasPrice should be less then 16 characters";
    }

    /////////////////////////////////////////////

    token_index = object_get_value(0, "l", parsed_transaction, transaction);
    if (token_index == -1) {
        return "Missing gasLimit element";
    }

    length = parsed_transaction->Tokens[token_index].end -
            parsed_transaction->Tokens[token_index].start;

    if(length > 16)
    {
        return "GasLimit should be less then 16 characters";
    }

    /////////////////////////////////////////////

    token_index = object_get_value(0, "d", parsed_transaction, transaction);
    if (token_index == -1) {
        return "Missing dataBuffer element";
    }

    length = parsed_transaction->Tokens[token_index].end -
                parsed_transaction->Tokens[token_index].start;

    if(length > 512)
    {
        return "DataBuffer should be less then 512 characters";
    }

    return NULL;
}