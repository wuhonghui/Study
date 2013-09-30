#!/bin/sh

function while_read_line_bottom_fd_out
{
	>output.txt
# 1
	exec 4<&1
	exec 1>output.txt
	while read LINE
	do
		echo "$LINE"
	done < input.txt

	exec 1<&4
	exec 4>&-

}

ll>input.txt
while_read_line_bottom_fd_out
