#! /bin/bash
if [ -z "$HFS" ]; then
    echo "hello"
    export HFS=/opt/hfs18.0.416
    export PATH=$HFS\bin:$PATH
    export CUR=`pwd`
    cd $HFS
    echo `pwd`
    source houdini_setup
    cd $CUR
fi

# note: for Houdini 15.5, you need GCC 4.8!

ARNOLD_DIR=deps/Arnold-6.0.3.0-linux
HOUDINI_DIR=/opt/hfs18.0.416

hcustom -e -i ./build -g src/ai_ocean_samplelayers.cpp -L ${ARNOLD_DIR}/bin -lai -I ${ARNOLD_DIR}/include -L ${HOUDINI_DIR}/dsolib -lHoudiniUI -lHoudiniOPZ -lHoudiniOP3 -lHoudiniOP2 -lHoudiniOP1 -lHoudiniSIM -lHoudiniGEO -lHoudiniPRM -lHoudiniUT -lboost_system
