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

sudo apt-get update \
&& sudo apt-get -y install \
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

VTK_CMAKE_VARIABLES_VALUES_FILE_NAME="vtk-cmake-variables-values.txt"
VTK_CMAKE_VARIABLES_VALUES_FILE="${WORKING_DIR}/${VTK_CMAKE_VARIABLES_VALUES_FILE_NAME}"


# Defines variable $VTK_DISABLE_MODULES
source "${SCRIPT_DIR}/define-disable-modules-variable.sh"
if [[ -z "${VTK_DISABLE_MODULES}" ]]; then
    echo "ERROR: Failed to source script, which defines VTK_DISABLE_MODULES variable." >&2
    exit 1;
fi


mkdir -p "${BUILD_DIR}"
RC=$?
if [[ ${RC} != 0 ]]; then
    echo "ERROR: Failed to create build directory." >&2
    echo "Return code: ${RC}"
    exit 1;
fi


cmake \
  -G Ninja \
  -DCMAKE_VERBOSE_MAKEFILE=ON \
  -DVTK_GROUP_ENABLE_Qt=YES \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
  -DCMAKE_BUILD_TYPE="Release" \
  ${VTK_DISABLE_MODULES} \
  -S "${SRC_DIR}" \
  -B "${BUILD_DIR}" \
  2>&1 > "${BUILD_LOG}"

RC=$?
if [[ ${RC} != 0 ]]; then
    echo "ERROR: Failed to generate VTK project files (cmake run)." >&2
    echo "Return code: ${RC}"
    exit 1;
fi

cmake -LA -N -S "${SRC_DIR}" -B "${BUILD_DIR}" 2>&1 > "${VTK_CMAKE_VARIABLES_VALUES_FILE}"

cmake --build "${BUILD_DIR}" 2>&1 >> "${BUILD_LOG}"
RC=$?
if [[ ${RC} != 0 ]]; then
    echo "ERROR: Failed to build VTK." >&2
    echo "Return code: ${RC}"
    exit 1;
fi


# cmake --install "${BUILD_DIR}" 2>&1 >> "${BUILD_LOG}"

# RC=$?
# if [[ ${RC} != 0 ]]; then
#     echo "ERROR: Failed to install VTK" >&2
#     echo "Return code: ${RC}"
#     exit 1;
# fi


BUILD_ENDTIME=$(date +%s)
echo "It takes $(($BUILD_ENDTIME - $BUILD_STARTTIME)) seconds to complete the build..."



ARCH_STARTTIME=$(date +%s)

# It is impossible to specify input directory for cpack (or I could not find, how to do it),
# so we need to cd there

cd "${BUILD_DIR}"
RC=$?
if [[ ${RC} != 0 ]]; then
    echo "ERROR: Cannot CD to BUILD directory to invoke cpack." >&2
    echo "Return code: ${RC}"
    exit 1;
fi


cpack -G TGZ -P daswb-depends-vtk -R 9.0.1 2>&1 >> "${BUILD_LOG}"
RC=$?
if [[ ${RC} != 0 ]]; then
    echo "ERROR: Failed to create package (cpack invocation)." >&2
    echo "Return code: ${RC}"
    exit 1;
fi

# tar \
#   --create \
#   --file=vtk-binaries.tar.bz2 \
#   --use-compress-program=pbzip2 \
#   --directory="${WORKING_DIR}" \
#   "${BUILD_DIR_NAME}" \
#   "${INSTALL_DIR_NAME}" \
#   "${ENV_DESCRIPTION_FILE_NAME}" \
#   "${BUILD_LOG_NAME}"

ARCH_ENDTIME=$(date +%s)
echo "It takes $(($ARCH_ENDTIME - $ARCH_STARTTIME)) seconds to complete the arch..."


ENDTIME=$(date +%s)
echo "It takes $(($ENDTIME - $STARTTIME)) seconds to complete full task..."
