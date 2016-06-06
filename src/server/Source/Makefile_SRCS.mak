#===================================================================================================
# Simba Technologies Inc.
# Copyright (C) 2009-2014 Simba Technologies Incorporated
#
# Defines SRCS for the project.
#===================================================================================================

##--------------------------------------------------------------------------------------------------
## Common Sources used to build this project.
##--------------------------------------------------------------------------------------------------
COMMON_SRCS = \
Core/R1Connection.cpp \
Core/R1Driver.cpp \
Core/R1Environment.cpp \
Core/R1Statement.cpp \
DataEngine/Metadata/R1CatalogOnlyMetadataSource.cpp \
DataEngine/Metadata/R1ColumnsMetadataSource.cpp \
DataEngine/Metadata/R1SchemaOnlyMetadataSource.cpp \
DataEngine/Metadata/R1TablesMetadataSource.cpp \
DataEngine/Metadata/R1TypeInfoMetadataSource.cpp \
DataEngine/Passdown/R1OperationHandlerFactory.cpp \
DataEngine/Passdown/R1FilterHandler.cpp \
DataEngine/Passdown/R1FilterResult.cpp \
DataEngine/R1DataEngine.cpp \
DataEngine/R1Table.cpp \
Ryft1/ryft1_catalog.cpp \
Ryft1/ryft1_result.cpp \
Ryft1/ryft1_util.cpp \

##--------------------------------------------------------------------------------------------------
## There are no platform specific sources.
##--------------------------------------------------------------------------------------------------
SRCS = $(COMMON_SRCS) \
Main_Unix.cpp