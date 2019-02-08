#!/bin/bash

#
# must be run from root of ryft-odbc project directory
#

if [ ${1} = "ubuntu" ]
then
	containerId=ubuntu.14.04:b1
	dockerFile=Dockerfile.ubuntu.14.14
else
	containerId=centos.7:b1
	dockerFile=Dockerfile.centos.7
fi

echo "* containerId=${containerId}, dockerFile=${dockerFile}"

docker inspect ${containerId} 
if [ $? != 0 ]
then
	echo "* docker inspecit ${containerId} did not find container.  Building it..."
	sudo docker build -f ./utilities/${dockerFile} -t ${containerId} ./utilities
fi
cat /etc/os-release|grep -i centos
if [ $? = 0 ]
then
	echo "* CentOS OS detected, enabling svirt_sandbox_file_t permission on current directory"
	sudo chcon -Rt svirt_sandbox_file_t  .
fi
docker run --rm -v `pwd`:/opt/ryft-odbc -v /usr/local/simba:/usr/local/simba --user `id -u`:`id -g` -it ${containerId} /opt/ryft-odbc/buildit.sh
