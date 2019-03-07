#!/bin/bash

# default environment settings
if [ -z "${LANG}" ] ; then
    export LANG=en_US.UTF-8
fi
if [ -z "${SIMBAENGINE_DIR}" ] ; then
    export SIMBAENGINE_DIR=$HOME/development/SimbaEngineSDK/10.0/DataAccessComponents
fi
if [ -z "${SIMBAENGINE_THIRDPARTY_DIR}" ] ; then
    export SIMBAENGINE_THIRDPARTY_DIR=$HOME/development/SimbaEngineSDK/10.0/DataAccessComponents/ThirdParty
fi

ver=$(dos2unix < files/server/VERSION)
a=( ${ver//./ } )
version="${a[0]}.${a[1]}"
build="${a[2]}.${a[3]}"
directory="ryft1_odbc_server_64"
echo building ${version}-${build}

# libmeta
cd src/libmeta
make CONFIG=Release clean
make CONFIG=Release
if [ $? -ne 0 ] ; then
    cd ../..
    exit 1
fi
cd ../..

# libsqlite
cd src/libsqlite
make CONFIG=Release clean
make CONFIG=Release
if [ $? -ne 0 ] ; then
    cd ../..
    exit 1
fi
cd ../..

# odbcctl
cd src/odbcctl
make CONFIG=Release clean
make CONFIG=Release
if [ $? -ne 0 ] ; then
    cd ../..
    exit 1
fi
cd ../..

# odbcd
cd src/odbcd/Makefiles
make -f ryft1_odbcd.mak BUILDSERVER=exe ARCH=x8664 clean
make -f ryft1_odbcd.mak BUILDSERVER=exe ARCH=x8664 release
if [ $? -ne 0 ] ; then
    cd ../../..
    exit 1
fi
cd ../../..

# package
rm -r ${directory}
mkdir ${directory}

cp files/server/{install.sh,VERSION,README,r.ld.so.conf,ryftodbcd.template} ${directory}
dos2unix ${directory}/{install.sh,VERSION,README,r.ld.so.conf,ryftodbcd.template}
chmod 755 ${directory}/install.sh

mkdir ${directory}/bin
mkdir ${directory}/bin/x8664
cp src/odbcd/Bin/Linux_x8664/ryft1_odbcd ${directory}/bin/x8664
cp src/odbcctl/Release/ryft1_odbcctl ${directory}/bin/x8664
cp files/server/.ryftone.server.ini ${directory}/bin/x8664
dos2unix ${directory}/bin/x8664/.ryftone.server.ini
cp ${directory}/VERSION ${directory}/bin/x8664/.version

mkdir ${directory}/lib
mkdir ${directory}/lib/x8664
cp -P ${SIMBAENGINE_THIRDPARTY_DIR}/icu/53.1/centos5/gcc4_4/release64/lib/{libicudata_sb64*,libicui18n_sb64*,libicuuc_sb64*} ${directory}/lib/x8664

cp -r files/server/errormessages ${directory}
WHICHPLATFORM=`cat /etc/os-release|grep -io centos|head -1|tr '[A-Z]' '[a-z]'`
if [ "${WHICHPLATFORM}" == "centos" ]
then
	cp -P ${SIMBAENGINE_THIRDPARTY_DIR}/openssl/1.0.1/centos5/gcc4_4/release64/lib/{libcrypto*,libssl*} ${directory}/lib/x8664
	tar -zcvf ryft1_odbc_server_linux_64-${version}-${build}_${WHICHPLATFORM}.tar.gz ${directory}
else
	tar -zcvf ryft1_odbc_server_linux_64-${version}-${build}.tar.gz ${directory}
fi

rm -r ${directory}
