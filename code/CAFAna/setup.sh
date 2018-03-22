#!/bin/bash

#if [ "$0" == "$BASH_SOURCE" ]
#then
#    echo 'Please source this script (it needs to modify your environment)'
#    exit 1
#fi

# Doesn't have the exact versions I require below
#export CVMFS_DISTRO_BASE=/cvmfs/larsoft.opensciencegrid.org/
#export EXTERNALS=$CVMFS_DISTRO_BASE/products

export CVMFS_DISTRO_BASE=/cvmfs/nova.opensciencegrid.org/
export EXTERNALS=$CVMFS_DISTRO_BASE/externals
export PRODUCTS=$EXTERNALS

export UPS_SHELL=/bin/bash

export PATH=$EXTERNALS/ups/v6_0_7/Linux64bit+2.6-2.12/bin/:$PATH

setup()
{
    source `ups setup "$@"`
}

setup ups v6_0_7
setup root v6_10_08b -q e14:nu:prof
setup ifdhc v2_3_2 -q e14:p2714:prof
setup clhep v2_3_4_5a -q e14:prof
setup boost v1_65_1 -q e14:prof
setup python v2_7_14

export SRT_PUBLIC_CONTEXT=~/git/lblpwgtools.buildsystem/code/CAFAna/
export SRT_PRIVATE_CONTEXT=~/git/lblpwgtools.buildsystem/code/CAFAna/
#export SRT_SUBDIR=Linux2.6-GCC-maxopt
export SRT_MAKEFLAGS='-r -I/home/bckhouse/git/lblpwgtools.buildsystem/code/CAFAna/'

export LD_LIBRARY_PATH=$SRT_PRIVATE_CONTEXT/lib

#cd releases/development/CAFAna/

# export SRT_SUBDIR=Linux2.6-GCC-maxopt
# export SRT_QUAL=maxopt
# export MAKEFLAGS='-r -I./include/SRT_NOVA -I/home/bckhouse/git/lblpwgtools/code/CAFAna/releases/development/include/SRT_NOVA -I./include -I/home/bckhouse/git/lblpwgtools/code/CAFAna/releases/development/include'
# export SRT_ARCH=Linux2.6
# export SRT_ENV_SET=yes
# export SRT_DIST=/home/bckhouse/git/lblpwgtools/code/CAFAna
# export DEFAULT_SRT_DIST=/home/bckhouse/git/lblpwgtools/code/CAFAna
# export SRT_PUBLIC_CONTEXT=/home/bckhouse/git/lblpwgtools/code/CAFAna/releases/development
# export SRT_MAKEFILES=SoftRelTools/preamble.mk
# export SRT_PRIVATE_CONTEXT=.
# export SRT_PROJECT=NOVA
# export SRT_MAKEFLAGS='-r -I./include/SRT_NOVA -I/home/bckhouse/git/lblpwgtools/code/CAFAna/releases/development/include/SRT_NOVA -I./include -I/home/bckhouse/git/lblpwgtools/code/CAFAna/releases/development/include'
# export SRT_CXX=GCC
# export SRT_BASE_RELEASE=development
