#!/bin/sh 

###################################################################################
### Setup script for NOvASoft.                                                  ###
### It parses the options that the script is invoked with using getopt.         ### 
### It checks which OS the script is being sourced on. If the release is new    ###
### ie, made after 2014-07-31, we maintain both SLF5 and SLF6 builds and the    ###
### appropriate build is sourced.                                               ###
### The script also logs the release being sourced and the user sourcing it     ###
### using datagram.                                                             ###
### We also nuke the /tmp and /var/tmp directories of $USER-owned files that    ###
### are older than 7 days.                                                      ###
### If enabled in the user's environment (NOVA_BASH_COMPLETION=true), set up    ###
### custom bash completion for nova-art.                                        ###
###################################################################################



###################################################################################
# Prints a help menu when the script is invoked with -h option or invalid option
###################################################################################

usage () {
    echo "" >&2
    echo "usage: setup_nova.sh [options]" >&2
    echo "options:" >&2
    echo "     -h:            prints this usage message                    " >&2
    echo "     -c:            Use c-shell                                  " >&2
    echo "     -s:            Use sh-shell (standard)                      " >&2
    echo "     -r:            specifies the release to be set up           " >&2
    echo "     -b:            specifies the build (debug/prof/maxopt)      " >&2
    echo "     -e:            user-defined PRODUCTS path                  " >&2
    echo "     -5:            user-defined \$SRT_DIST path for slf5 build   " >&2
    echo "     -6:            user-defined \$SRT_DIST path for slf6 build   " >&2
    echo "     -n:            user-defined setup_novasoft_setup equivalent " >&2
    echo "                                                                 " >&2
    echo "You either requested the help or setup has failed. Try again!    " >&2
    echo "                                                                 " >&2
    return 1 
}


###################################################################################
# Sets defaults in case these arguments are not provided 
###################################################################################

set_defaults () {
    shell_type="-s"
    release=development
    build=debug
    os=slf6
    expath=/cvmfs/nova.opensciencegrid.org/externals:/cvmfs/fermilab.opensciencegrid.org/products/common/db
    s5path=/cvmfs/nova.opensciencegrid.org/novasoft/slf5/novasoft
    s6path=/cvmfs/nova.opensciencegrid.org/novasoft/slf6/novasoft
    setup_novasoft_setup_script=setup_novasoft_setup.sh
    unset INVALID_RELEASE
}


###################################################################################
# Processes OS running on the machine that the script is sourced on. The only two
# acceptable options are SLF5 and SLF6 currently.
###################################################################################

process_os () {
    redhat=`cat /etc/redhat-release`
    if [[ "$redhat" =~ "release 5." ]]; then
	os=slf5
    elif [[ "$redhat" =~ "release 6." ]]; then
	os=slf6
    else
	echo "Unsupported platform. NOvASoft is only available for SLF5 and SLF6 distributions."
	return 1
    fi
    return 0
}

###################################################################################
# For geant4 v4_10_1_p03 we need to set a new environment variable.
# Specifically: G4NEUTRONHP_USE_ONLY_PHOTONEVAPORATION=1
###################################################################################
export_g4_var () {
    # g4_version=v4_10_1_p03
    # if [ -n "${GEANT4_VERSION}" ]; then
    #     if [ "${GEANT4_VERSION}" == "${g4_version}" ]; then
    #         export G4NEUTRONHP_USE_ONLY_PHOTONEVAPORATION=1
    #     fi
    # else
    #     echo -e "\e[01;31mError: GEANT4_VERSION env. variable not set. Something is wrong here.\e[0m"
    #     echo -e "\e[01;31mSetup aborting.\e[0m"
    #     return 1
    # fi
    return 0
}

###################################################################################
# Use getopt to process the arguments that the script is invoked with
###################################################################################

process_args () {
    errorcode=0
    s5path_set=0
    s6path_set=0
    while getopts "hcse:r:b:5:6:n:-:" opt; do
        if [ "$opt" = "-" ]; then
            opt=$OPTARG
        fi
	gettingopt=1
        case $opt in
            h | help)
                usage
		errorcode=1
                ;;
            s | sh)
                shell_type="-s"
                ;;
            c | csh)
                shell_type="-c"
                ;;
            r | release)
                release=$OPTARG
                ;;
            b | build)
                build=$OPTARG
                ;;
	    e ) 
		expath=$OPTARG
		;;
	    5 )
		s5path=$OPTARG
		s5path_set=1
		;;
	    6 )
		s6path=$OPTARG
		s6path_set=1
		;;
	    n )
		setup_novasoft_setup_script=$OPTARG
		;;
	    ?) 
                usage
		errorcode=1
                ;;
        esac
    done
    if [ "$#" != "0" ] && [ "$gettingopt" != "1" ] && [ "$errorcode" != "1" ]; then
    	echo -e "\e[01;31mERROR! Invalid argument/option. Try again!\e[0m" >&2
 	usage
    	errorcode=1
    fi

    if [[ ${release} == "development" ]] || [[ ${release} == N* ]]; then
        if [ $s5path_set == 0 ]; then
	    s5path=/cvmfs/nova-development.opensciencegrid.org/novasoft
        fi
        if [ $s6path_set == 0 ]; then
            s6path=/cvmfs/nova-development.opensciencegrid.org/novasoft
        fi
    fi

    return $errorcode
}


