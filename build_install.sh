#!/bin/bash

ver=$(dos2unix < ryft1_files/VERSION)
a=( ${ver//./ } )
version="${a[0]}.${a[1]}"
build="${a[2]}.${a[3]}"
directory="ryft1_odbc_server_64"
echo building ${version}-${build}

rm -r ${directory}
mkdir ${directory}

cp ryft1_files/{install.sh,VERSION,README,r.ld.so.conf,ryftodbcd.template} ${directory}
dos2unix ${directory}/{install.sh,VERSION,README,r.ld.so.conf,ryftodbcd.template}
chmod 777 ${directory}/install.sh

mkdir ${directory}/bin
mkdir ${directory}/bin/x8664
cp ryft1_odbcd/Bin/Linux_x8664/ryft1_odbcd ${directory}/bin/x8664
cp ryft1_odbcctl/Release/ryft1_odbcctl ${directory}/bin/x8664
cp ryft1_files/.ryftone.server.ini ${directory}/bin/x8664
dos2unix ${directory}/bin/x8664/.ryftone.server.ini
cp ${directory}/VERSION ${directory}/bin/x8664/.version

mkdir ${directory}/lib
mkdir ${directory}/lib/x8664
cp -P SimbaEngineSDK/10.0/DataAccessComponents/ThirdParty/icu/53.1/centos5/gcc4_4/release64/lib/{libicudata_sb64*,libicui18n_sb64*,libicuuc_sb64*} ${directory}/lib/x8664

cp -r ryft1_files/errormessages ${directory}
 
tar -zcvf ryft1_odbc_server_linux_64-${version}-${build}.tar.gz ${directory}

rm -r ${directory}