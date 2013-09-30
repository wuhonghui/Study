#!/bin/sh
function dot_line
{
	while true
	do
		echo -e -n "."
		sleep $1
	done
}

function rotate_line
{
	rotate_interval=1
	rotate_count=1
	while true
	do
		case ${rotate_count} in
			"1")
				echo -n -e '-'"\b\c"
				;;
			"2")
				echo -n -e '\\'"\b\c"
				;;
			"3")
				echo -n -e '|'"\b\c"
				;;
			"4")
				echo -n -e '/'"\b\c"
				rotate_count=0
				;;
		esac
		sleep $1
		rotate_count=`expr ${rotate_count} + 1`
	done
}


rotate_line 1 &
DOT_LINE_PID=$!
sleep 10
kill -9 ${DOT_LINE_PID}
