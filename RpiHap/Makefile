TARGET = RpiHap
TARGET_DIR = /usr/sbin
INIT_DIR = /etc/init.d
INIT_SCRIPT = homekit

DEBUG ?= 0

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
LDFLAGS = 
endif

CC = gcc
CPP = gcc
CFLAGS += -I. -I.. -Wall
CPPFLAGS = $(CFLAGS)
LDFLAGS += -lm -lstdc++ -lpthread -ldns_sd -lwiringPi

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

