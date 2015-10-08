#===================================================================================================
# Simba Technologies Inc.
# Copyright (C) 2009-2010 Simba Technologies Incorporated
#
# File: Source/Makefile
#===================================================================================================

## -------------------------------------------------------------------------------------------------
## PROJECT defines the project name. This is required for dependency inclusion.
## -------------------------------------------------------------------------------------------------
PROJECT = ryft1_odbcd

MAKEFILE_PATH = ../Makefiles

ifeq ($(BUILDSERVER),exe)
TARGET_BIN_PATH = ../Bin/$(PLATFORM)
TARGET_BIN = $(TARGET_BIN_PATH)/$(PROJECT)_<TARGET>
else
TARGET_SO_PATH = ../Bin/$(PLATFORM)
TARGET_SO = $(TARGET_SO_PATH)/lib$(PROJECT)_<TARGET>.$(SO_SUFFIX)
endif


## -------------------------------------------------------------------------------------------------
## Sources used to build this library.
## -------------------------------------------------------------------------------------------------
include ./Makefile_SRCS.mak


## -------------------------------------------------------------------------------------------------
## CFLAGS, LDFLAGS, SO_LDFLAGS, etc used.
## -------------------------------------------------------------------------------------------------
include ./Makefile_FLAGS.mak


##--------------------------------------------------------------------------------------------------
## Platform-dependent and global Makefile rules includes
##--------------------------------------------------------------------------------------------------
include $(SIMBAENGINE_DIR)/Makefiles/Rules/Rules_$(OS).mak
include $(SIMBAENGINE_DIR)/Makefiles/Rules/Rules.mak


##--------------------------------------------------------------------------------------------------
## Makedepend include
##--------------------------------------------------------------------------------------------------
include $(MAKEFILE_PATH)/Makedepend/$(PROJECT).depend
