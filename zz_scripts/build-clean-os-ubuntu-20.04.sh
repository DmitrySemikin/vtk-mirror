#!/bin/bash

## This script is being run from the repo, so we assume, that
## the repo is already cloned, i.e. the commands below were already
## run.
# sudo apt-get -y install git
# git clone https://github.com/DmitrySemikin/vtk-mirror.git

# For Actions we don't need to install git (and I guess also cmake).
# And cloning is done using action

STARTTIME=$(date +%s)

sudo apt-get -y install \
  cmake \
  git \
  g++ \
  libgl1-mesa-dev \
  qtbase5-dev \
  pbzip2

if [[ "$?" -ne "0" ]]; then
  echo "ERROR: Failed to install packages" >&2
  exit 1;
fi

# another possible variant: libqt5gui5 - but it seems to provide only runtime

BUILD_STARTTIME=$(date +%s)


SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
ROOT_DIR="${SCRIPT_DIR}/.."
BUILD_DIR="${ROOT_DIR}/../vtk-build"
INSTALL_DIR="${ROOT_DIR}/../vtk-install"

mkdir "${BUILD_DIR}" \
&& cd "${BUILD_DIR}" \
&& cmake -DVTK_GROUP_ENABLE_Qt=YES -DCMAKE_INSTALL_PREFIX= "${ROOT_DIR}" \
&& cmake --build . \
&& cmake --install .

if [[ "$?" -ne "0" ]]; then
    echo "ERROR: Failed to build the package" >&2
    exit 1;
fi

BUILD_ENDTIME=$(date +%s)
echo "It takes $(($BUILD_ENDTIME - $BUILD_STARTTIME)) seconds to complete the build..."



ARCH_STARTTIME=$(date +%s)

tar --create --file=vtk-binaries.tar.bz2 --use-compress-program=pbzip2 --directory=${BUILD_DIR}/.. ./vtk-build ./vtk-install

ARCH_ENDTIME=$(date +%s)
echo "It takes $(($ARCH_ENDTIME - $ARCH_STARTTIME)) seconds to complete the arch..."


ENDTIME=$(date +%s)
echo "It takes $(($ENDTIME - $STARTTIME)) seconds to complete full task..."
