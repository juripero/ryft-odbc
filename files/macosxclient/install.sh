#!/bin/bash

if [ -x /usr/bin/clear ]; then
	/usr/bin/clear
fi
cat <<EOF

Installing Ryft Mac OS X ODBC Client Driver

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
if [ ! -d "$path/lib" ]; then
	mkdir $path/lib
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

cat odbc.template | sed "s#_INSTALLDIR_#$path#;s#_IPADDR_#$ipaddr#;s#_PORT_#$port#" > odbc.ini
chmod 666 odbc.ini
cat odbcinst.template | sed "s#_INSTALLDIR_#$path#" > odbcinst.ini
chmod 666 odbcinst.ini
cat simbaclient.template | sed "s#_INSTALLDIR_#$fullpath#" > simbaclient.ini
chmod 666 simbaclient.ini

# move required files to a common location
cp -avR errormessages/* $fullpath/errormessages 
cp -avR lib/* $path/lib
cp -av simbaclient.ini $HOME/.simbaclient.ini
chmod 666 $HOME/.simbaclient.ini 
cp -av README $fullpath/README
cp -av VERSION $fullpath/VERSION

if [ ! -d "$HOME/Library/ODBC" ]; then
	mkdir $HOME/Library/ODBC
	chmod 777 $HOME/Library/ODBC
	cp -av odbc.ini $HOME/Library/ODBC/odbc.ini
	ln -sf $HOME/Library/ODBC/odbc.ini $HOME/.odbc.ini
	cp -av odbcinst.ini $HOME/Library/ODBC/odbcinst.ini
	ln -sf $HOME/Library/ODBC/odbcinst.ini $HOME/.odbcinst.ini
fi

echo -n "Installation complete... [enter] "
read foo

if [ -x /usr/bin/clear ]; then
	/usr/bin/clear
fi

cat README

