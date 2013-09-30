#!/bin/sh
clear
echo -e "\n\t MENU\n"

PS3="Select an option and press [Enter]: "
select i in OS HOST FS Date Quit
do
	case $i in
		OS) echo
			uname
			;;
		HOST) echo
			hostname
			;;
		FS) echo
			df -k | more
			;;
		Date) echo
			date
			;;
		Quit) break
			;;
	esac

	REPLY=

	echo -e "\nPress Enter to Continue. . . \c"
	read

	clear
	echo -e "\n\t MENU\n"
done
clear

