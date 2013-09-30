#!/bin/sh

function random_int()
{
	RANDOM=$$
	echo "$((${RANDOM} % $1 + 1))"
}

function random_dev()
{
	echo "$(dd if=/dev/random count=1 2>/dev/null | od -t u4 | awk '{print $2}' | head -n 1)"
}
random_dev 300
