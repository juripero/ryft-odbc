#!/bin/bash 

# 0/1
cloneODBC=0
simbaSDKInstall=1

# project setup
if [ "$cloneODBC" == 1 ]
then
	cd
	git clone https://github.com/getryft/ryft-odbc
	cd ryft-odbc
fi

# simba SDK install
if [ "$simbaSDKInstall" == 1 ]
then
	scp bigdata@172.16.10.84:/usr/local/simba/SimbaEngineSDK_Release_Linux-x86_10.0.7.1028.tar.gz .
	sudo mkdir /usr/local/simba
	cd /usr/local/simba
	sudo tar -xvf ~ryftuser/ryft-odbc/Simba*.gz
fi
