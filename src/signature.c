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
#include "signature.h"
#include "cx.h"

void keys_secp256k1(cx_ecfp_public_key_t *publicKey,
                    cx_ecfp_private_key_t *privateKey,
                    const uint8_t privateKeyData[32]) {
    cx_ecfp_init_private_key(CX_CURVE_256K1, privateKeyData, 32, privateKey);
    cx_ecfp_init_public_key(CX_CURVE_256K1, NULL, 0, publicKey);
    cx_ecfp_generate_pair(CX_CURVE_256K1, publicKey, privateKey, 1);
}

int sign_secp256k1(const uint8_t* hash, cx_ecfp_private_key_t *privateKey) {


    PRINTF("READY TO SIGN\n");

    //cx_ecfp_public_key_t publicKey;
    //cx_ecdsa_init_public_key(CX_CURVE_256K1, NULL, 0, &publicKey);
    //cx_ecfp_generate_pair(CX_CURVE_256K1, &publicKey, privateKey, 1);

    PRINTF("HASH:%.*h\n", 32, hash);

    unsigned int length = 0;
    unsigned int info = 0;

    length = cx_ecdsa_sign(
        privateKey,
        CX_RND_RFC6979 | CX_LAST,
        CX_SHA256,
        hash,
        32,
        G_io_apdu_buffer,
        IO_APDU_BUFFER_SIZE,
        &info);

    G_io_apdu_buffer[0] = 0;
    if (info & CX_ECCINFO_PARITY_ODD) {
      G_io_apdu_buffer[0]++;
    }
    if (info & CX_ECCINFO_xGTn) {
      G_io_apdu_buffer[0] += 2;
    }

    PRINTF("LENGTH:%d\n", length);
    PRINTF("signature:%.*H\n", length, G_io_apdu_buffer);

/*
    uint8_t rLength, sLength, rOffset, sOffset;

    uint8_t signature[75];
    os_memcpy(signature, G_io_apdu_buffer, length);

    rLength = G_io_apdu_buffer[3];
    sLength = G_io_apdu_buffer[4 + rLength + 1];
    rOffset = (rLength == 33 ? 1 : 0);
    sOffset = (sLength == 33 ? 1 : 0);
    os_memmove(signature, G_io_apdu_buffer + 4 + rOffset, 32);
    os_memmove(signature + 32, G_io_apdu_buffer + 4 + rLength + 2 + sOffset, 32);

    signature[64] = 0;
    if (info & CX_ECCINFO_PARITY_ODD) {
      signature[64]++;
    }
    if (info & CX_ECCINFO_xGTn) {
      signature[64] += 2;
    }

    //length = 65;

    //PRINTF("LENGTH:%d\n", length);
    PRINTF("signature:%.*H\n", 65, signature);
*/
    os_memset(&privateKey, 0, sizeof(privateKey));

    return length;
}