############################################################
# <bsn.cl fy=2015 v=onl>
#
#           Copyright 2015 Big Switch Networks, Inc.
#
# Licensed under the Eclipse Public License, Version 1.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#        http://www.eclipse.org/legal/epl-v10.html
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
# either express or implied. See the License for the specific
# language governing permissions and limitations under the
# License.
#
# </bsn.cl>
############################################################
#
# Prepare and build a kernel.
#
############################################################

ifndef ARCH
$(error $$ARCH must be set)
endif

#
# The kernel major version
#
ifndef K_MAJOR_VERSION
$(error $$K_MAJOR_VERSION must be set)
endif

#
# The kernel patchlevel.
#
ifndef K_PATCH_LEVEL
$(error $$K_PATCH_LEVEL must be set)
endif

#
# The kernel sublevel
#
ifndef K_SUB_LEVEL
$(error $$K_SUB_LEVEL must be set)
endif

#
# The directory containing the patches to be applied
# to the kernel sources.
#
ifndef K_PATCH_DIR
$(error $$K_PATCH_DIR must be set)
endif

ifndef K_PATCH_SERIES_FILE
    ifndef K_PATCH_SERIES
        K_PATCH_SERIES = series
    endif
    K_PATCH_SERIES_FILE = $(K_PATCH_DIR)/$(K_PATCH_SERIES)
endif

#
# This is the directory that will receive the build targets.
# The kernel build tree is placed in this directory,
# as well as any custom copy targets.
#
ifndef K_TARGET_DIR
$(error $$K_TARGET_DIR not set)
endif

#
# This is the absolute path to the kernel configuration
# that should be used for this build.
#
ifndef K_CONFIG
$(error $$K_CONFIG not set)
endif

#
# This is the build target. bzImage, uImage, etc.
#
ifndef K_BUILD_TARGET
$(error $$K_BUILD_TARGET not set)
endif

############################################################
############################################################
ONL_KERNELS := $(ONL)/packages/base/any/kernels

K_VERSION := $(K_MAJOR_VERSION).$(K_PATCH_LEVEL).$(K_SUB_LEVEL)$(K_SUFFIX)
ifndef K_NAME
K_NAME := linux-$(K_VERSION)
endif
ifndef K_ARCHIVE_EXT
K_ARCHIVE_EXT := tar.xz
endif
ifndef K_ARCHIVE_NAME
K_ARCHIVE_NAME := $(K_NAME).$(K_ARCHIVE_EXT)
endif
K_ARCHIVE_PATH := $(ONL_KERNELS)/archives/$(K_ARCHIVE_NAME)
ifndef K_ARCHIVE_URL
K_ARCHIVE_URL := https://www.kernel.org/pub/linux/kernel/v$(K_MAJOR_VERSION).x/$(K_ARCHIVE_NAME)
endif

ifndef K_SOURCE_DIR
K_SOURCE_DIR := $(K_TARGET_DIR)/$(K_NAME)
endif

ifndef K_MBUILD_DIR
K_MBUILD_DIR := $(K_SOURCE_DIR)-mbuild
endif


K_INSTALL_MOD_PATH := $(K_TARGET_DIR)

ifndef K_DTBS_DIR
K_DTBS_DIR := $(K_SOURCE_DIR)-dtbs
endif

ifndef K_SKIP_DOWNLOAD
K_SKIP_DOWNLOAD := 0
endif

ifndef K_SKIP_EXTRACT
K_SKIP_EXTRACT := 0
endif
#
# The kernel source archive. Download if not present.
#

# The certificates for kernel.org are expired in the jessie container.
# Until that can be fixed we skip cert checks here:
EXTRA_WGET_OPTIONS := --no-check-certificate

$(K_ARCHIVE_PATH):
	if [ $(K_SKIP_DOWNLOAD) -eq 0 ]; then cd $(ONL_KERNELS)/archives && wget $(EXTRA_WGET_OPTIONS) $(K_ARCHIVE_URL); fi
	touch $@

.PHONY : ksource kpatched

#
# The extracted kernel sources
#
$(K_SOURCE_DIR)/Makefile: $(K_ARCHIVE_PATH)
	if [ $(K_SKIP_EXTRACT) -eq 0 ]; then mkdir -p $(K_TARGET_DIR) && cd $(K_TARGET_DIR) && tar kxf $(K_ARCHIVE_PATH); fi
	touch -c $(K_SOURCE_DIR)/Makefile
	$(K_MAKE) mrproper

