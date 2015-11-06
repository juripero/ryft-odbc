#!/bin/bash

# move required files to a common location
mkdir /usr/local/ryft
cp -avr * /usr/local/ryft

# configure ini files
chmod 666 /usr/local/ryft/bin/x8664/.ryftone.server.ini

# attach root sticky bit to odbc binaries
chown root /usr/local/ryft/bin/x8664/ryft1_odbcctl
chmod 4777 /usr/local/ryft/bin/x8664/ryft1_odbcctl

chown root /usr/local/ryft/bin/x8664/ryft1_odbcd
chmod 4777 /usr/local/ryft/bin/x8664/ryft1_odbcd

# update shared library cache
cp r.ld.so.conf /etc/ld.so.conf.d
ldconfig
