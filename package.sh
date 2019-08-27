#!/usr/bin/env bash

APPNAME=$1
APPVERSION=$2
ICONFILE=$3
TARGET_ID=0x31100004
ICONHEX=$(python ${BOLOS_SDK}/icon.py ${ICONNAME} hexbitmaponly)

python -m ledgerblue.loadApp --delete --targetId ${TARGET_ID} --apdu --fileName app.hex --appName ${APPNAME} --appVersion ${APPVERSION} --appFlags 0x00 --icon ${ICONHEX} --tlv --dataSize 0x00000C00