# ryft-odbc

Steps to install libraries required for server build:
1. sudo apt-get install libjson0 libjson0-dev
2. sudo apt-get install uuid-dev libuuid1
3. sudo apt-get install libcurl4-gnutls-dev
4. sudo apt-get install libglib2.0-dev
5. sudo apt-get install libconfig-dev
6. unzip latest Simba SDK (currently build with SimbaEngineSDK_Release_Linux-x86_10.0.7.1028.tar.gz)

Set environment variables for LANG, SIMBAENGINE_DIR and SIMBAENGINE_THIRDPARTY_DIR, e.g.:
export LANG=en_US.UTF-8
export SIMBAENGINE_DIR=/usr/local/simba/SimbaEngineSDK/10.0/DataAccessComponents
export SIMBAENGINE_THIRDPARTY_DIR=/usr/local/simba/SimbaEngineSDK/10.0/DataAccessComponents/ThirdParty

Run build_install.sh from the ryft-odbc root to build and package the installer.

