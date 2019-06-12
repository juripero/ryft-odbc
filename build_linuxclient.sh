#!/bin/bash
 
RELDIR=blacklynx_odbc_client_linux
VERSION=2.1
RELEASE=20.0
 
rm -r $RELDIR
mkdir $RELDIR
mkdir $RELDIR/lib
mkdir $RELDIR/lib/x8664
mkdir $RELDIR/errormessages
mkdir $RELDIR/errormessages/en-US
cp -av ~/development/SimbaEngineSDK/10.0/DataAccessComponents/ThirdParty/icu/53.1/centos5/gcc4_4/release64/lib/*.so* $RELDIR/lib/x8664
cp -av ~/development/SimbaEngineSDK/10.0/DataAccessComponents/Bin/Linux_x8664/libSimbaClient.so $RELDIR/lib/x8664
cp -av ~/development/SimbaEngineSDK/10.0/DataAccessComponents/ErrorMessages/en-US/ClientMessages.xml $RELDIR/errormessages/en-US
cp -av ~/development/SimbaEngineSDK/10.0/DataAccessComponents/ErrorMessages/en-US/CSCommonMessages.xml $RELDIR/errormessages/en-US
cp -av ~/development/SimbaEngineSDK/10.0/DataAccessComponents/ErrorMessages/en-US/ODBCMessages.xml $RELDIR/errormessages/en-US
cp -av ~/development/SimbaEngineSDK/10.0/DataAccessComponents/ErrorMessages/en-US/NetworkMessages.xml $RELDIR/errormessages/en-US
cp -av ~/development/ryft-odbc/files/linuxclient/odbc.template $RELDIR
chmod 666 $RELDIR/odbc.template 
cp -av ~/development/ryft-odbc/files/linuxclient/odbcinst.template $RELDIR
chmod 666 $RELDIR/odbcinst.template
cp -av ~/development/ryft-odbc/files/linuxclient/simbaclient.template $RELDIR
chmod 666 $RELDIR/simbaclient.template
cp -av ~/development/ryft-odbc/files/linuxclient/r.ld.so.conf.template $RELDIR
chmod 666 $RELDIR/r.ld.so.conf.template
cp -av ~/development/ryft-odbc/files/linuxclient/README $RELDIR
chmod 666 $RELDIR/README
cp -av ~/development/ryft-odbc/files/linuxclient/VERSION $RELDIR
chmod 666 $RELDIR/VERSION
cp -av ~/development/ryft-odbc/files/linuxclient/install.sh $RELDIR
chmod 777 $RELDIR/install.sh
tar -czf ~/development/$RELDIR-$VERSION-$RELEASE.tar.gz $RELDIR
rm -r $RELDIR

