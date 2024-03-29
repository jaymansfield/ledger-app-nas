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

#pragma once

#include <jsmn.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Max number of accepted tokens in the JSON input
#define MAX_NUMBER_OF_TOKENS    19
#define ROOT_TOKEN_INDEX 0

//---------------------------------------------

// Context that keeps all the parsed data together. That includes:
//  - parsed json tokens
//  - re-created SendMsg struct with indices pointing to tokens in parsed json
typedef struct {
    uint16_t NumberOfTokens;
    jsmntok_t Tokens[MAX_NUMBER_OF_TOKENS];
} parsed_json_t;

/// Resets parsed_json data structure
/// \param
void reset_parsed_json(parsed_json_t *);

typedef struct {
    const parsed_json_t *parsed_tx;
    const char *tx;
} parsing_context_t;

//---------------------------------------------
// NEW JSON PARSER CODE

/// Parse json to create a token representation
/// \param parsed_json
/// \param transaction
/// \param transaction_length
/// \return Error message
const char *json_parse_s(
        parsed_json_t *parsed_json,
        const char *transaction,
        uint16_t transaction_length);

/// Parse json to create a token representation
/// \param parsed_json
/// \param transaction
/// \return Error message
const char *json_parse(
        parsed_json_t *parsed_json,
        const char *transaction);

/// Get the token index of the value that matches the given key
/// \param object_token_index: token index of the parent object
/// \param key_name: key name of the wanted value
/// \param parsed_transaction
/// \param transaction
/// \return returns token index or -1 if not found
int16_t object_get_value(uint16_t object_token_index,
                         const char *key_name,
                         const parsed_json_t *parsed_transaction,
                         const char *transaction);

#ifdef __cplusplus
}
#endif