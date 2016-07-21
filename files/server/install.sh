#!/bin/bash

# move required files to a common location
if [ ! -d "/usr/local/ryft" ]; then
    mkdir /usr/local/ryft
else 
    /usr/local/ryft/bin/x8664/ryft1_odbcctl -k
fi

if [ -f "/usr/local/ryft/bin/x8664/.ryftone.server.ini" ]; then
    mv /usr/local/ryft/bin/x8664/.ryftone.server.ini /usr/local/ryft/bin/x8664/.ryftone.server.ini.1
fi

cp -avr * /usr/local/ryft

# configure ini files
chmod 666 /usr/local/ryft/bin/x8664/.ryftone.server.ini

# attach root sticky bit to odbc binaries
chown root:root /usr/local/ryft/bin/x8664/ryft1_odbcctl
chmod 6755 /usr/local/ryft/bin/x8664/ryft1_odbcctl

chown root:root /usr/local/ryft/bin/x8664/ryft1_odbcd
chmod 6755 /usr/local/ryft/bin/x8664/ryft1_odbcd

# update shared library cache
cp r.ld.so.conf /etc/ld.so.conf.d
ldconfig

# upstart file
sed 's/\r//g' ryftodbcd.template > ryftodbcd.conf
cp ryftodbcd.conf /etc/init
chmod 644 /etc/init/ryftodbcd.conf
