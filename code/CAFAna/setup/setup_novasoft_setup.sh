#!/bin/sh
#
#  NOvA-ART setup script
#  B. Rebel - June 19, 2008
#  G. Davies - April 10, 2011
#  stolen from P. Shanahan script for nova
#  G.Davies - March 15, 2012
#  Major change to mirror how Online sets up UPS products

#  Based on logic of SRT setup scripts
#
#  This script does nothing other than setup another (temporary) script, which
#  will either be in csh or sh, as desired, and then return the full name
#  of the temporary script.
#
#  It is intended that a wrapper will then source the temp script.
#
#  Why is it done this way?  Basically to allow one script to handle
#  csh and sh.  The persistence of variables requires source'ing rather
#  than direct execution, but sourcing is incompatible with forcing a
#  shell.  So, we force the shell, but then write a sourceable file
#  in the user's prefered shell.


#################################################################
########### Following routines are site-specific   ##############
#################################################################
remove_tww () {
    #  remove TheWrittenWord from the path. Evil. Bad.
    print_var PATH "\`dropit -p \$PATH /opt/TWWfsw/bin\`"  $shell_type
}

set_defaults () {
    shell_type=csh
    release=development
    build=debug
    find_output_file_name
}

set_ups () {

    if [ -z "${PRODUCTS}" ]; then
	echo "You need to set the PRODUCTS directory"
    else
	IFS=':' read -ra extpath <<< "${PRODUCTS}"
	for ipath in "${extpath[@]}"; do
	    if [ -e "${ipath}/setup" ]; then
	    	upssource="${ipath}/setup"
		break
	    fi
	done
	if [ -z "${upssource}" ]; then
	    echo "setup not found in the $PRODUCTS path. Setup aborting."
	    exit
	fi
    fi
    if [ -f $upssource ]; then
	insert_source $upssource
    fi
}

usage () {
    echo "" >&2
    echo "usage: `basename $0` [options]" >&2
    echo "options:" >&2
    echo "     -h, --help:    prints this usage message                " >&2
    echo "     -c:            Use c-shell                              " >&2
    echo "     -s:            Use sh-shell (standard)                  " >&2
    echo "     -r, --release: specifies the release to be set up       " >&2
    echo "     -b, --build:   specifies the build (debug/prof/maxopt)  " >&2
    echo "                                                             " >&2
    echo "You either requested the help or setup has failed. Try again!" >&2
    echo "                                                             " >&2
    exit
}

process_args () {
    while getopts "hcsr:b:-:" opt; do
	if [ "$opt" = "-" ]; then
	    opt=$OPTARG
	fi
	case $opt in
	    h | help)
		usage
		;;
	    s | sh)
		shell_type=sh
		;;
	    c | csh)
		shell_type=csh
		;;
	    r | release)
		release=$OPTARG
		;;
	    b | build)
		build=$OPTARG
		;;
	    ?)
		usage
		;;
	esac
    done

    # Check for argument with no option
    if [ $OPTIND -eq $# ]; then
	echo -e "\e[01;31mERROR! Invalid argument/option. Try again!\e[0m" >&2
	usage
    fi
    shift $((OPTIND - 1))

    # Check for further stray arguments between options
    while (( "$#" )); do
	if [[ $1 == -* ]] ; then
	    echo -e "\e[01;31mERROR! All options up front, please.\e[0m" >&2
	    usage
	fi
	shift
    done
}

find_output_file_name () {
    output_file="/tmp/env_tmp.$$"
    if [ -f $output_file ]; then
	i=0
	while [ -f $output_file ]; do
	    i=`expr $i + 1`
	    output_file="/tmp/env_tmp.$i.$$"
	done
	fi
}

get_vars () {
    process_args $*
}

print_var () {
    # print a statement to set a variable in the desired shell type
    local_style=$3
    if [ "$local_style" = "sh" ]; then
	echo "$1=\"$2\"" >> $output_file
	echo "export $1" >> $output_file
    elif [ "$local_style" = "csh" ]; then
	echo "setenv $1 \"$2\"" >> $output_file
    elif [ "$local_style" = "human" ]; then
	echo "$1 = \"$2\""
    fi
}

unprint_var () {
    # print a statement to set a variable in the desired shell type
    local_style=$2
    if [ "$local_style" = "sh" ]; then
	echo "unset $1" >> $output_file
    elif [ "$local_style" = "csh" ]; then
	    echo "unsetenv $1" >> $output_file
    elif [ "$local_style" = "human" ]; then
	echo "unsetting $1"
    fi
}

insert_source () {
    echo "source $1" >> $output_file
}

insert_cmd () {
    echo "$1" >> $output_file
}