ksource: $(K_SOURCE_DIR)/Makefile

#
# The patched kernel sources
#
$(K_SOURCE_DIR)/.PATCHED: $(K_SOURCE_DIR)/Makefile
	$(ONL)/tools/scripts/apply-patches.sh $(K_SOURCE_DIR) $(K_PATCH_DIR) $(K_PATCH_SERIES_FILE)
	touch $(K_SOURCE_DIR)/.PATCHED

kpatched: $(K_SOURCE_DIR)/.PATCHED

#
# Setup the kernel and output directory for the build.
#
setup: $(K_SOURCE_DIR)/.PATCHED
	cp $(K_CONFIG) $(K_SOURCE_DIR)/.config

#
# Kernel build command.
#
K_MAKE    := $(MAKE) -C $(K_SOURCE_DIR)

#
# Build the kernel.
#
build: setup
	+$(K_MAKE) $(K_BUILD_TARGET)
	+$(K_MAKE) modules
	+$(K_MAKE) modules_install INSTALL_MOD_PATH=$(K_INSTALL_MOD_PATH)
	find $(K_INSTALL_MOD_PATH) -type l -name source -delete
	find $(K_INSTALL_MOD_PATH) -type l -name build -delete


ifdef K_COPY_SRC
ifdef K_COPY_DST
ifdef K_COPY_GZIP
	gzip -c $(K_SOURCE_DIR)/$(K_COPY_SRC) > $(K_TARGET_DIR)/$(K_COPY_DST)
else
	cp $(K_SOURCE_DIR)/$(K_COPY_SRC) $(K_TARGET_DIR)/$(K_COPY_DST)
endif
endif
endif


MODSYNCLIST_DEFAULT := .config System.map Module.symvers Makefile include scripts drivers \
			arch/x86/include arch/x86/Makefile \
			arch/powerpc/include arch/powerpc/Makefile arch/powerpc/lib arch/powerpc/boot/dts \
			arch/arm/include arch/arm/Makefile arch/arm/lib arch/arm/boot/dts arch/arm/kernel

MODSYNCLIST := $(MODSYNCLIST_DEFAULT) $(MODSYNCLIST_EXTRA) $(K_MODSYNCLIST)

MODSYNCKEEP := $(K_MODSYNCKEEP)

ifeq ($(ARCH), powerpc)
# This file must be preserved for PPC module builds.
MODSYNCKEEP += arch/powerpc/lib/crtsavres.o
endif

mbuild: build
	rm -rf $(K_MBUILD_DIR)
	mkdir -p $(K_MBUILD_DIR)
	$(foreach f,$(MODSYNCLIST),$(ONL)/tools/scripts/tree-copy.sh $(K_SOURCE_DIR) $(f) $(K_MBUILD_DIR);)
	find $(K_MBUILD_DIR) -name "*.o*" -delete
	find $(K_MBUILD_DIR) -name "*.c" -delete
	find $(K_MBUILD_DIR) -name "*.ko" -delete
	$(foreach f,$(MODSYNCKEEP),$(ONL)/tools/scripts/tree-copy.sh $(K_SOURCE_DIR) $(f) $(K_MBUILD_DIR);)

dtbs: mbuild
ifdef DTS_LIST
	rm -rf $(K_DTBS_DIR)
	mkdir -p $(K_DTBS_DIR)
ifeq ($(ARCH) $(DTS_LIST),arm64 fsl)
	cp $(K_SOURCE_DIR)/arch/$(ARCH)/boot/dts/freescale/*.dtb $(K_DTBS_DIR)
else
	$(foreach name,$(DTS_LIST),mkdir -p $(dir $(K_DTBS_DIR)/$(name).dtb) && $(K_SOURCE_DIR)/scripts/dtc/dtc -I dts -O dtb -o $(K_DTBS_DIR)/$(name).dtb $(K_SOURCE_DIR)/arch/$(ARCH)/boot/dts/$(name).dts; )
endif
endif

#
# This target can be used to manage the configuration file.
#
configure: setup
	$(K_MAKE) menuconfig
	cp $(K_SOURCE_DIR)/.config $(K_CONFIG)

.DEFAULT_GOAL := dtbs
