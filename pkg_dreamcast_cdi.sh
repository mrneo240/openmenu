#!/bin/sh
cp $1 ../../../Build_CDI/
cd ../../../Build_CDI/
pwd
./pkg_dreamcast_cdi.sh $1 openMenu.cdi
