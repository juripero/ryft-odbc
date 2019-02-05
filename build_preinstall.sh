#!/bin/bash 

# tracking build packages specific to ODBC when porting to centos
# several ubuntu packages are identified ( but not all !)

# 0/1
cloneODBC=0
osInstalls=1
simbaInstall=0

WHICHPLATFORM=`cat /etc/os-release|grep -io centos|head -1|tr '[A-Z]' '[a-z]'`
echo "WHICHPLATFORM=${WHICHPLATFORM}"

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
			x=$?
			#echo "yum return code=$x"
			if [ $x != 0 ]
			then
				echo
				echo "*** problem - yum install $1 - returnCode $x ***"; 
				echo
			fi
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
			sudo apt-get --assume-yes install $1 
			x=$?
			#echo "apt-get return code=$x"
			if [ $x != 0 ]
			then
				echo
				echo "*** problem - apt-get install $1 - returnCode $x ***"; 
				echo
			fi
		}
	
		#
		ubuntuInstall libjson0
		ubuntuInstall libjson0-dev 
		ubuntuInstall uuid-dev 
		ubuntuInstall libuuid1
		ubuntuInstall libcurl4-gnutls-dev
		# following libs... less-certain
		ubuntuInstall geoip-dev
		ubuntuInstall geoip-database
		ubuntuInstall openldap2-dev
                ubuntuInstall openldap-2.4-2
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
	cd /usr/local/simba
	sudo tar -xvf ~ryftuser/ryft-odbc/Simba*.gz
fi
