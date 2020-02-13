TARGET = RpiHap
TARGET_DIR = /usr/sbin
INIT_DIR = /etc/init.d
INIT_SCRIPT = homekit

DEBUG ?= 0

# enable GCC9
# refer to https://solarianprogrammer.com/2017/12/08/raspberry-pi-raspbian-install-gcc-compile-cpp-17-programs/
GCC9 ?= 0

SRCS_C := \
    $(shell find ../Hap/*.c) \


SRCS_CPP := \
    $(shell find ../Hap/*.cpp) \
    $(shell find ../Crypto/*.cpp) \
    $(shell find ../CryptoTest/*.cpp) \
    $(shell find ../Util/*.cpp) \
    $(shell find ../Linux/*.cpp) \
    RpiHap.cpp

ifeq ($(DEBUG),1)
CFLAGS = -g -O0
else
CFLAGS = -O2
endif

CC = gcc
CPP = g++
CFLAGS += -I. -I.. -Wall
CPPFLAGS = $(CFLAGS) -std=c++17
LDFLAGS = -lm -lstdc++ -lpthread -ldns_sd -lwiringPi

ifeq ($(GCC9),1)
CC = /opt/gcc-9.1.0/bin/gcc-9.1
CPP = /opt/gcc-9.1.0/bin/g++-9.1
LDFLAGS = -L/opt/gcc-9.1.0/lib -lm -lstdc++ -lpthread -ldns_sd -lwiringPi
endif


all: $(TARGET)

depend: .depend

.depend: cmd = $(CC) $(CFLAGS) -MM -MF depend $(var); cat depend >> .depend;
.depend:
	@echo "Generating dependencies..."
	@$(foreach var, $(SRCS_C) $(SRCS_CPP), $(cmd))
	@rm -f depend

-include .depend

OBJS=
define make_c =
 $(2): $(1)
	$$(CC) $$(CFLAGS) -c $$< -o $$@
 OBJS += $(2)
endef
define make_cpp =
 $(2): $(1)
	$$(CC) $$(CPPFLAGS) -c $$< -o $$@
 OBJS += $(2)
endef

$(foreach src,$(SRCS_C),$(eval $(call make_c,$(src),$(basename $(notdir $(src))).o)))

$(foreach src,$(SRCS_CPP),$(eval $(call make_cpp,$(src),$(basename $(notdir $(src))).o)))

$(TARGET): .depend $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(TARGET)

install: $(TARGET) $(INIT_SCRIPT)
	install $(TARGET) $(TARGET_DIR)
	install $(INIT_SCRIPT) $(INIT_DIR)

clean:
	rm -f .depend *.o $(TARGET)

.PHONY: clean depend

