/*******************************************************************************
*   (c) 2018 ZondaX GmbH
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
#include "json_parser.h"

void reset_parsed_json(parsed_json_t *parser_data) {
    memset(parser_data, 0, sizeof(parsed_json_t));
}

const char *json_parse(
        parsed_json_t *parsed_json,
        const char *transaction) {
    return json_parse_s(parsed_json, transaction, strlen(transaction));

}

const char *json_parse_s(
        parsed_json_t *parsed_json,
        const char *transaction,
        uint16_t transaction_length) {

    jsmn_parser parser;
    jsmn_init(&parser);

    reset_parsed_json(parsed_json);

    int num_tokens = jsmn_parse(
            &parser,
            transaction,
            transaction_length,
            parsed_json->Tokens,
            MAX_NUMBER_OF_TOKENS);

    switch (num_tokens) {
        case JSMN_ERROR_NOMEM:
            return "NOMEM: JSON string contains too many tokens";
        case JSMN_ERROR_INVAL:
            return "Invalid character in JSON string";
        case JSMN_ERROR_PART:
            return "JSON string is not complete";
    }

    parsed_json->NumberOfTokens = 0;

    // Parsing error
    if (num_tokens <= 0) {
        return "Unknown parser error";
    }

    // We cannot support if number of tokens exceeds the limit
    if (num_tokens > MAX_NUMBER_OF_TOKENS) {
        return "TOK: JSON string contains too many tokens";
    }

    parsed_json->NumberOfTokens = num_tokens;
    return NULL;
}

int16_t object_get_value(uint16_t object_token_index,
                         const char *key_name,
                         const parsed_json_t *parsed_transaction,
                         const char *transaction) {
    if (object_token_index < 0 || object_token_index > parsed_transaction->NumberOfTokens) {
        return -1;
    }

    size_t length = strlen(key_name);
    jsmntok_t object_token = parsed_transaction->Tokens[object_token_index];
    int token_index = object_token_index;
    int prev_element_end = object_token.start;
    token_index++;
    while (true) {
        if (token_index >= parsed_transaction->NumberOfTokens) {
            break;
        }
        jsmntok_t key_token = parsed_transaction->Tokens[token_index++];
        jsmntok_t value_token = parsed_transaction->Tokens[token_index];
        if (key_token.start > object_token.end) {
            break;
        }
        if (key_token.start <= prev_element_end) {
            continue;
        }
        prev_element_end = value_token.end;
        char *cmper = (char *) (transaction + key_token.start);
        size_t cmper_l = (size_t) (key_token.end - key_token.start);
        if (memcmp(key_name, cmper, length) == 0 && cmper_l == length) {
            return token_index;
        }
    }

    return -1;
}