LINUX_PATH ?= ../linux
CROSS_COMPILE_PATH ?= ../toolchains/aarch64/bin

SRC 		= src
FLAGS 		= CROSS_COMPILE=$(CROSS_COMPILE_PATH)/aarch64-linux-gnu- CFG_ARM64_core=y PLATFORM=hikey-hikey960 ARCH=arm64
MODULE_NAME = Load_Step
OBJ_LIST	= pmu_ctrl debugfs_ctrl side_channel_attacks memory kern_hijack cache_helpers_a64
obj-m += $(MODULE_NAME).o
$(MODULE_NAME)-objs := $(SRC)/$(MODULE_NAME).o
$(MODULE_NAME)-objs += $(foreach var, $(OBJ_LIST), $(SRC)/$(var).o)

all: clean
	make -C $(LINUX_PATH) M=$(shell pwd) modules $(FLAGS)

clean:
	make -C $(LINUX_PATH) M=$(shell pwd) clean