# Simple function for checking if one version number is
# greather than another.
# Usage: version_gt first_version second_version

version_gt () {
    test "$(echo "$@" | tr " " "\n" | sort -V | head -n 1)" != "$1";
}


###################################################################################
# Once the release and build to source are determined, source the setup script and
# setup the srt environment
###################################################################################

source_setup_script () {
    setup_location=$SRT_DIST/setup

    #if [ ${release} == "development" ]; then
    #     release="$(basename `ls -td -- /cvmfs/nova-development.opensciencegrid.org/novasoft/releases/* | head -n 1`)"
    #fi

    if [ ! -d ${SRT_DIST}/releases/${release} ]; then
    	echo -e "\e[01;31mINVALID RELEASE. Try sourcing again with a valid release name.\e[0m" >&2
	return 1 
    fi
    if [ ${build} != "debug" ] && [ ${build} != "maxopt" ]; then
    	echo -e "\e[01;31mINVALID BUILD. Valid options are 'debug' and 'maxopt'.\e[0m" >&2
	echo "Try sourcing again with a valid build" >&2
	return 1 
    fi

     setup_script=${SRT_DIST}/releases/${release}/setup/${setup_novasoft_setup_script}
     #setup_script=${setup_novasoft_setup_script}

    if [ ! -x ${setup_script} ];then
    	echo -e "\e[01;31mERROR: setup_script ${setup_script} does not exist or is not executable.\e[0m" >&2
	return 1 
    fi

    result=`${setup_script} ${shell_type} -r ${release} -b ${build}`

    if [ -r "$result" ]; then
	source ${result}
    fi
    
    if [ -x ${SRT_PUBLIC_CONTEXT}/bin/${SRT_SUBDIR}/srt_environment_nova ]; then
        srt_setup () {
            source `srt_environment_nova -X "$@"`
        }
    fi   
    return 0
}


###################################################################################
# Summarize what we have setup so the user doesn't miss it
###################################################################################

echo_setup_vars () {
    # Don't bother echoing if you are not being sourced in a terminal.
    # No one needs to read this in their logs.
    if [ -t 1 ]; then
	if [ -z "${_CONDOR_SCRATCH_DIR}" ]; then
            local build_date_file=${SRT_PUBLIC_CONTEXT}/build_date
            if [ -f $build_date_file ];then
                echo "" >&2
                echo -e "Release:    \e[01;34m$SRT_BASE_RELEASE\e[0m" >&2
                echo -e "Build:      \e[01;34m$SRT_QUAL\e[0m" >&2
                echo -e "Build Date: \e[01;34m`cat $build_date_file`\e[0m \n" >&2
            else
                echo "" >&2
                echo -e "Release: \e[01;34m$SRT_BASE_RELEASE\e[0m" >&2
                echo -e "Build:   \e[01;34m$SRT_QUAL\e[0m \n" >&2
            fi
            
	fi
    fi 
}



###################################################################################
# Run datagram. Courtesy Robert Hatcher. Scripts that log which release is being
# sourced, by whom and how often. The server side is run on novabuild01.
###################################################################################

run_datagram () {
    $SRT_DIST/setup/datagram/datagram_client.py "${BFCURRENT} ${build}"   
}


###################################################################################
# Check which release we are sourcing. If it is newer than July 31, we maintain an
# SLF6 build for it. In case the OS is SLF6, we will source the SLF6 build.
###################################################################################

