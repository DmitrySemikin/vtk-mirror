#!/bin/bash

## This script is being run from the repo, so we assume, that
## the repo is already cloned, i.e. the commands below were already
## run.
# sudo apt-get -y install git
# git clone https://github.com/DmitrySemikin/vtk-mirror.git

# For Actions we don't need to install git (and I guess also cmake).
# And cloning is done using action

STARTTIME=$(date +%s)


INSTALL_STARTTIME=$(date +%s)

sudo apt-get -y install \
  git \
  g++ \
  cmake \
  ninja-build \
  libgl1-mesa-dev \
  qtbase5-dev \
  pbzip2

RC=$?

INSTALL_ENDTIME=$(date +%s)
echo "It takes $(($INSTALL_ENDTIME - $INSTALL_STARTTIME)) seconds to complete the installation..."

if [[ ${RC} != 0 ]]; then
  echo "ERROR: Failed to install packages" >&2
  exit 1;
fi


BUILD_STARTTIME=$(date +%s)

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
SRC_DIR="${SCRIPT_DIR}/.."
WORKING_DIR="$( cd "${SRC_DIR}/.." >/dev/null 2>&1 && pwd )"
BUILD_DIR_NAME="vtk-build"
INSTALL_DIR_NAME="vtk-install"
BUILD_DIR="${WORKING_DIR}/${BUILD_DIR_NAME}"
INSTALL_DIR="${WORKING_DIR}/${INSTALL_DIR_NAME}"

ENV_DESCRIPTION_FILE_NAME="vtk-build-env.txt"
ENV_DESCRIPTION_FILE="${WORKING_DIR}/${ENV_DESCRIPTION_FILE_NAME}"
echo "GCC --version" > "${ENV_DESCRIPTION_FILE}"
gcc --version 2>&1 >> "${ENV_DESCRIPTION_FILE}"
echo "qmake --version" >> "${ENV_DESCRIPTION_FILE}"
qmake --version 2>&1 >> "${ENV_DESCRIPTION_FILE}"
echo "cmake --version" >> "${ENV_DESCRIPTION_FILE}"
cmake --version 2>&1 >> "${ENV_DESCRIPTION_FILE}"
echo "ninja --version" >> "${ENV_DESCRIPTION_FILE}"
ninja --version 2>&1 >> "${ENV_DESCRIPTION_FILE}"

BUILD_LOG_NAME="build.log"
BUILD_LOG="${WORKING_DIR}/${BUILD_LOG_NAME}"

mkdir -p "${BUILD_DIR}" \
&& cmake \
  -G Ninja \
  -DCMAKE_VERBOSE_MAKEFILE=ON \
  -DVTK_GROUP_ENABLE_Qt=YES \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
  -S "${SRC_DIR}" \
  -B "${BUILD_DIR}"\
  2>&1 > "${BUILD_LOG}" \
&& cmake --build "${BUILD_DIR}" \
  2>&1 >> "${BUILD_LOG}" \
&& cmake --install "${BUILD_DIR}" \
  2>&1 > "${BUILD_LOG}"

RC=$?

BUILD_ENDTIME=$(date +%s)
echo "It takes $(($BUILD_ENDTIME - $BUILD_STARTTIME)) seconds to complete the build..."

if [[ ${RC} != 0 ]]; then
    echo "ERROR: Failed to build the package" >&2
    echo "Return code: ${RC}"
    exit 1;
fi


ARCH_STARTTIME=$(date +%s)

tar \
  --create \
  --file=vtk-binaries.tar.bz2 \
  --use-compress-program=pbzip2 \
  --directory="${WORKING_dir}" \
  "${BUILD_DIR_NAME}" \
  "${INSTALL_DIR_NAME}" \
  "${ENV_DESCRIPTION_FILE_NAME}" \
  "${BUILD_LOG_NAME}"

ARCH_ENDTIME=$(date +%s)
echo "It takes $(($ARCH_ENDTIME - $ARCH_STARTTIME)) seconds to complete the arch..."


ENDTIME=$(date +%s)
echo "It takes $(($ENDTIME - $STARTTIME)) seconds to complete full task..."
