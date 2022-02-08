#!/bin/bash

# msys2 installer for Windows: http://repo.msys2.org/distrib/i686/msys2-i686-20160205.exe

# Stop script on error
set -e

is_root_user()   { [ $(id -u) -eq 0 ]; }
program_exists() { command -v "$1" >/dev/null 2>&1; }

is_root_user && { echo "Please don't run this script as root."; exit 1; }

if program_exists getconf; then
	CORE_COUNT="$(getconf _NPROCESSORS_ONLN)"
else
	CORE_COUNT=2
fi

# Force mingw32 shell on MSYS 32 bit, disallow MSYS 64 bit (untested)
HAVE_MINGW32=0
case "$(uname)" in
	"MINGW64"*|"MSYS"*)
		echo "Please use c:\msys32\mingw32_shell.bat"
		exit 1
		;;
	"MINGW32"*)
		echo "MingW32 detected."
		HAVE_MINGW32=1
		;;
esac

if [ $HAVE_MINGW32 -eq 0 ] && ! program_exists sudo; then
	echo "Please install sudo."
	exit 1
fi

# Install requirements on msys2
if [ $HAVE_MINGW32 -eq 1 ]; then
	# Don't upgrade packages here because it requires restarting the shell and loses Windows XP support (bash etc. will miss functions in the Windows XP kernel)
	pacman -Sy --needed --noconfirm \
		mingw-w64-i686-binutils \
		mingw-w64-i686-gcc \
		mingw-w64-i686-pkg-config \
		mingw-w64-i686-libpng \
		mingw-w64-i686-libjpeg-turbo \
		diffutils git make subversion patch python tar texinfo wget
# Install requirements using OS X homebrew
elif program_exists brew; then
	brew install libjpeg libpng
	brew link libjpeg libpng
# Install required programs on debian or derived distributions
elif program_exists apt-get; then
	sudo apt-get -y install binutils bzip2 curl gcc pkg-config libpng-dev libjpeg-dev diffutils git make subversion patch python tar texinfo wget
elif program_exists yum && program_exists sudo; then
	sudo yum -y install binutils bzip2 curl gcc pkgconfig libpng-devel libjpeg-devel diffutils git make subversion patch python tar texinfo wget
# Install required programs on gentoo or derived distributions
elif program_exists emerge && program_exists sudo; then
	sudo emerge -an sys-devel/binutils app-arch/bzip2 net-misc/curl sys-devel/gcc dev-util/pkgconfig media-libs/libpng media-libs/libjpeg-turbo sys-apps/diffutils dev-vcs/git sys-devel/make dev-vcs/subversion sys-devel/patch dev-lang/python app-arch/tar sys-apps/texinfo net-misc/wget
# Catch-all, installs requirements when no package manager is available
else
	echo "Checking for zlib, libjpeg and libpng.."
	[ -d /usr/local/src ] || sudo mkdir -p /usr/local/src

	if ([ ! -f /usr/include/zlib.h ] || [ ! -f /usr/lib/libz.so ]) && ([ ! -f /usr/local/include/zlib.h ] || [ ! -f /usr/local/lib/libz.so ]); then
		[ -d http://zlib.net/zlib-1.2.8.tar.gz ] || curl 'http://zlib.net/zlib-1.2.8.tar.gz' | sudo tar xz -C /usr/local/src
		echo "Building zlib"
		cd /usr/local/src/zlib-1.2.8
		sudo ./configure
		sudo make -j $CORE_COUNT install
	else
		echo "zlib is already installed."
	fi

	if ([ ! -f /usr/include/jpeglib.h ] || [ ! -f /usr/lib/libjpeg.so ]) && ([ ! -f /usr/local/include/jpeglib.h ] || [ ! -f /usr/local/lib/libjpeg.so ]); then
		[ -d /usr/local/src/jpeg9b ] || curl 'http://www.ijg.org/files/jpegsrc.v9b.tar.gz' | sudo tar xz -C /usr/local/src
		echo "Building libjpeg"
		cd /usr/local/src/jpeg-9b
		sudo ./configure
		sudo make -j $CORE_COUNT install
	else
		echo "libjpeg is already installed."
	fi

	if ([ ! -f /usr/include/png.h ] || [ ! -f /usr/lib/libpng.so ]) && ([ ! -f /usr/local/include/png.h ] || [ ! -f /usr/local/lib/libpng.so ]); then
		[ -d /usr/local/src/libpng-1.6.25 ] || curl -L 'http://download.sourceforge.net/libpng/libpng-1.6.25.tar.gz' | sudo tar xz -C /usr/local/src
		echo "Building libpng"
		cd /usr/local/src/libpng-1.6.25
		sudo ./configure
		sudo make -j $CORE_COUNT install
	else
		echo "libpng is already installed."
	fi
fi


# Check for programs
MISSING_PROGRAMS=0
for i in bzip2 curl diff gcc git gzip make makeinfo patch pkg-config python svn tar wget; do
	program_exists "$i" || { echo "Please install $i."; MISSING_PROGRAMS=1; }
done

[ $MISSING_PROGRAMS -eq 1 ] && exit 1

# Create install directory
INSTALL_DIR="/opt/toolchains/dc"
KOS="$INSTALL_DIR/kos"
PORTS="$INSTALL_DIR/kos-ports"
echo "Creating directory $INSTALL_DIR as root, giving users access to it."

if program_exists sudo; then
	sudo mkdir -p    "$INSTALL_DIR"
	sudo chmod o+rwx "$INSTALL_DIR"
else
	mkdir -p "$INSTALL_DIR"
fi

# Download the code for KOS + KOS-ports
echo "Downloading KOS and KOS ports."
[ -d "$KOS"   ] || git clone              git://git.code.sf.net/p/cadcdev/kallistios "$KOS"
[ -d "$PORTS" ] || git clone  --recursive git://git.code.sf.net/p/cadcdev/kos-ports  "$PORTS"

# Build the compiler
# You need to change the Makefile if you want to install somewhere else.
if ! program_exists "$INSTALL_DIR/sh-elf/bin/sh-elf-gcc" || ! program_exists "$INSTALL_DIR/sh-elf/bin/sh-elf-g++"; then
	echo "Building the compiler."
	cd   "$KOS/utils/dc-chain"

	echo "Downloading.."; sh download.sh
	echo "Unpacking..";   sh unpack.sh

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
fi

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
DOCURL='http://gamedev.allusion.net/docs/kos-current/structcont__state__t.html'
if program_exists x-www-browser; then
	x-www-browser "$DOCURL"
elif program_exists firefox; then
	firefox       "$DOCURL"
elif program_exists chromium; then
	chromium      "$DOCURL"
fi

echo ""
echo "Welcome to KOS!"
echo "Please check out the example in \`$KOS/examples/dreamcast/kgl/nehe/nehe02' to get started."
echo "After editing main.c, type \`make' to create an ELF executable. Run it using an emulator."
echo "KOS documentation is at http://gamedev.allusion.net/docs/kos-current."
echo "When you start a new shell, please type \`source $KOS/environ.sh' in order to set the environment variables."