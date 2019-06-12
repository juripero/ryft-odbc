#!/bin/bash

RELDIR=ryft1_odbc_client_macosx
VERSION=2.1
RELEASE=19.0

rm -r $RELDIR
mkdir $RELDIR
mkdir $RELDIR/lib
mkdir $RELDIR/errormessages
mkdir $RELDIR/errormessages/en-US
cp -av ~/Development/Ryft/SimbaEngineSDK/10.0/DataAccessComponents/ThirdParty/icu/53.1/osx10_9/xcode6_1/release3264/lib/*.dylib $RELDIR/lib
cp -av ~/Development/Ryft/SimbaEngineSDK/10.0/DataAccessComponents/ThirdParty/openssl/1.0.1/osx10_9/xcode6_1/release3264/lib/*.dylib $RELDIR/lib
cp -av ~/Development/Ryft/SimbaEngineSDK/10.0/DataAccessComponents/Bin/Darwin_universal_clang/libSimbaClient.dylib $RELDIR/lib
cp -av ~/development/Ryft/SimbaEngineSDK/10.0/DataAccessComponents/ErrorMessages/en-US/ClientMessages.xml $RELDIR/errormessages/en-US
cp -av ~/development/Ryft/SimbaEngineSDK/10.0/DataAccessComponents/ErrorMessages/en-US/CSCommonMessages.xml $RELDIR/errormessages/en-US
cp -av ~/development/Ryft/SimbaEngineSDK/10.0/DataAccessComponents/ErrorMessages/en-US/ODBCMessages.xml $RELDIR/errormessages/en-US
cp -av ~/development/Ryft/SimbaEngineSDK/10.0/DataAccessComponents/ErrorMessages/en-US/NetworkMessages.xml $RELDIR/errormessages/en-US
cp -av ~/Development/Ryft/ryft-odbc/files/macosxclient/odbc.template $RELDIR
chmod 666 $RELDIR/odbc.template 
cp -av ~/Development/Ryft/ryft-odbc/files/macosxclient/odbcinst.template $RELDIR
chmod 666 $RELDIR/odbcinst.template
cp -av ~/Development/Ryft/ryft-odbc/files/macosxclient/simbaclient.template $RELDIR
chmod 666 $RELDIR/simbaclient.template
cp -av ~/Development/Ryft/ryft-odbc/files/macosxclient/README $RELDIR
chmod 666 $RELDIR/README
cp -av ~/Development/Ryft/ryft-odbc/files/macosxclient/VERSION $RELDIR
chmod 666 $RELDIR/VERSION
cp -av ~/Development/Ryft/ryft-odbc/files/macosxclient/install.sh $RELDIR
chmod 777 $RELDIR/install.sh
tar -czf ~/Development/Ryft/$RELDIR-$VERSION-$RELEASE.tar.gz $RELDIR
rm -r $RELDIR
