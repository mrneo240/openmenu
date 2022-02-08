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

cp config.mk.stable.sample config.mk
echo "Downloading.."
./download.sh


echo "Unpacking.."
./unpack.sh

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