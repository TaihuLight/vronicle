#
# Copyright (C) 2011-2017 Intel Corporation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#   * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in
#     the documentation and/or other materials provided with the
#     distribution.
#   * Neither the name of Intel Corporation nor the names of its
#     contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#



######## SGX SDK Settings ########
SGX_MODE ?= HW
SGX_ARCH ?= x64
UNTRUSTED_DIR=app
USE_OPENSSL = 1
ENABLE_DCAP ?= 0

ifeq ($(shell getconf LONG_BIT), 32)
	SGX_ARCH := x86
else ifeq ($(findstring -m32, $(CXXFLAGS)), -m32)
	SGX_ARCH := x86
endif

ifeq ($(SGX_ARCH), x86)
	$(error x86 build is not supported, only x64!!)
else
	SGX_COMMON_CFLAGS := -m64 -Wall
	ifeq ($(LINUX_SGX_BUILD), 1)
		include ../../../../../buildenv.mk
		SGX_LIBRARY_PATH := $(BUILD_DIR)
		SGX_EDGER8R := $(BUILD_DIR)/sgx_edger8r
		SGX_SDK_INC := $(COMMON_DIR)/inc
		SGX_SHARED_LIB_FLAG := -Wl,-rpath,${SGX_LIBRARY_PATH}
	else
		SGX_LIBRARY_PATH := $(SGX_SDK)/lib64
		SGX_EDGER8R := $(SGX_SDK)/bin/x64/sgx_edger8r
		SGX_SDK_INC := $(SGX_SDK)/include
	endif
endif

ifeq ($(DEBUG), 1)
ifeq ($(SGX_PRERELEASE), 1)
$(error Cannot set DEBUG and SGX_PRERELEASE at the same time!!)
endif
endif

OPENSSL_LIBRARY_PATH := $(PACKAGE_LIB)
ifeq ($(DEBUG), 1)
        SGX_COMMON_CFLAGS += -O0 -g
		SgxSSL_Link_Libraries := sgx_usgxssld
else
        SGX_COMMON_CFLAGS += -O2 -D_FORTIFY_SOURCE=2
		SgxSSL_Link_Libraries := sgx_usgxssl
endif

ifeq ($(ENABLE_DCAP), 1)
        SGX_COMMON_CFLAGS += -DENABLE_DCAP
endif

######## App Settings ########


App_Cpp_Files := $(UNTRUSTED_DIR)/TestApp.cpp $(UNTRUSTED_DIR)/tcp_module/TCPServer.cpp $(UNTRUSTED_DIR)/tcp_module/TCPClient.cpp
App_Cpp_Objects := $(App_Cpp_Files:.cpp=.o)

App_C_Files := $(UNTRUSTED_DIR)/sgxsdk-ra-attester_u.c $(UNTRUSTED_DIR)/ias-ra.c $(UNTRUSTED_DIR)/ra-challenger_u.c
App_C_Objects := $(App_C_Files:.c=.o)

App_Include_Paths := -I$(UNTRUSTED_DIR) -I$(SGX_SDK_INC) -Icommon

App_C_Flags := $(SGX_COMMON_CFLAGS) -fpic -fpie -fstack-protector -Wformat -Wformat-security -Wno-attributes $(App_Include_Paths) -lcurl
App_Cpp_Flags := $(App_C_Flags) -std=c++11 -lcrypto -I/usr/include/openssl -lssl -L/usr/lib/x86_64-linux-gnu/

Security_Link_Flags := -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now -pie

ifneq ($(SGX_MODE), HW)
	Urts_Library_Name := sgx_urts_sim
	Epid_Library_Name := sgx_epid_sim
	Quote_Library_Name := sgx_quote_ex_sim
else
	Urts_Library_Name := sgx_urts
	Epid_Library_Name := sgx_epid
	Quote_Library_Name := sgx_quote_ex
endif
App_Link_Flags := -lcrypto -I/usr/include/openssl -lssl -L/usr/lib/x86_64-linux-gnu/ $(SGX_COMMON_CFLAGS) $(Security_Link_Flags) $(SGX_SHARED_LIB_FLAG) -L$(SGX_LIBRARY_PATH) -l$(Urts_Library_Name) -l$(Epid_Library_Name) -l$(Quote_Library_Name) -L$(OPENSSL_LIBRARY_PATH) -l$(SgxSSL_Link_Libraries) -lpthread -lcurl

