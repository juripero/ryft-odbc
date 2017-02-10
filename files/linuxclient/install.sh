#!/bin/bash

if [ -x /usr/bin/clear ]; then
    /usr/bin/clear
fi
cat <<EOF
Installing Ryft Linux ODBC Client Driver
EOF

if [ "$(id -u)" == "0" ]; then
    defpath="/usr/local"
else
    defpath="$HOME"
fi

echo -n "Install to directory [$defpath]: "
read path
if [ "$path" == "" ]; then
    path="$defpath"
fi
if [ ! -d "$path" ]; then
    mkdir $path
fi
fullpath=`echo $path/ryft | sed 's,//,/,g'`
if [ ! -d "$fullpath" ]; then
    mkdir $fullpath
fi

echo -n "What is the IP address of the Ryft ODBC Server [127.0.0.1]? "
read ipaddr
if [ "$ipaddr" == "" ]; then
    ipaddr="127.0.0.1"
fi

echo -n "What is the IP port of the Ryft ODBC Server [7409]? "
read port
if [ "$port" == "" ]; then
    port="7409"
fi

cat odbc.template | sed "s#_INSTALLDIR_#$fullpath#;s#_IPADDR_#$ipaddr#;s#_PORT_#$port#" > odbc.ini
cat odbcinst.template | sed "s#_INSTALLDIR_#$fullpath#" > odbcinst.ini
cat simbaclient.template | sed "s#_INSTALLDIR_#$fullpath#" > simbaclient.ini
cat r.ld.so.conf.template | sed "s#_INSTALLDIR_#$fullpath#" > r.ld.so.conf

# move required files to a common location
cp -avr * $fullpath
mv $fullpath/simbaclient.ini $HOME/.simbaclient.ini

# update shared library cache
if [ "$(id -u)" == "0" ]; then
    cp r.ld.so.conf /etc/ld.so.conf.d
    ldconfig
fi

echo -n "Installation complete... [enter] "
read foo

if [ -x /usr/bin/clear ]; then
    /usr/bin/clear
fi

cat README