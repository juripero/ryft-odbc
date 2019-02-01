#!/bin/bash 

# tracking build packages specific to ODBC when porting to centos
# several ubuntu packages are identified ( but not all !)

# 0/1
cloneODBC=0
osInstalls=0
simbaInstall=0

if [ "$cloneODBC" == 1 ]
then
	cd
	git clone https://github.com/getryft/ryft-odbc
	cd ryft-odbc
fi

# os installs
if [ "$osInstalls" == 1 ]
then
	if [ "${WHICHPLATFORM}" == "centos" ]
	then
		function centosInstall() {
			sudo yum install $1 -y
			[ $? != 0 ] && echo; echo "*** $1 problem ***"; echo
		}
	
		centosInstall json-c-devel
		centosInstall uuid-devel
		centosInstall libuuid-devel
		centosInstall gnutls-devel
		centosInstall curl-devel
		centosInstall GeoIP-devel
		centosInstall openldap-devel
		centosInstall glib2-devel
		centosInstall libconfig-devel
		centosInstall dos2unix
	else
		function ubuntuInstall() {
			sudo yum install $1 -y
			[ $? != 0 ] && echo; echo "*** $1 problem ***"; echo
		}
	
		#
		centosInstall libjson0
		centosInstall libjson0-dev 
		ubuntuInstall uuid-dev 
		ubuntuInstall libuuid1
		ubuntuInstall libcurl4-gnutls-dev
		#? ubuntuInstall GeoIP-devel
		#? ubuntuInstall openldap-devel
		ubuntuInstall libglib2.0-dev
		ubuntuInstall libconfig-dev
		ubuntuInstall dos2unix
	fi

fi

# simba install
if [ "$simbaInstall" == 1 ]
then
	scp bigdata@172.16.10.84:/usr/local/simba/SimbaEngineSDK_Release_Linux-x86_10.0.7.1028.tar.gz .
	sudo mkdir /usr/local/simba
	sudo cd /usr/local/simba; tar -xvf ~ryftuser/ryft-odbc/Simba*.gz
fi