process_release () {
    if [ ${release} == "development" ]; then
    	time="renaissance"
    elif [ ${release} == "first-ana" ]; then
     	time="renaissance"
    else
    	# Check the date of the release 

        # The release name format has to be ?YY-MM-DD*, where ? cannot include numbers, and * can include any character

        #This sed command removes any occurances of lower case or upper case characters (leaves the numbers and -)
        tempdate=`echo $release | sed 's/[a-zA-Z.]//g'`
        #We also need to protect against people putting numbers after the date in the release, for example R16-01-27-prod2calib.a (the "2")
        tempdate=${tempdate:0:8}
	numdate=${tempdate//[^0-9]/}

	# Relying on the assumption that if there are exactly six numbers
	# in the release name, it's got to be a date. Fingers crossed!
	if [ ${#numdate} -ne 6 ]; then
	    time="classical"
	    return
	fi

    	reldate=$(date -d $tempdate +"%y%m%d") || return 1
    	sl6compdate=$(date -d 14-07-31 +"%y%m%d") 
	gridcompdate=$(date -d 14-03-31 +"%y%m%d")
	
	if [ $reldate -ge $sl6compdate ]; then
	    time="renaissance"
	elif [ $reldate -ge $gridcompdate ]; then
	    time="medieval"
	else 
	    time="classical"
	fi
    fi
}

nuke_tmp () {
    
    # Check hostname is a novagpvm ending in digits, i.e. novagpvm10
    # Extends if we add more gpvms
    if [[ $(hostname -s) =~ ^novagpvm.*[0-9]+$ ]]; then
	# Find /tmp files from user older than 30 minutes.
	# Delete the x509up* files older than 7 days.
	# Suppress permission denied entries to /dev/null
	find /tmp -user $USER ! -name "x509up*" ! -name "krb5cc*" -mmin +30 -delete &> /dev/null
	find /tmp -user $USER -name "x509up*" -mtime +7 -delete &> /dev/null
	# In /var/tmp, ifdh puts temp files in directories
	# as does dk2nu running. Delete these after 7 days too
	find /var/tmp -user $USER -mtime +7 -exec rm -rf {} \; &> /dev/null
    fi
}

###################################################################################
# Test for presence of "Linux3.1" in path/library list.
# This is a symptom of an older release that hasn't had the necessary SRT patch.
###################################################################################

test_linux() {
        if echo $PATH $LD_LIBRARY_PATH | grep Linux3.1 > /dev/null 2>&1; then
        	      echo
        	      echo
                echo "WARNING: Incorrect kernel version (Linux3.1) detected in \$PATH or \$LD_LIBRARY_PATH."
                echo "         Probably any ART jobs you try to run will fail."
                echo
                echo "         This release is too old to have the necessary patches to SRT."
                echo "         Solutions are discussed on Slack:"
                echo "         https://neutrino.slack.com/archives/C02FS4L15/p1523389105000118?thread_ts=1515181055.000361" 
        fi
}

###################################################################################
# Enable nova bash completion in environments where NOVA_BASH_COMPLETION=true
###################################################################################

setup_nova_bashcompletion () {
    if [ "$NOVA_BASH_COMPLETION" = true ] && [ -e $SRT_PUBLIC_CONTEXT/setup/nova_bashcompletion ]; then
        source $SRT_PUBLIC_CONTEXT/setup/nova_bashcompletion
    fi
}

setup_ccache () {
    # The ccache product should already be setup, this is just configuring it

    # These first settings are generically what you'd want at any site

    # Evict old cache entries once it reaches this size
    export CCACHE_MAXSIZE=50G
    # Make sure everyone has permissions to files placed in the cache
    export CCACHE_UMASK=002

    # Necessary to get cache hits, at least in debug build
    export CCACHE_SLOPPINESS=file_macro,time_macros

    export CCACHE_TEMPDIR=/tmp

    # These settings are only appropriate for the GPVMs. Other machines will
    # need different paths.
    if [[ `hostname -s` == novagpvm?? ]]
        then

        # This is the line that actually activates ccache. Make sure the ccache
        # links are ahead of the actual compilers in the path.
        export PATH=$CCACHE_DIR/bin:$PATH

        # The ccache ups product is confused. This environment variable needs
        # to point to the location of the cache to share with other
        # users. Update it after we grab its value for the $PATH
        export CCACHE_DIR=/nova/app/home/novasoft/ccache_cache/

        # Rewrite paths below this location to relative paths. This helps the
        # sharing of caches in different directories. But we don't want to
        # specify a directory much higher up, because $SRT_PUBLIC_CONTEXT is on
        # /nova/app/ too, and making all links to that relative would mean only
        # builds at the same depth in the directory tree would share cache
        # entries.
        export CCACHE_BASEDIR=/nova/app/users/`whoami`
    fi
}


###################################################################################
# Cleanup the variables used in this script. We don't want to litter the environment
# It is also necessary for getopts to not remember it's previous state to react 
# properly if the script is invoked multiple times.
###################################################################################

cleanup_vars () {
    
    unset opt
    unset gettingopt
    unset OPTARG
    unset OPTIND
    unset redhat
    unset result
    unset setup_location
    unset release
    unset build
    unset AUSER
    unset kernel
    unset sl
    unset time
    unset tempdate
    unset reldate
    unset compdate
    unset errorcode
    unset retval_os
    unset retval_rel
    unset retval_args
    unset retval_g4
    unset expath
    unset s5path
    unset s6path
    unset g4_version
}

###################################################################################
# Main function. Calls other functions in the script and keeps a check on whether
# or not novasoft has been setup in the current shell before.
###################################################################################

main () {

    if [[ ${NOVASOFT_SETUP} = 1 ]]; then
    	echo
	echo -e "\e[00;31m***********************WARNING!***********************\e[0m"
    	echo "Currently we prevent multiple sourcing, i.e. switching" 
    	echo "between releases mid-session!"
    	echo "You have NOT successfully sourced a different release."
    	echo "If you want to source a different release you must either" 
    	echo "log out and login to source again, or open a new terminal."
    	echo "This is to prevent indefinite extension of PATH and "
    	echo "LD_LIBRARY_PATH"
	echo -e "\e[00;31m********************END OF WARNING!*******************\e[0m"
    	echo
    	return 1
    fi

    set_defaults
    
    process_os
    retval_os=$?
    if [ "$retval_os" == 1 ]; then
	cleanup_vars
	return 1
    fi
    
    process_args $*


    retval_args=$?
    if [ "$retval_args" == 1 ]; then
	cleanup_vars
	return 1
    fi
    

    # Set PRODUCTS env variable. Needed to setup up all of the ups products
    # First check that the provided path is valid by looking for setup file
    # in it.

    IFS=':' read -ra tmppath <<< "${expath}"
    for ipath in "${tmppath[@]}"; do
        if [ -e "${ipath}/setup" ]; then
            upssetup="${ipath}/setup"
            break
        fi
    done
    
    if [ -z ${upssetup} ]; then
	echo "No setup script found in ${expath}"
	echo -e "\e[01;31mInvalid PRODUCTS path. Setup has failed.\e[0m"
	cleanup_vars
	return 1
    fi
    export PRODUCTS=${expath}
    # Need to set this variable to setup tags from before 2014-08-22.
    export EXTERNALS=`dirname ${upssetup}`


    process_release
    retval_rel=$?
    if [ "$retval_rel" == 1 ]; then
	cleanup_vars
	return 1
    fi

    # Choose where to set the SRT_DIST to based on whether we are running
    # on SLF5 or SLF6
    if [ $os == "slf6" ] && [ $time == "renaissance" ] ; then
	if [ -f ${s6path}/srt/srt.sh ]; then
	    source ${s6path}/srt/srt.sh
	else 
	    echo -e "\e[01;31m${s6path}/srt/srt.sh does not exist. Setup has failed.\e[0m"
	    cleanup_vars
	    return 1
	fi
    else
	if [ -f ${s5path}/srt/srt.sh ]; then
	    source ${s5path}/srt/srt.sh
	else 
	    echo -e "\e[01;31m${s5path}/srt/srt.sh does not exist. Setup has failed.\e[0m"
	    cleanup_vars
	    return 1
	fi
    fi
        
    # source setup_novasoft_setup script
    source_setup_script $*
    retval_rel=$?
    if [ "$retval_rel" == 1 ]; then
	cleanup_vars
	return 1
    fi

    # export g4 env. var. G4NEUTRONHP_USE_ONLY_PHOTONEVAPORATION=1
    export_g4_var
    retval_g4=$?
    if [ "$retval_g4" == 1 ]; then
	cleanup_vars
	unset G4NEUTRONHP_USE_ONLY_PHOTONEVAPORATION
	return 1
    fi

    export NOVASOFT_SETUP=1

    # setup_novagrid didn't exist before April 2014, don't try to source
    # it for those times.
    if [ $time == "medieval" ] || [ $time == "renaissance" ] ; then
	if [ ! -d /grid/fermiapp/ ]; then
	    source $NOVAGRIDUTILS_DIR/bin/setup_shim
	fi
    fi

    # Source cetbuidtools.sh to get checkClassVersion in $PATH
    # Last nutools version before checkClassVersion moved to cetbuildtools
    # v2_02_00 - only setup cetbuildtools if nutools version is greater
#    nutools_premove=v2_02_00
#    if version_gt $NUTOOLS_VERSION $nutools_premove ; then
#	source setup_cetbuildtools.sh
#    else
#	echo "nutools version $NUTOOLS_VERSION NOT GREATER THAN $nutools_premove; DO NOTHING"
#	echo "`which checkClassVersion`"
#    fi

    echo_setup_vars

    echo "PWD: $PWD"
    echo
    echo -e "\033[1;34m\033[4mWELCOME TO CAFAna for DUNE\033[0m     "
    echo -e "\e[00;33mROOT:\e[0m $ROOT_VERSION "
    echo -e "\e[00;32mHelp is nearby: #cafana \e[0m"
    echo
    nuke_tmp
    setup_nova_bashcompletion
    setup_ccache
    cleanup_vars
    test_linux

}

main $*
