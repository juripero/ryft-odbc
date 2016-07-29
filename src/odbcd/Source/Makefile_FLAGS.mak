#===================================================================================================
# Simba Technologies Inc.
# Copyright (C) 2009-2010 Simba Technologies Incorporated
#
# Defines FLAGS (CFLAGS, LDFLAGS, etc) for the project.
#===================================================================================================

##--------------------------------------------------------------------------------------------------
## Common CFLAGS.
##--------------------------------------------------------------------------------------------------
COMMON_CFLAGS = $(DMFLAGS) \
-I. \
-I./Core \
-I./DataEngine \
-I./DataEngine/Metadata \
-I./DataEngine/Passdown \
-I./Ryft1 \
-I../../libmeta \
-I../../libsqlite \
-I$(SIMBAENGINE_DIR)/Include/DSI \
-I$(SIMBAENGINE_DIR)/Include/DSI/Client \
-I$(SIMBAENGINE_DIR)/Include/Support \
-I$(SIMBAENGINE_DIR)/Include/Support/Exceptions \
-I$(SIMBAENGINE_DIR)/Include/Support/TypedDataWrapper \
-I$(SIMBAENGINE_DIR)/Include/SQLEngine \
-I$(SIMBAENGINE_DIR)/Include/SQLEngine/AETree \
-I$(SIMBAENGINE_DIR)/Include/SQLEngine/Parser \
-I$(SIMBAENGINE_DIR)/Include/SQLEngine/DSIExt \
-I$(SIMBAENGINE_DIR)/Include/Server \
-I$(SIMBAENGINE_THIRDPARTY_DIR)/Expat/2.0.1 \
-I/usr/include/glib-2.0 \
-I/usr/lib/x86_64-linux-gnu/glib-2.0/include \
-DHAVE_MEMMOVE

ifeq ($(BUILDSERVER),exe)
CFLAGS = $(COMMON_CFLAGS) -I$(SIMBAENGINE_DIR)/Include/Server -DSERVERTARGET
else
CFLAGS = $(COMMON_CFLAGS)
endif

ifeq ($(OS), Darwin)
ifneq ($(findstring clang,$(PLATFORM)),)
CFLAGS += -std=c++11 
endif
endif

SIMBA_LIB_PATH = $(SIMBAENGINE_DIR)/Lib/$(LIB_PLATFORM)

## Set the base set of libs.
SIMBA_LIBS = $(SIMBA_LIB_PATH)/libSimbaDSI_<TARGET>.a,$(SIMBA_LIB_PATH)/libSimbaSupport_<TARGET>.a,$(SIMBA_LIB_PATH)/libAEProcessor_<TARGET>.a,$(SIMBA_LIB_PATH)/libCore_<TARGET>.a,$(SIMBA_LIB_PATH)/libDSIExt_<TARGET>.a,$(SIMBA_LIB_PATH)/libExecutor_<TARGET>.a,$(SIMBA_LIB_PATH)/libParser_<TARGET>.a

ifeq ($(BUILDSERVER),exe)

SIMBA_LIBS := $(SIMBA_LIBS),$(SIMBA_LIB_PATH)/libSimbaCommunications_<TARGET>.a,$(SIMBA_LIB_PATH)/libSimbaMessages_<TARGET>.a,$(SIMBA_LIB_PATH)/libSimbaServer_<TARGET>.a

else # BUILDSERVER != exe

# For SimbaODBC only, replace _<TARGET> with unixODBC_<TARGET>.
ifeq ($(UNIXODBC),1)
SIMBA_LIBS := $(SIMBA_LIBS),$(SIMBA_LIB_PATH)/libSimbaODBC_unixODBC_<TARGET>.a
else
SIMBA_LIBS := $(SIMBA_LIBS),$(SIMBA_LIB_PATH)/libSimbaODBC_<TARGET>.a
endif

endif

comma := ,
empty :=
space := $(empty) $(empty)

