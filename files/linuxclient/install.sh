#!/bin/bash

# move required files to a common location
mkdir /usr/local/ryft
cp -avr * /usr/local/ryft

# configure odbc ini files
cat odbc.ini >> ~/.odbc.ini
chmod 666 ~/.odbc.ini
cp simbaclient.ini ~/.simbaclient.ini
chmod 666 ~/.simbaclient.ini

# update shared library cache
cp r.ld.so.conf /etc/ld.so.conf.d
ldconfig
