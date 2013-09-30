#!/bin/sh

test_string()
{
	if [[ $# -ne 1 ]]; then
		echo "ERROR: $(basename $0) require only one parameter"
		return 1
	fi

	test_string=$1
	case ${test_string} in
		[0-9]*.[0-9]*.[0-9]*.[0-9]*)
			IP_INVALID=FALSE
			for i in $(echo ${test_string}|tr "." " ")
			do
				if [[ ${i} > 255 ]];then 
					IP_INVALID=TRUE
				fi
			done

			case ${IP_INVALID} in
				TRUE) echo 'INVALID IP ADDRESS'
					;;
				FALSE) echo 'VALID IP ADDRESS'
					;;
			esac
			;;
		[0-1]*)
			echo 'BINARY OR POSTIVE_INTEGER'
			;;
	esac
}
test_string 11.22.33.44
test_string 11101