# Create space seperated list of Simba Lib dependencies so that the driver or server binary will be
# relinked if a Simba library is updated.
# (Note, this will not trigger recompiling DSII source files if Simba library headers
# are updated)
# The list should have _<TARGET> which will be processed by the Simba makefiles.
SIMBA_LIB_DEPENDENCIES = $(subst $(comma),$(space),$(SIMBA_LIBS))

##--------------------------------------------------------------------------------------------------
## AIX
##--------------------------------------------------------------------------------------------------
ifeq ($(OS),AIX)

ifeq ($(USE_GCC),1)
EXPORT_DEF=-Wl,-bM:SRE,-bnoentry,-bE:exports_AIX.map
else
EXPORT_DEF = -bE:exports_AIX.map
endif

## Transform the SIMBA_LIBS to replace commas with spaces. This platform separates lib names with
## spaces.
SIMBA_LIBS_NORMALIZED = $(subst $(comma),$(space),$(SIMBA_LIBS))

SIMBA_LIBS_DEBUG = $(subst _<TARGET>,_debug,$(SIMBA_LIBS_NORMALIZED))
SIMBA_LIBS_RELEASE = $(subst _<TARGET>,,$(SIMBA_LIBS_NORMALIZED))

COMMON_LDFLAGS = -L$(ICULIB_PATH) $(ICU_LIBS)

ifeq ($(BUILDSERVER),exe)
COMMON_LDFLAGS :=$(COMMON_LDFLAGS) -L$(OPENSSLLIB_PATH) $(OPENSSL_LIBS) -lnsl -lc
else
COMMON_LDFLAGS :=$(COMMON_LDFLAGS) $(EXPORT_DEF) -lc
endif

ifeq ($(BUILDSERVER),exe)
BIN_LDFLAGS = $(COMMON_LDFLAGS) $(SIMBA_LIBS_RELEASE)
BIN_LDFLAGS_DEBUG = $(COMMON_LDFLAGS) $(SIMBA_LIBS_DEBUG)
else
SO_LDFLAGS       = $(COMMON_LDFLAGS) $(SIMBA_LIBS_RELEASE)
SO_LDFLAGS_DEBUG = $(COMMON_LDFLAGS) $(SIMBA_LIBS_DEBUG)
endif


##--------------------------------------------------------------------------------------------------
## HP-UX
##--------------------------------------------------------------------------------------------------
else
ifeq ($(OS),HP-UX)

## Export map file
EXPORT_DEF = 

SIMBA_LIBS_DEBUG = $(subst _<TARGET>,_debug,$(SIMBA_LIBS))
SIMBA_LIBS_RELEASE = $(subst _<TARGET>,,$(SIMBA_LIBS))


COMMON_LDFLAGS = -L$(ICULIB_PATH) $(ICU_LIBS) $(EXPORT_DEF)

ifeq ($(BUILDSERVER),exe)
COMMON_LDFLAGS := -L$(OPENSSLLIB_PATH) $(OPENSSL_LIBS) $(COMMON_LDFLAGS)
endif

ifeq ($(BUILDSERVER),exe)
BIN_LDFLAGS = $(COMMON_LDFLAGS) \
               -Wl,+forceload,$(SIMBA_LIBS_RELEASE) -Wl,+noforceload

BIN_LDFLAGS_DEBUG = $(COMMON_LDFLAGS) \
               -Wl,+forceload,$(SIMBA_LIBS_DEBUG) -Wl,+noforceload
else
SO_LDFLAGS       = $(COMMON_LDFLAGS) \
               -Wl,+forceload,$(SIMBA_LIBS_RELEASE)  -Wl,+noforceload\
               -Wl,+h=$(TARGET_SO_RELEASE)

SO_LDFLAGS_DEBUG = $(COMMON_LDFLAGS) \
               -Wl,+forceload,$(SIMBA_LIBS_DEBUG) -Wl,+noforceload\
               -Wl,+h=$(TARGET_SO_DEBUG)
endif


##--------------------------------------------------------------------------------------------------
## Linux
##--------------------------------------------------------------------------------------------------
else
ifeq ($(OS),Linux)

## Export map file
EXPORT_DEF = -Wl,--version-script=exports_Linux.map