set_extern () {
    # a file containing the ups setup command args for each external
    # product
    if [[ "$build" = "prof" || "$build" = "maxopt" ]];then
	srt_ups_versions='$SRT_PUBLIC_CONTEXT/setup/nova-offline-ups-externals-$SRT_BASE_RELEASE-prof'
    else
	srt_ups_versions='$SRT_PUBLIC_CONTEXT/setup/nova-offline-ups-externals-$SRT_BASE_RELEASE'
    fi

    # the script to run the setups
    srt_ups_script='$SRT_DIST/setup/setup_srt_ups.'$shell_type

    insert_source "$srt_ups_script $srt_ups_versions"
}

set_srt () {

    # Source the srt setup file
    insert_source "\$SRT_DIST/srt/srt.$shell_type"

    # setup desired release for the user. This adds the lib and bin areas
    # for the chosen base release to $path and to LD_LIBRARY_PATH.
    #
    # First try to unsetup the current settings

    if [ $release = "none" ]; then
      	echo "Skipping SRT Setup"
    else
       	srt_setup_cmd="srt_setup -d"
       	if [ $release = "default" ]; then
	    srt_setup_cmd="$srt_setup_cmd SRT_QUAL=$build"
	else
	    srt_setup_cmd="$srt_setup_cmd SRT_QUAL=$build SRT_BASE_RELEASE=$release"
       	fi
	insert_cmd "srt_setup --unsetup"
	insert_cmd "$srt_setup_cmd"
    fi

    #set environmental variables necessary for using ART FileInPath functionality
    print_var FW_BASE         "\${SRT_PUBLIC_CONTEXT}"                                 $shell_type
    print_var FW_RELEASE_BASE "\${SRT_PUBLIC_CONTEXT}"                                 $shell_type
    print_var FW_DATA         "\${CVMFS_DISTRO_BASE}/nova/data:\${CVMFS_DISTRO_BASE}/grid/fermiapp/nova/aux:\${CVMFS_DISTRO_BASE}/nusoft/data/flux"   $shell_type
    print_var NOVA_ANA          "/nova/ana"                                              $shell_type
    print_var NOVA_APP          "/nova/app"                                              $shell_type
    print_var NOVA_DATA         "/nova/data"                                             $shell_type
    print_var NOVA_PROD         "/nova/prod"                                             $shell_type
    print_var CONDOR_EXEC       "/nova/app/condor-exec/\${USER}"                           $shell_type
    print_var CONDOR_TMP        "/nova/app/condor-tmp/\${USER}"                            $shell_type
    print_var PNFS_NOVA_DATA    "/pnfs/nova/data"                                                             $shell_type
    print_var PNFS_NOVA_SCRATCH "/pnfs/nova/scratch/users"                                                    $shell_type
    print_var PNFS_NOVA_USERS   "/pnfs/nova/users"                                                            $shell_type

    # this one is fermilab site-specific, need to remove somehow
    # possibly put in setup function in bash
    print_var FW_SEARCH_PATH  "\${SRT_PUBLIC_CONTEXT}/:\${FW_DATA}:./" 		       $shell_type
    print_var NOVADOCPWDFILE  "/nova/app/home/novasoft/doc_db_pwd" 		       $shell_type

}

set_devdb () {

    print_var NOVADBHOST      "ifdbrep.fnal.gov"                                       $shell_type
    print_var NOVADBHOST1     "ifdbprod.fnal.gov"                                      $shell_type
    print_var NOVADBWSURL     "http://novacon-data.fnal.gov:8091/NOvACon/v2_2b/app/"   $shell_type
    print_var NOVADBWSURLINT  "http://novacon-data.fnal.gov:8109/NOvACon/v2_2b/app/"   $shell_type
    print_var NOVADBWSURLPUT  "http://novacon-data.fnal.gov:8107/NOvACon/v2_2b/app/"   $shell_type
    print_var NOVADBQEURL     "http://novacon-data.fnal.gov:8105/QE/NOvA/app/SQ/"      $shell_type
    print_var NOVADBNAME      "nova_prod"   					       $shell_type
    print_var NOVADBUSER      "nova_reader"                                            $shell_type
    print_var NOVADBPWDFILE   "\${SRT_PUBLIC_CONTEXT}/Database/config/nova_reader_pwd" $shell_type
    print_var NOVADBGRIDPWDFILE "\${SRT_PUBLIC_CONTEXT}/Database/config/nova_grid_pwd" $shell_type
    print_var NOVADBWSPWDFILE "/nova/app/db/nova_devdbws_pwd"                          $shell_type
    print_var NOVADBPORT      "5433"                                                   $shell_type
    print_var NOVAHWDBHOST    "ifdbrep.fnal.gov"                                       $shell_type
    print_var NOVAHWDBHOST1   "ifdbprod.fnal.gov"                                      $shell_type
    print_var NOVAHWDBNAME    "nova_hardware"   				       $shell_type
    print_var NOVAHWDBUSER    "nova_reader"                                            $shell_type
    print_var NOVAHWDBPORT    "5432"                                                   $shell_type
    print_var NOVADBTIMEOUT   "30"                                                     $shell_type
    print_var NOVANEARDAQDBHOST "ifdbrep.fnal.gov"                                     $shell_type
    print_var NOVANEARDAQDBNAME "nova_prod"                                            $shell_type
    print_var NOVANEARDAQDBPORT "5434"                                                 $shell_type
    print_var NOVANEARDAQDBUSER "nova_grid"                                            $shell_type
    print_var NOVAFARDAQDBHOST "ifdbrep.fnal.gov"                                      $shell_type
    print_var NOVAFARDAQDBNAME "nova_prod"                                             $shell_type
    print_var NOVAFARDAQDBPORT "5436"                                                  $shell_type
    print_var NOVAFARDAQDBUSER "nova_grid"                                             $shell_type
    print_var NOVAFARDCSDBHOST "ifdbrep.fnal.gov"                                      $shell_type
    print_var NOVAFARDCSDBNAME "nova_prod"                                             $shell_type
    print_var NOVAFARDCSDBPORT "5437"                                                  $shell_type
    print_var NOVAFARDCSDBUSER "nova_grid"                                             $shell_type
}

