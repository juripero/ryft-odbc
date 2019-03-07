#!/bin/bash

getScriptDir() {
    dir=`dirname ${BASH_SOURCE}`
    cd $dir
    fullpathname=`pwd`
    cd - >/dev/null
    echo $fullpathname
}

scriptDir=`getScriptDir`
cd ${scriptDir}

. ./build_environment.sh
bash -x ./build_install_orig.sh