SIMBA_LIBS_DEBUG = $(subst _<TARGET>,_debug,$(SIMBA_LIBS))
SIMBA_LIBS_RELEASE = $(subst _<TARGET>,,$(SIMBA_LIBS))

SONAME_RELEASE=$(notdir $(TARGET_SO_RELEASE))
SONAME_DEBUG=$(notdir $(TARGET_SO_DEBUG))

COMMON_LDFLAGS = -L$(ICULIB_PATH) $(ICU_LIBS) $(EXPORT_DEF) -ldl -lconfig -lglib-2.0 -lldap -lryftone -lcrypt -luuid -ljson -lcurl

ifeq ($(BUILDSERVER),exe)
COMMON_LDFLAGS := -L$(OPENSSLLIB_PATH) $(OPENSSL_LIBS) $(COMMON_LDFLAGS)
endif

ifneq ($(EVALFLAG),) 
COMMON_LDFLAGS += -L$(OPENSSLLIB_PATH) $(OPENSSL_LIBS)
endif

ifeq ($(BUILDSERVER),exe)
BIN_LDFLAGS = -Wl,--whole-archive,$(SIMBA_LIBS_RELEASE) -Wl,--no-whole-archive $(COMMON_LDFLAGS) \
              -Wl,--no-whole-archive,../../libmeta/Release/libmeta.a \
              -Wl,--no-whole-archive,../../libsqlite/Release/libsqlite.a
BIN_LDFLAGS_DEBUG = -Wl,--whole-archive,$(SIMBA_LIBS_DEBUG) -Wl,--no-whole-archive $(COMMON_LDFLAGS) \
                    -Wl,--no-whole-archive,../../libmeta/Debug/libmeta.a \
                    -Wl,--no-whole-archive,../../libsqlite/Debug/libsqlite.a
else
SO_LDFLAGS       = -Wl,--whole-archive,$(SIMBA_LIBS_RELEASE) -Wl,--no-whole-archive \
                 -Wl,--soname=$(SONAME_RELEASE) \
                 $(COMMON_LDFLAGS) 

SO_LDFLAGS_DEBUG = -Wl,--whole-archive,$(SIMBA_LIBS_DEBUG) -Wl,--no-whole-archive \
                 -Wl,--soname=$(SONAME_DEBUG) \
                 $(COMMON_LDFLAGS) 
endif

##--------------------------------------------------------------------------------------------------
## Solaris
##--------------------------------------------------------------------------------------------------
else
ifeq ($(OS),Solaris)


ifeq ($(USE_GCC),1)
## Export map file
EXPORT_DEF = -Wl,-M,exports_Solaris.map

SIMBA_LIBS_DEBUG = $(subst _<TARGET>,_debug,$(SIMBA_LIBS))
SIMBA_LIBS_RELEASE = $(subst _<TARGET>,,$(SIMBA_LIBS))

COMMON_LDFLAGS = -L$(ICULIB_PATH) $(ICU_LIBS) -lsocket -lnsl 

ifeq ($(BUILDSERVER),exe)

COMMON_LDFLAGS := -L$(OPENSSLLIB_PATH) $(OPENSSL_LIBS) $(COMMON_LDFLAGS)

BIN_LDFLAGS       = $(COMMON_LDFLAGS) \
               -Wl,-zallextract,$(SIMBA_LIBS_RELEASE) -Wl,-zweakextract \


BIN_LDFLAGS_DEBUG = $(COMMON_LDFLAGS) \
               -Wl,-zallextract,$(SIMBA_LIBS_DEBUG) -Wl,-zweakextract \

else

COMMON_LDFLAGS := $(COMMON_LDFLAGS) $(EXPORT_DEF)

SO_LDFLAGS       = $(COMMON_LDFLAGS) \
               -Wl,-zallextract,$(SIMBA_LIBS_RELEASE) -Wl,-zweakextract \


SO_LDFLAGS_DEBUG = $(COMMON_LDFLAGS) \
               -Wl,-zallextract,$(SIMBA_LIBS_DEBUG) -Wl,-zweakextract \


endif


else

## Export map file
EXPORT_DEF = -M exports_Solaris.map

