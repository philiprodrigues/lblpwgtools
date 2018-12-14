
# reads in a file consisting of arguments to UPS setup command

# P. Shanahan 10/7/09


# setup_srt_ups filename [-u]
# -u option causes unsetup for each product

export UPS_OPTIONS=-B

if [ $# -eq 0 ]; then
	echo "usage: setup_srt_ups filename [-u]"
	echo " "
	echo "filename contains arguments for ups setup commands."
	echo "-u causes unsetup for each product prior to setup."
	return
fi

version_file=$1

if [ ! -f $version_file ]; then
	echo "UPS products version file $version_file not found!"
	return 1
fi

if [ $# -eq 2 ]; then
	if [ "$2" = "-u" ]; then
		do_unsets=1
	fi
fi



while read line 
do
	if [ -n "$do_unsets" ]; then
		prodname=`echo $line |sed -e "s/ .*$//g" `
		unsetup $prodname
		unset prodname
	fi
	setup -B $line

done < $version_file

unset do_unsets
