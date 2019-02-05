
cd ~/ryft-odbc
sudo chcon -Rt svirt_sandbox_file_t  .
echo "###################################### centos.7"
docker run -v `pwd`:/opt/ryft-odbc -v /usr/local/simba:/usr/local/simba --user `id -u`:`id -g` -it centos.7:b1 /opt/ryft-odbc/buildit.sh

echo "###################################### ubuntu.14.04"
docker run -v `pwd`:/opt/ryft-odbc -v /usr/local/simba:/usr/local/simba --user `id -u`:`id -g` -it ubuntu.14.04:b1 /opt/ryft-odbc/buildit.sh

#echo "###################################### ubuntu.16.04"
#docker run -v `pwd`:/opt/ryft-odbc -v /usr/local/simba:/usr/local/simba --user `id -u`:`id -g` -it ubuntu.16.04:b1 /opt/ryft-odbc/buildit.sh