## Transform the SIMBA_LIBS to replace commas with spaces. This platform separates lib names with
## spaces.
SIMBA_LIBS_NORMALIZED = $(subst $(comma),$(space),$(SIMBA_LIBS))

SIMBA_LIBS_DEBUG = $(subst _<TARGET>,_debug,$(SIMBA_LIBS_NORMALIZED))
SIMBA_LIBS_RELEASE = $(subst _<TARGET>,,$(SIMBA_LIBS_NORMALIZED))

COMMON_LDFLAGS = -L$(ICULIB_PATH) $(ICU_LIBS) -lsocket -lnsl 

ifeq ($(BUILDSERVER),exe)
COMMON_LDFLAGS := -L$(OPENSSLLIB_PATH) $(OPENSSL_LIBS) $(COMMON_LDFLAGS)
else
COMMON_LDFLAGS := $(COMMON_LDFLAGS) $(EXPORT_DEF)

endif

ifeq ($(BUILDSERVER),exe)

BIN_LDFLAGS       = $(COMMON_LDFLAGS) \
               -zallextract $(SIMBA_LIBS_RELEASE) -zweakextract \


BIN_LDFLAGS_DEBUG = $(COMMON_LDFLAGS) \
               -zallextract $(SIMBA_LIBS_DEBUG) -zweakextract \

else

SO_LDFLAGS       = $(COMMON_LDFLAGS) \
               -zallextract $(SIMBA_LIBS_RELEASE) -zweakextract \


SO_LDFLAGS_DEBUG = $(COMMON_LDFLAGS) \
               -zallextract $(SIMBA_LIBS_DEBUG) -zweakextract \


endif

endif

##--------------------------------------------------------------------------------------------------
## Mac OSX (Darwin)
##--------------------------------------------------------------------------------------------------
else
ifeq ($(OS),Darwin)

## Export map file
EXPORT_DEF = -exported_symbols_list exports_Darwin.map

## Transform the SIMBA_LIBS to replace commas with spaces. This platform separates lib names with
## spaces.
SIMBA_LIBS_NORMALIZED = $(subst $(comma),$(space),$(SIMBA_LIBS))

SIMBA_LIBS_DEBUG = $(subst _<TARGET>,_debug,$(SIMBA_LIBS_NORMALIZED))
SIMBA_LIBS_RELEASE = $(subst _<TARGET>,,$(SIMBA_LIBS_NORMALIZED))

SONAME_RELEASE=$(notdir $(TARGET_SO_RELEASE))
SONAME_DEBUG=$(notdir $(TARGET_SO_DEBUG))

COMMON_LDFLAGS = -L$(ICULIB_PATH) $(ICU_LIBS)

ifeq ($(BUILDSERVER),exe)
COMMON_LDFLAGS := -L$(OPENSSLLIB_PATH) $(OPENSSL_LIBS) $(COMMON_LDFLAGS)
else
ifneq ($(EVALFLAG),) 
COMMON_LDFLAGS += -L$(OPENSSLLIB_PATH) $(OPENSSL_LIBS) $(EXPORT_DEF)
else
COMMON_LDFLAGS := $(COMMON_LDFLAGS) $(EXPORT_DEF)
endif
endif

ifeq ($(BUILDSERVER),exe)
BIN_LDFLAGS       = $(COMMON_LDFLAGS) \
               -all_load $(SIMBA_LIBS_RELEASE)

BIN_LDFLAGS_DEBUG = $(COMMON_LDFLAGS) \
               -all_load $(SIMBA_LIBS_DEBUG)
else

SO_LDFLAGS       = $(COMMON_LDFLAGS) \
               -all_load $(SIMBA_LIBS_RELEASE) \
               -install_name=$(SONAME_RELEASE)

SO_LDFLAGS_DEBUG = $(COMMON_LDFLAGS) \
               -all_load $(SIMBA_LIBS_DEBUG) \
               -install_name=$(SONAME_DEBUG)
endif

## Darwin
endif
## Solaris
endif
## Linux
endif
## HP-UX
endif
## AIX
endif
