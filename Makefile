#*******************************************************************************
#   Ledger Blue
#   (c) 2016 Ledger
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#*******************************************************************************

ifeq ($(BOLOS_SDK),)
$(error BOLOS_SDK is not set)
endif
include $(BOLOS_SDK)/Makefile.defines

dummy_submodules := $(shell git submodule update --init --recursive)

# Main app configuration

APPNAME = "Nebulas"
APPVERSION_M=1
APPVERSION_N=0
APPVERSION_P=0
APPVERSION = $(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)
ICONNAME=$(CURDIR)/icon_nebulas.gif
APP_LOAD_PARAMS = --delete --appFlags 0x00 --path "44'/2718'" $(COMMON_LOAD_PARAMS)

# Build configuration

SCRIPT_LD:=$(CURDIR)/script.ld

APP_SOURCE_PATH += src
SDK_SOURCE_PATH += lib_stusb lib_stusb_impl

DEFINES += APPVERSION=\"$(APPVERSION)\"

DEFINES += OS_IO_SEPROXYHAL IO_SEPROXYHAL_BUFFER_SIZE_B=128
DEFINES += HAVE_BAGL HAVE_SPRINTF
#DEFINES += PRINTF\(...\)=
DEFINES += HAVE_SPRINTF HAVE_PRINTF PRINTF=screen_printf

DEFINES += UNUSED\(x\)=\(void\)x

DEFINES += HAVE_IO_USB HAVE_L4_USBLIB IO_USB_MAX_ENDPOINTS=7 IO_HID_EP_LENGTH=64 HAVE_USB_APDU
DEFINES += LEDGER_MAJOR_VERSION=$(APPVERSION_M) LEDGER_MINOR_VERSION=$(APPVERSION_N) LEDGER_PATCH_VERSION=$(APPVERSION_P)

#SDK_SOURCE_PATH += lib_u2f
#DEFINES   += HAVE_U2F HAVE_IO_U2F
#DEFINES   += U2F_PROXY_MAGIC=\"NAS\"
#DEFINES   += USB_SEGMENT_SIZE=64
#DEFINES   += U2F_MAX_MESSAGE_SIZE=264

WEBUSB_URL     = www.ledgerwallet.com
DEFINES       += HAVE_WEBUSB WEBUSB_URL_SIZE_B=$(shell echo -n $(WEBUSB_URL) | wc -c) WEBUSB_URL=$(shell echo -n $(WEBUSB_URL) | sed -e "s/./\\\'\0\\\',/g")

#DEFINES += CX_COMPLIANCE_141

# Compiler, assembler, and linker

ifneq ($(BOLOS_ENV),)
$(info BOLOS_ENV=$(BOLOS_ENV))
#CLANGPATH := $(BOLOS_ENV)/clang-arm-fropi/bin/
CLANGPATH := /usr/bin/
GCCPATH := $(BOLOS_ENV)/gcc-arm-none-eabi-5_3-2016q1/bin/
else
$(info BOLOS_ENV is not set: falling back to CLANGPATH and GCCPATH)
endif
ifeq ($(CLANGPATH),)
$(info CLANGPATH is not set: clang will be used from PATH)
endif
ifeq ($(GCCPATH),)
$(info GCCPATH is not set: arm-none-eabi-* will be used from PATH)
endif

#CC := $(CLANGPATH)clang
CC := $(CLANGPATH)clang-4.0
CFLAGS += -O3 -Os

AS := $(GCCPATH)arm-none-eabi-gcc
AFLAGS +=

LD := $(GCCPATH)arm-none-eabi-gcc
LDFLAGS += -O3 -Os
LDLIBS += -lm -lgcc -lc

APP_SOURCE_PATH += src deps/jsmn/src deps/ledger-zxlib/include deps/ledger-zxlib/src
SDK_SOURCE_PATH += lib_stusb lib_u2f lib_stusb_impl

# Main rules

all: default

load: all
	python -m ledgerblue.loadApp $(APP_LOAD_PARAMS)

delete:
	python -m ledgerblue.deleteApp $(COMMON_DELETE_PARAMS)

package:
	./package.sh ${APPNAME} ${APPVERSION} ${ICONNAME}

# Import generic rules from the SDK

include $(BOLOS_SDK)/Makefile.rules