ifeq ($(ENABLE_DCAP), 0)
	Epid_Library_Name := sgx_epid
	ifneq ($(SGX_MODE), HW)
		Epid_Library_Name += _sim
	endif
	App_Link_Flags += -l$(Epid_Library_Name)
else
	Dcap_Library_Name := sgx_dcap_ql
	ifneq ($(SGX_MODE), HW)
		Dcap_Library_Name += _sim
	endif
	App_Link_Flags += -l$(Dcap_Library_Name)
	App_Link_Flags += -lsgx_dcap_quoteverify
endif

# ifneq ($(SGX_MODE), HW)
# 	Urts_Library_Name := sgx_urts_sim
# 	UaeService_Library_Name := sgx_uae_service_sim
# else
# 	Urts_Library_Name := sgx_urts
# 	UaeService_Library_Name := sgx_uae_service
# endif
# App_Link_Flags := -lcrypto -I/usr/include/openssl -lssl -L/usr/lib/x86_64-linux-gnu/ $(SGX_COMMON_CFLAGS) $(Security_Link_Flags) $(SGX_SHARED_LIB_FLAG) -L$(SGX_LIBRARY_PATH) -l$(Urts_Library_Name) -l$(UaeService_Library_Name) -L$(OPENSSL_LIBRARY_PATH) -l$(SgxSSL_Link_Libraries) -lpthread -lcurl


.PHONY: all test

all: TestApp

test: all
	@$(CURDIR)/TestApp
	@echo "RUN  =>  TestApp [$(SGX_MODE)|$(SGX_ARCH), OK]"

######## App Objects ########

ifeq ($(ENABLE_DCAP), 0)
$(UNTRUSTED_DIR)/TestEnclave_u.c: $(SGX_EDGER8R) enclave/TestEnclave.edl $(TCP_Server_Library_Compiled_Name)
	@cd $(UNTRUSTED_DIR) && $(SGX_EDGER8R) --untrusted ../enclave/TestEnclave.edl --search-path $(PACKAGE_INC) --search-path $(SGX_SDK_INC)
else
$(UNTRUSTED_DIR)/TestEnclave_dcap_u.c: $(SGX_EDGER8R) enclave/TestEnclave_dcap.edl $(TCP_Server_Library_Compiled_Name)
	@cd $(UNTRUSTED_DIR) && $(SGX_EDGER8R) --untrusted ../enclave/TestEnclave_dcap.edl --search-path $(PACKAGE_INC) --search-path $(SGX_SDK_INC)
endif
	@echo "GEN  =>  $@"

ifeq ($(ENABLE_DCAP), 0)
$(UNTRUSTED_DIR)/TestEnclave_u.o: $(UNTRUSTED_DIR)/TestEnclave_u.c
else
$(UNTRUSTED_DIR)/TestEnclave_dcap_u.o: $(UNTRUSTED_DIR)/TestEnclave_dcap_u.c
endif
	$(VCC) $(App_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

$(UNTRUSTED_DIR)/%.o: $(UNTRUSTED_DIR)/%.cpp
	$(VCXX) $(App_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

$(UNTRUSTED_DIR)/%.o: $(UNTRUSTED_DIR)/%.c
	$(VCC) $(App_C_Flags) -c $< -o $@
	@echo "CC  <=  $<"

ifeq ($(ENABLE_DCAP), 0)
TestApp: $(UNTRUSTED_DIR)/TestEnclave_u.o $(App_Cpp_Objects) $(App_C_Objects)
else
TestApp: $(UNTRUSTED_DIR)/TestEnclave_dcap_u.o $(App_Cpp_Objects) $(App_C_Objects)
endif
	$(VCXX) $^ -o $@ $(App_Link_Flags)
	@echo "LINK =>  $@"


.PHONY: clean

clean:
ifeq ($(ENABLE_DCAP), 0)
	@rm -f TestApp $(App_Cpp_Objects) $(App_C_Objects) $(UNTRUSTED_DIR)/TestEnclave_u.*
else
	@rm -f TestApp $(App_Cpp_Objects) $(App_C_Objects) $(UNTRUSTED_DIR)/TestEnclave_dcap_u.*
endif
	
