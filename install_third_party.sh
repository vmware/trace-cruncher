#!/bin/bash

TRUNK=${PWD}
THIRD_PARTY_LIB=${TRUNK}/tracecruncher/lib/libtracecmd.so

if [ -f ${THIRD_PARTY_LIB} ]; then
    exit 0
fi

THIRD_PARTY_DIR=${TRUNK}/third_party
mkdir ${THIRD_PARTY_DIR}
cd ${THIRD_PARTY_DIR}

echo 'Installing: ' ${TAG}

git clone git://git.kernel.org/pub/scm/utils/trace-cmd/trace-cmd.git
cd trace-cmd
make prefix=${THIRD_PARTY_DIR} install_libs
cd ${THIRD_PARTY_DIR}

git clone git@github.com:yordan-karadzhov/kernel-shark-2.alpha.git
cd kernel-shark-2.alpha/build/
export TRACE_CMD=${THIRD_PARTY_DIR}/trace-cmd
cmake -D_DEVEL=1 -D_LIBS=1 -D_INSTALL_PREFIX=${THIRD_PARTY_DIR} -D_RPATH_TO_ORIGIN=1 ..
make install

cd ${TRUNK}

LIB_DIR=${TRUNK}/tracecruncher/lib/
if [ ! -d "${LIB_DIR}" ]; then
    mkdir ${LIB_DIR}
fi

cp -v ${THIRD_PARTY_DIR}/lib/kernelshark/libkshark.so.* ${TRUNK}/tracecruncher/lib/
cp -v ${THIRD_PARTY_DIR}/lib/traceevent/*.so ${TRUNK}/tracecruncher/lib/
cp -v ${THIRD_PARTY_DIR}/lib/trace-cmd/*.so ${TRUNK}/tracecruncher/lib/
