#! /bin/bash
if [ -z "$HOUDINI_DIR" ]; then
    export HOUDINI_DIR=/opt/hfs18.5.351
fi

# if [ -z "$HFS" ]; then
    export HFS=$HOUDINI_DIR
    export PATH=$HFS\bin:$PATH
    export CUR=`pwd`
    cd $HFS
    echo "HOUDINI DIR: "
    echo `pwd`
    source houdini_setup
    cd $CUR
# fi

if [ -z "$ARNOLD_DIR" ]; then
        export ARNOLD_DIR=deps/Arnold-6.1.0.0-linux
fi

# extra flags: -g: debug
DEBUG_FLAG=
# DEBUG_FLAG=-g
hcustom -e ${DEBUG_FLAG} -i ./build src/ai_ocean_samplelayers.cpp -L ${ARNOLD_DIR}/bin -lai -I ${ARNOLD_DIR}/include -L ${HOUDINI_DIR}/dsolib -lHoudiniUI -lHoudiniOPZ -lHoudiniOP3 -lHoudiniOP2 -lHoudiniOP1 -lHoudiniSIM -lHoudiniGEO -lHoudiniPRM -lHoudiniUT -lboost_system -lHoudiniOP4 -lpxr_pxOsd -losdCPU
