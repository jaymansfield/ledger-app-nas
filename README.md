# Ledger Nano S Nebulas App
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

# Dependencies

This project requires Ledger firmware 1.5.5

# Building

To build the app, follow ALL of the instructions in the link below to get your BOLOS development environment set up.

https://ledger.readthedocs.io/en/latest/userspace/getting_started.html

Then, use the Makefile to build and load the app:

```bash
$ make
$ make load
```

#### Loading a Precompiled HEX File
```bash
python -m ledgerblue.loadApp \
--delete \
--targetId 0x31100004 \
--fileName HEX_FILENAME.hex \
--icon "0100000000ffffff00ffffffff7fff7ffebffebffd1fc16fec37f683f8bffd7ffd7ffefffeffffffff" \
--curve secp256k1 \
--path "44'/2718'" \
--apdu \
--appName "Nebulas" \
--appVersion "1.0.0" \
--appFlags 0x00 \
--dataSize 0x00000C00 \
--tlv 
```

# General Structure

#### Nebulas BIP32 Path

44'/2718'/0'

#### Commands

The general structure of commands and responses is as follows:


| Field   | Type     | Content                | Note |
|:------- |:-------- |:---------------------- | ---- |
| CLA     | byte (1) | Application Identifier | 0x6e |
| INS     | byte (1) | Instruction ID         |      |
| P1      | byte (1) | Parameter 1            |      |
| P2      | byte (1) | Parameter 2            |      |
| L       | byte (1) | Bytes in payload       |      |
| PAYLOAD | byte (L) | Payload                |      |

#### Response

| Field   | Type     | Content     | Note                     |
| ------- | -------- | ----------- | ------------------------ |
| ANSWER  | byte (?) | Answer      | depends on the command   |
| SW1-SW2 | byte (2) | Return code | see list of return codes |

#### Return codes

| Return code | Description                 |
| ----------- | ----------------------------|
| 0x9000      | Success                     |
| 0x9001      | Device is busy              |
| 0x6400      | Execution Error             |
| 0x6700      | Wrong Length                |
| 0x6804      | Ledger device is locked     |
| 0x6982      | Empty Buffer                |
| 0x6983      | Output buffer too small     |
| 0x6984      | Data is invalid             |
| 0x6985      | Conditions not satisfied    |
| 0x6986      | Transaction Rejected        |
| 0x6A80      | Data element is too long    |
| 0x6B00      | Invalid P1/P2               |
| 0x6D00      | Instruction not Supported   |
| 0x6E00      | Nebulas Ledger App not open |
| 0x6F00      | Unknown Error               |
| 0x6F01      | Sign/verify Error           |
