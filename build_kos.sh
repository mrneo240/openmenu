#!/usr/bin/env bash

sudo apt-get -y install binutils bzip2 curl gcc pkg-config libpng-dev libjpeg-dev diffutils git make subversion patch python tar texinfo wget

# Create install directory
INSTALL_DIR="/opt/toolchains/dc"
KOS="$INSTALL_DIR/kos"
PORTS="$INSTALL_DIR/kos-ports"
echo "Creating directory $INSTALL_DIR as root, giving users access to it."

sudo mkdir -p    "$INSTALL_DIR"
sudo chmod o+rwx "$INSTALL_DIR"


# Download the code for KOS + KOS-ports
echo "Downloading KOS and KOS ports."
[ -d "$KOS"   ] || git clone              git://git.code.sf.net/p/cadcdev/kallistios "$KOS"
[ -d "$PORTS" ] || git clone  --recursive git://git.code.sf.net/p/cadcdev/kos-ports  "$PORTS"


# Build the compiler
# You need to change the Makefile if you want to install somewhere else.

echo "Building the compiler."
cd   "$KOS/utils/dc-chain"

echo "Downloading.."
# Getting configuration from Makefile
source ./scripts/common.sh

print_banner "Downloader"

while [ "$1" != "" ]; do
    PARAM=`echo $1 | awk -F= '{print $1}'`
    case $PARAM in
        --config-guess-only)
            CONFIG_GUESS_ONLY=1
            ;;
        *)
            echo "error: unknown parameter \"$PARAM\""
            exit 1
            ;;
    esac
    shift
done

function download()
{
  local name=$1
  local ver=$2
  local url=$3
  local filename=$(basename $url)

  if [ -n "$ver" ]; then
    if [ ! -f $filename ]; then
      echo "Downloading ${name} ${ver}..."
      ${WEB_DOWNLOADER} "${DOWNLOAD_PROTOCOL}${url}" || exit 1
    else
      echo "$name $ver was already downloaded"
	fi
  fi
}

function download_dependencies()
{
  local arch=$1

  local gmp_ver=$SH_GMP_VER
  local mpfr_ver=$SH_MPFR_VER
  local mpc_ver=$SH_MPC_VER
  local isl_ver=$SH_ISL_VER
  local gmp_url=$SH_GMP_URL
  local mpfr_url=$SH_MPFR_URL
  local mpc_url=$SH_MPC_URL
  local isl_url=$SH_ISL_URL

  if [ "$arch" == "arm" ]; then
    gmp_ver=$ARM_GMP_VER
    mpfr_ver=$ARM_MPFR_VER
    mpc_ver=$ARM_MPC_VER
    isl_ver=$ARM_ISL_VER
    gmp_url=$ARM_GMP_URL
    mpfr_url=$ARM_MPFR_URL
    mpc_url=$ARM_MPC_URL
    isl_url=$ARM_ISL_URL
  fi

  if [ "$USE_CUSTOM_DEPENDENCIES" == "1" ]; then
    download "GMP"  "$gmp_ver"   "$gmp_url"
    download "MPFR" "$mpfr_ver"  "$mpfr_url"
    download "MPC"  "$mpc_ver"   "$mpc_url"
    download "ISL"  "$isl_ver"   "$isl_url"
  fi
}

# Download everything.
if [ -z "${CONFIG_GUESS_ONLY}" ]; then
  # Downloading SH components
  download "Binutils" "$SH_BINUTILS_VER" "$SH_BINUTILS_URL"
  download "GCC" "$SH_GCC_VER" "$SH_GCC_URL"
  download_dependencies "sh"
  download "Newlib" "$NEWLIB_VER" "$NEWLIB_URL"

  # Downloading ARM components
  download "Binutils" "$ARM_BINUTILS_VER" "$ARM_BINUTILS_URL"
  download "GCC" "$ARM_GCC_VER" "$ARM_GCC_URL"
  download_dependencies "arm"
fi

# Downloading config.guess.
if [ ! -f ${CONFIG_GUESS} ]; then
  WEB_DOWNLOAD_OUTPUT_SWITCH="-O"
  if [ ! -z "${IS_CURL}" ] && [ "${IS_CURL}" != "0" ]; then
    WEB_DOWNLOADER="$(echo ${WEB_DOWNLOADER} | cut -c-9)"
    WEB_DOWNLOAD_OUTPUT_SWITCH="-o"
  fi

  echo "Downloading ${CONFIG_GUESS}..."
  ${WEB_DOWNLOADER} ${WEB_DOWNLOAD_OUTPUT_SWITCH} ${CONFIG_GUESS} "http://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=${CONFIG_GUESS};hb=HEAD" || exit 1

  # This is needed for all systems except MinGW.
  chmod +x "./${CONFIG_GUESS}"
fi

echo "Done!"


echo "Unpacking.."

# Getting configuration from Makefile
source ./scripts/common.sh

print_banner "Unpacker"

function unpack()
{
  local name="$1"
  local ver="$2"
  local ext="$3"

  local dirname=$(tolower $name)-$ver
  local filename=$dirnam

# Update config.guess and config.sub to recognize msys
[ -s config.guess ] || wget 'http://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.guess;hb=HEAD' -O config.guess
[ -s config.sub ]   || wget 'http://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.sub;hb=HEAD'   -O config.sub
cp config.guess config.sub binutils-*/
cp config.guess config.sub gcc-*/
cp config.guess config.sub newlib-*/
cp config.guess config.sub gcc-*/gmp
cp config.guess config.sub gcc-*/mpc
cp config.guess config.sub gcc-*/mpfr

sed -e "s/,objc,obj-c++//" -i.bak Makefile

echo "Patching..";  make patch
echo "Compiling.."; make build

# Create environment
# You need to change environ.sh and environ_base.sh if you want to install somewhere else.
[ -f "$KOS/environ.sh" ] || cp "$KOS/doc/environ.sh.sample" "$KOS/environ.sh"
. "$KOS/environ.sh"

# Build KOS
echo "Compiling KOS."
cd   "$KOS"
make -j $CORE_COUNT

# Build KOS ports
echo "Compiling KOS ports."
sh "$PORTS/utils/build-all.sh"

# Examples
echo ""
echo "Welcome to KOS!"
echo "Please check out the example in \`$KOS/examples/dreamcast/kgl/nehe/nehe02' to get started."
echo "After editing main.c, type \`make' to create an ELF executable. Run it using an emulator."
echo "KOS documentation is at http://gamedev.allusion.net/docs/kos-current."
echo "When you start a new shell, please type \`source $KOS/environ.sh' in order to set the environment variables."