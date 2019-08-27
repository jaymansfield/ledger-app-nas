/*******************************************************************************
*   (c) 2019 Binance
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

#include "fixed8.h"
#include <string.h>

#define DECIMAL_SCALE 18
#define ZERO_FRACTION "000000000000000000"

int fixed8_str_conv(char *output, char *input, char terminator) {
    size_t input_len = strlen(input);
    if (strrchr(output, '.')) return 0; // already converted
    char tmp[DECIMAL_SCALE + 1];
    tmp[DECIMAL_SCALE] = '\0'; // just in case
    int newLen = 0;
    if (input_len <= DECIMAL_SCALE) { // satoshi amount
        strcpy(tmp, input);
        output[0] = '0';
        output[1] = '.';
        strcpy(&output[2], ZERO_FRACTION);
        int add_decs = DECIMAL_SCALE - strlen(tmp);
        strcpy(&output[2 + add_decs], tmp);
        output[input_len + 2 + add_decs] = terminator;
        newLen = input_len + 2 + add_decs;
    }
    else
    {
        int input_dec_offset = input_len - DECIMAL_SCALE;
        strcpy(tmp, &input[input_dec_offset]);
        output[input_dec_offset] = '.';
        strncpy(output, input, input_len - DECIMAL_SCALE);
        strcpy(&output[input_dec_offset + 1], tmp);
        output[input_len + 1] = terminator;
        newLen = input_len + 1;
    }

    // remove trailing 0's
    for(int i = newLen - 1; i >= 1; i--)
    {
        if(output[i] == '.')
        {
            output[i] = '\0';
            break;
        }

        if(output[i] == '1' || output[i] == '2' || output[i] == '3' || output[i] == '4' || output[i] == '5' || output[i] == '6' || output[i] == '7' || output[i] == '8' || output[i] == '9')
            break;

        if(output[i] == '0')
            output[i] = '\0';
    }
    return 1;
}