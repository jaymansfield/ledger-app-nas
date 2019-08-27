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

#include "json_parser.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// This is the main function called from ledger that updates key and value strings
/// that are going to be displayed in the UI.
/// \param key, an array that will be filled with key string
/// \param max_key_length, size of the key array
/// \param value, an array that will be filled with value string
/// \param max_value_length, size of the value array
/// \param page_index, index of the UI page for which key and value will be returned
/// \param chunk_index, [optional] value is split into chunks if it's very long, here we specify which chunk we should use
/// \return number of chunks or -1 if it was not possible to find the items
int16_t transaction_get_display_key_value(char *key,
                                          int16_t max_key_length,
                                          char *value,
                                          int16_t max_value_length,
                                          int16_t page_index,
                                          int16_t chunk_index);

/// Validate json transaction
/// \param parsed_transacton
/// \param transaction
/// \return
const char *json_validate(parsed_json_t *parsed_transaction,
                          const char *transaction);

//---------------------------------------------
// Delegates

typedef void(*copy_delegate)(void *dst, const void *source, size_t size);
void set_copy_delegate(copy_delegate delegate);
void set_parsing_context(parsing_context_t context);

//---------------------------------------------

#ifdef __cplusplus
}
#endif