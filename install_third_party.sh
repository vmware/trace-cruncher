#!/bin/bash

TRUNK=${PWD}
THIRD_PARTY_LIB=${TRUNK}/tracecruncher/lib/libtracecmd.so

if [ -f ${THIRD_PARTY_LIB} ]; then
    exit 0
fi

TAG=kernelshark-v1.1
THIRD_PARTY_DIR=${TRUNK}/third_party
mkdir ${THIRD_PARTY_DIR}
cd ${THIRD_PARTY_DIR}

echo 'Installing: ' ${TAG}

git clone git://git.kernel.org/pub/scm/utils/trace-cmd/trace-cmd.git --branch=${TAG}

cd trace-cmd
git am ${TRUNK}/0001-kernel-shark-*
git am ${TRUNK}/0002-kernel-shark-*

make prefix=${THIRD_PARTY_DIR} install_libs

cd kernel-shark/build/
cmake -D_DEVEL=1 -D_LIBS=1 -D_INSTALL_PREFIX=${THIRD_PARTY_DIR} -D_RPATH_TO_ORIGIN=1 ..
make install

cd ${TRUNK}

LIB_DIR=${TRUNK}/tracecruncher/lib/
if [ ! -d "${LIB_DIR}" ]; then
    mkdir ${LIB_DIR}
fi

cp -v ${THIRD_PARTY_DIR}/lib/kernelshark/libkshark.so.* ${TRUNK}/tracecruncher/lib/
cp -v ${THIRD_PARTY_DIR}/lib/kernelshark/plugins/*.so ${TRUNK}/tracecruncher/lib/
cp -v ${THIRD_PARTY_DIR}/lib/traceevent/*so ${TRUNK}/tracecruncher/lib/
cp -v ${THIRD_PARTY_DIR}/lib/trace-cmd/*.so ${TRUNK}/tracecruncher/lib/
