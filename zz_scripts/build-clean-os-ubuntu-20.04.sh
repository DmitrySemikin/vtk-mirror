#!/bin/bash

## This script is being run from the repo, so we assume, that
## the repo is already cloned, i.e. the commands below were already
## run.
# sudo apt-get -y install git
# git clone https://github.com/DmitrySemikin/vtk-mirror.git

# For Actions we don't need to install git (and I guess also cmake).
# And cloning is done using action

sudo apt-get -y install \
  cmake \
  git \
  g++ \
  libgl1-mesa-dev \
  qtbase5-dev

if [[ "$?" -ne "0" ]]; then
  echo "ERROR: Failed to install packages" >&2
  exit 1;
fi

# another possible variant: libqt5gui5 - but it seems to provide only runtime


SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
ROOT_DIR="${SCRIPT_DIR}/.."
BUILD_DIR="${ROOT_DIR}/../vtk-mirror-build"

mkdir "${BUILD_DIR}" \
&& cd "${BUILD_DIR}" \
&& cmake -DVTK_GROUP_ENABLE_Qt=YES "${ROOT_DIR}" \
&& cmake --build .