# Data Handling variables
set_dh () {

    print_var IFDH_BASE_URI  "http://samweb.fnal.gov:8480/sam/nova/api"  $shell_type

    print_var SAM_STATION    "nova"                                      $shell_type
    print_var SAM_EXPERIMENT "nova"                                      $shell_type
    print_var EXPERIMENT     "nova"                                      $shell_type
    print_var GROUP          "nova"                                      $shell_type
    print_var JOBSUB_GROUP   "nova"                                      $shell_type

    print_var SITES          "Fermigrid"                                  $shell_type
    print_var ALLSITES       "FZU_NOVA,Fermigrid,Harvard,UCSD,SMU,OSC,Wisconsin,MWT2,Nebraska" $shell_type

    print_var NOVA_FLUX_VERSION "nova_v08"                               $shell_type

    # Minimize xrootd timeouts by retrying more. Should be equivalent to
    # setting XNet.MaxRedirectCount in .rootrc
    print_var XRD_REDIRECTLIMIT  255                                     $shell_type
    # There are rumours that this variable also exists and is relevant...
    print_var XRD_CONNECTIONRETRY 255                                    $shell_type
}

set_paths () {

    print_var LD_LIBRARY_PATH "\${LD_LIBRARY_PATH}:\${LHAPDF_FQ_DIR}/lib"              $shell_type
    print_var LD_LIBRARY_PATH "\${LD_LIBRARY_PATH}:\${GENIE}/lib"                      $shell_type
    print_var ROOT_INCLUDE_PATH "\${SRT_PUBLIC_CONTEXT}/include/OnlineMonitoring:\${SRT_PUBLIC_CONTEXT}/include/OnlineMonitoring/viewer:\${SRT_PUBLIC_CONTEXT}/include/EventDisplay:\${SRT_PUBLIC_CONTEXT}/include:\${ROOT_INCLUDE_PATH}"         $shell_type
    print_var FHICL_FILE_PATH "./:\${SRT_PUBLIC_CONTEXT}/job/:\${SRT_PUBLIC_CONTEXT}/:\${FHICL_FILE_PATH}" $shell_type
    
#JPD -- Request from Martin Frank 14/09/25 to ${FW_SEARCH_PATH} with ${NOVADDT_DIR}/source, for multi-point conversion (DDTConverters)
#       ${NOVADDT_DIR} is only available once 'set_extern' has been called (as this sets up external ups products)
#       In order to append ${FW_SEARCH_PATH} with ${NOVADDT_DIR}/source we must do this here
    print_var FW_SEARCH_PATH "\${FW_SEARCH_PATH}:\${NOVADDT_DIR}/source"               $shell_type
#L -- Request from Shih-Kai Lin 18/10/18 for the new product ROOUNFOLD
    print_var LD_LIBRARY_PATH "\${LD_LIBRARY_PATH}:\${ROOUNFOLD_LIB}"                             $shell_type
    print_var ROOT_INCLUDE_PATH "\${ROOT_INCLUDE_PATH}:\${ROOUNFOLD_INC}:\${ROOUNFOLD_INC}/../"   $shell_type

}

set_artworkbook () {

    print_var ART_WORKBOOK_WORKING_BASE "\${NOVA_APP}/users"                           $shell_type
    print_var ART_WORKBOOK_OUTPUT_BASE  "\${NOVA_APP}/users"                           $shell_type
    print_var ART_WORKBOOK_QUAL         "nu:e5"                                        $shell_type

}

set_other () {

    print_var ACK_OPTIONS "--type-set=fcl=.fcl"                                        $shell_type

}

finish () {
	echo "/bin/rm $output_file" >> $output_file
	echo $output_file
}

main () {

     set_defaults
     get_vars $*
     set_srt
     set_ups
     set_extern
     set_dh
     set_devdb
     set_paths
     set_artworkbook
     set_other
     finish
}

main $*
