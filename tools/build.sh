#!/usr/bin/env bash
# tools/build.sh
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

function cleanup()
{
  # keep the mapping but change to the link since:
  # 1.root can't access mount point created by normal user
  # 2.debugger could find the source code without manual setting
  fusermount -u ${MOUNTDIR}
  rmdir ${MOUNTDIR}
  ln -s ${ROOTDIR} ${MOUNTDIR}
}

function mount_unionfs()
{
  echo -e "Mount command line:"
  echo -e "  unionfs-fuse -o cow ${OUTDIR}=RW:${ROOTDIR}=RO ${MOUNTDIR}"

  rm -f ${MOUNTDIR}
  mkdir -p ${MOUNTDIR}
  unionfs-fuse -o cow ${OUTDIR}=RW:${ROOTDIR}=RO ${MOUNTDIR}
}

function setup_environment()
{
  PACKAGES=( \
      "autoconf" \
      "automake" \
      "bison" \
      "build-essential" \
      "dfu-util" \
      "genromfs" \
      "flex" \
      "git" \
      "gperf" \
      "libncurses5" \
      "lib32ncurses5-dev" \
      "libc6-dev-i386" \
      "libx11-dev" \
      "libx11-dev:i386" \
      "libxext-dev" \
      "libxext-dev:i386" \
      "net-tools" \
      "pkgconf" \
      "unionfs-fuse" \
      "zlib1g-dev" \
      "kconfig-frontends" \
      "g++-11" \
      "g++-11-multilib" \
      "libpulse-dev:i386" \
      "libasound2-dev:i386" \
      "libasound2-plugins:i386" \
      "libusb-1.0-0-dev" \
      "libusb-1.0-0-dev:i386" \
      "libmad0-dev:i386" \
      "libv4l-dev" \
      "libv4l-dev:i386" \
      "libuv1-dev" \
      "libmp3lame-dev:i386" \
      "libmad0-dev:i386" \
      "libv4l-dev:i386" \
      "npm" \
      "nodejs" \
      "xxd" \
      "qemu-system-arm" \
      "qemu-efi-aarch64" \
      "qemu-utils" \
      "nasm" \
      "yasm" \
      "libdivsufsort-dev" \
      "libc++-dev" \
      "libc++abi-dev" \
      )

  for (( i = 0; i < ${#PACKAGES[*]}; i++)); do
    dpkg -l ${PACKAGES[$i]} > /dev/null 2>&1
    if [ $? -eq 1 ]; then
      echo "WARNING: no packages found matching ${PACKAGES[$i]}"
      INSTALLS[${#INSTALLS[@]}]=${PACKAGES[$i]}
    fi
  done

  if [ ${#INSTALLS[*]} -eq 0 ]; then
    return
  fi

  if [ ${#INSTALLS[*]} -eq 1 ] && [ "${INSTALLS[0]}" == "kconfig-frontends" ]; then
    return
  fi

  echo "*************************************************************************************"
  echo "The environment of Vela depends on above tools, Run the following command to install:"
  echo ""
  echo " sudo dpkg --add-architecture i386"

  for (( i = 0; i < ${#INSTALLS[*]}; i++)); do
    result=`apt-cache search ${INSTALLS[$i]}`
    if [ "$result" == "" ]; then
      if [ "${INSTALLS[$i]}" == "g++-11" ]; then
        echo " sudo apt-get install -y software-properties-common"
        echo " sudo add-apt-repository ppa:ubuntu-toolchain-r/test"
        echo " sudo apt-get update"
      fi
      if [ "${INSTALLS[$i]}" == "kconfig-frontends" ]; then
        unset INSTALLS[$i]
      fi
    fi
  done

  echo " sudo apt-get install -y ${INSTALLS[@]}"
  echo ""
  echo "*************************************************************************************"

  exit 1
}

function setup_toolchain()
{
  if [ "`uname`" == "Darwin" ]; then
    export MACOSX_DEPLOYMENT_TARGET=11
    echo -e "Note: macOS users need to manually deploy toolchains. Skipping prebuilts setup."
    return
  fi

  ARCH=(\
      "xtensa" \
      "arm" \
      "arm64" \
      "risc-v" \
      "tc32" )

  TOOLCHAIN=(\
            "gcc" \
            "clang" )

  if [ "$XTENSAD_LICENSE_FILE" == "" ]; then
    export XTENSAD_LICENSE_FILE=28000@10.221.64.91
  fi
  export WASI_SDK_PATH=${ROOTDIR}/prebuilts/clang/linux/wasm
  export PATH=${WASI_SDK_PATH}:$PATH
  export PYTHONPATH=${PYTHONPATH}:${ROOTDIR}/prebuilts/tools/python/dist-packages/pyelftools
  export PYTHONPATH=${PYTHONPATH}:${ROOTDIR}/prebuilts/tools/python/dist-packages/cxxfilt

  for (( i = 0; i < ${#ARCH[*]}; i++)); do
    for (( j = 0; j < ${#TOOLCHAIN[*]}; j++)); do
      export PATH=${ROOTDIR}/prebuilts/${TOOLCHAIN[$j]}/linux/${ARCH[$i]}/bin:$PATH
    done
  done

  # Arm Compiler
  export PATH=${ROOTDIR}/prebuilts/clang/linux/armclang/bin:$PATH

  if [ ! -n "${ARM_PRODUCT_DEF}" ]; then
    export ARM_PRODUCT_DEF=${ROOTDIR}/prebuilts/clang/linux/armclang/mappings/eval.elmap
  fi
  if [ ! -n "${LM_LICENSE_FILE}" ]; then
    export LM_LICENSE_FILE=${HOME}/.arm/ds/licenses/DS000-EV-31030.lic
  fi
  if [ ! -n "${ARMLMD_LICENSE_FILE}" ]; then
    export ARMLMD_LICENSE_FILE=${HOME}/.arm/ds/licenses/DS000-EV-31030.lic
  fi

  # Generate compile database file compile_commands.json
  if type bear >/dev/null 2>&1; then
    # get version of bear
    BEAR_VERSION=$(bear --version | awk '{print $2}' | awk -F. '{printf("%d%03d%03d ", $1,$2,$3)}')

    # judge version of bear
    if [ $BEAR_VERSION -ge 3000000 ]; then
        # BEAR="bear --append --output compile_commands.json -- "
        echo -e "Note: currently not support bear 3.0.0+ for some prebuilt toolchain limited."
    else
        echo -e "Note: bear 2.4.3 in Ubuntu 20.04 works out of box."
        BEAR="bear -a -o compile_commands.json "
    fi
  fi

  # Add compile cache
  CCACHE_DIR=${ROOTDIR}/prebuilts/tools/ccache
  export PATH="${CCACHE_DIR}:$PATH"

  # AIDL Tool
  export PATH=${ROOTDIR}/prebuilts/tools/aidl:$PATH
  # HIDL Tool
  export PATH=${ROOTDIR}/prebuilts/tools/hidl:$PATH
}

function build_board()
{
  echo -e "Build command line:"
  echo -e "  ${TOOLSDIR}/configure.sh -e $1"
  echo -e "  make -C ${NUTTXDIR} EXTRAFLAGS="$EXTRA_FLAGS" ${@:2}"
  echo -e "  make -C ${NUTTXDIR} savedefconfig"

  if [ ! -f "${ROOTDIR}/prebuilts/kconfig-frontends/bin/kconfig-conf" ] &&
     [ ! -x "$(command -v kconfig-conf)" ]; then
    pushd ${ROOTDIR}/prebuilts/kconfig-frontends
    ./configure --prefix=${ROOTDIR}/prebuilts/kconfig-frontends 1>/dev/null
    touch aclocal.m4 Makefile.in
    make install 1>/dev/null
    popd
  fi
  export PATH=${ROOTDIR}/prebuilts/kconfig-frontends/bin:$PATH

  setup_toolchain

  if ! ${TOOLSDIR}/configure.sh -e $1; then
    echo "Error: ############# config ${1} fail ##############"
    exit 1
  fi

  if ! ${BEAR} make -C ${NUTTXDIR} EXTRAFLAGS="$EXTRA_FLAGS" ${@:2}; then
    echo "Error: ############# build ${1} fail ##############"
    exit 2
  fi

  if [ "${2}" == "distclean" ]; then
    return;
  fi

   if ! make -C ${NUTTXDIR} savedefconfig; then
    echo "Error: ############# save ${1} fail ##############"
    exit 3
  fi
  if [ ! -d $1 ]; then
    cp ${NUTTXDIR}/defconfig ${ROOTDIR}/nuttx/boards/*/*/${1/[:|\/]//configs/}
  else
    cp ${NUTTXDIR}/defconfig $1
  fi
}

function pack_sdk()
{
  VENDOR=(\
      "realtek" \
      "bes" )
  BOARDS=(\
      "
      amebaZ
      " \
      "
      best2001_ep
      " \
      )

  if [ "${2}" != "export" ]; then
    exit 0
  fi

  if [ ! -f $OUTDIR/nuttx/.version ]; then
    exit 1
  fi

  source $OUTDIR/nuttx/.version

  local exportzip=$OUTDIR/nuttx/nuttx-export-$CONFIG_VERSION_STRING.zip

  echo $exportzip
  if [ ! -f $exportzip ]; then
    exit 1
  fi

  local version=$CONFIG_VERSION_STRING.$CONFIG_VERSION_BUILD
  local board=`echo $1 | awk -F '/' '{print $4}'`
  local packname=`echo nuttx-sdk-$board-$version.zip`
  local vendor=

  for (( i = 0; i < ${#BOARDS[*]}; i++)); do
    if [ $board == ${BOARDS[$i]} ]; then
      vendor=vendor/${VENDOR[$i]}
      break
    fi
  done

  cp $exportzip .
  zip -r $packname sdk $vendor `basename $exportzip` -x "sdk/.git*" 1>/dev/null 2>&1

  if [ $? -eq 0 ]; then
    echo "############## Export SDK Success: $packname ##############"
  fi

  rm `basename $exportzip`
}

if [ $# == 0 ]; then
  echo "Usage: $0 [-m] <board-name>:<config-name> [-e <extraflags>] [make options]"
  echo ""
  echo "Where:"
  echo "  -m: out of tree build. Or default in tree build without it."
  echo "  -e: pass extra c/c++ flags such as -Werror via make command line"
  exit 1
fi

ROOTDIR=$(dirname $(readlink -f ${0}))
ROOTDIR=$(realpath ${ROOTDIR}/../..)

setup_environment

CONFIGPATH=$2

if [ $1 == "-m" ]; then
  # out of tree build
  confparams=(${CONFIGPATH//:/ })
  configdir=${confparams[1]}

  if [ -z "${configdir}" ]; then
    # handle cases where the end is a "/"
    if [ "${CONFIGPATH:(-1)}" = "/" ]; then
      CONFIGPATH=${CONFIGPATH:0:-1}
    fi
    boarddir=`echo ${CONFIGPATH} | rev | cut -d'/' -f3 | rev`
    configdir=`echo ${CONFIGPATH} | rev | cut -d'/' -f1 | rev`
  else
    boarddir=${confparams[0]}
  fi

  OUTDIR=${ROOTDIR}/out/${boarddir}/${configdir}
  MOUNTDIR=${OUTDIR}/.unionfs
  NUTTXDIR=${MOUNTDIR}/nuttx

  trap cleanup EXIT
  mount_unionfs
  shift
else
  # in tree build
  OUTDIR=${ROOTDIR}
  NUTTXDIR=${ROOTDIR}/nuttx
fi

TOOLSDIR=${NUTTXDIR}/tools
board_config=$1
shift

EXTRA_FLAGS="-Wno-cpp"
if [ "$1" == "-e" ]; then
  shift
  EXTRA_FLAGS+=" $1"
  echo "extraflags: $EXTRA_FLAGS"
  shift
fi

if [ -d ${ROOTDIR}/${board_config} ]; then
  build_board ${ROOTDIR}/${board_config} $*
else
  build_board ${board_config} $*
fi

pack_sdk ${board_config} $*
