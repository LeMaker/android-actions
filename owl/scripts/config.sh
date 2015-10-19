#!/bin/bash -e
#
# (C) Copyright 2015, actions Limited
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.


unset MENU_CHOICES_ARRAY
add_item(){
	local new_item=$1
	local c
	for c in ${MENU_CHOICES_ARRAY[@]} ; do
		if [ "$new_item" = "$c" ] ; then
			return
		fi
	done
	MENU_CHOICES_ARRAY=(${MENU_CHOICES_ARRAY[@]} $new_item)
}

build_ic_list() {
	unset MENU_CHOICES_ARRAY
	if [ -d "s500" ]
	then
		add_item "s500"
	fi
	
	if [ -d "s700" ]
	then
		add_item "s700"
	fi
	
	if [ -d "s900" ]
	then
		add_item "s900"
	fi
}

build_os_list() {
	unset MENU_CHOICES_ARRAY
	local file
	for file in ` ls -1 $1`
	do
		if [ -d $1"/"$file ]
		then
			if [ -d .."/"$file ]
			then
				add_item $file
			fi
		fi

	done
}

build_list() {
	unset MENU_CHOICES_ARRAY
	local file
	for file in ` ls -1 $1`
	do
		if [ -d $1"/"$file ]
		then
			add_item $file
		fi

	done
}

print_menu(){
	local i=1
	local choice
	for choice in ${MENU_CHOICES_ARRAY[@]}
	do
		echo "     $i. $choice"
		i=$(($i+1))
	done

	echo
}


read_choice(){
		####################################### select ic type
    local answer
    build_ic_list
    
    if [ ${#MENU_CHOICES_ARRAY[@]} -eq 0 ]
    then
        echo "error: no ic."
        return
    elif [ ${#MENU_CHOICES_ARRAY[@]} -eq 1 ]
    then
        selected_ic=${MENU_CHOICES_ARRAY[0]}
    else
        echo "Select IC name:"
        print_menu
        echo -n "Which would you like? [${MENU_CHOICES_ARRAY[0]}] "
        read answer
	
        local selected_ic=

        if [ -z "$answer" ]
        then
            selected_ic=${MENU_CHOICES_ARRAY[0]}
        elif (echo -n $answer | grep -q -e "^[0-9][0-9]*$")
        then
            if [ $answer -le ${#MENU_CHOICES_ARRAY[@]} ]
            then
                selected_ic=${MENU_CHOICES_ARRAY[$(($answer-1))]}
            fi
        elif (echo -n $answer | grep -q -e "^[^\-][^\-]*-[^\-][^\-]*$")
        then
            selected_ic=$answer
        fi

        if [ -z "$selected_ic" ]
        then
            echo
            echo "Invalid lunch combo: $answer"
            return 1
        fi
    
        echo
    fi

    
		###################################### select os type
    build_os_list $selected_ic/boards
    
    if [ ${#MENU_CHOICES_ARRAY[@]} -eq 0 ]
    then
        echo "error: no os."
        return
    elif [ ${#MENU_CHOICES_ARRAY[@]} -eq 1 ]
    then
        selected_os=${MENU_CHOICES_ARRAY[0]}
    else
        echo "Select build os:"
        print_menu
        echo -n "Which would you like? [${MENU_CHOICES_ARRAY[0]}] "
        read answer
	
        local selected_os=

        if [ -z "$answer" ]
        then
            selected_os=${MENU_CHOICES_ARRAY[0]}
        elif (echo -n $answer | grep -q -e "^[0-9][0-9]*$")
        then
            if [ $answer -le ${#MENU_CHOICES_ARRAY[@]} ]
            then
                selected_os=${MENU_CHOICES_ARRAY[$(($answer-1))]}
            fi
        elif (echo -n $answer | grep -q -e "^[^\-][^\-]*-[^\-][^\-]*$")
        then
            selected_os=$answer
        fi

        if [ -z "$selected_os" ]
        then
            echo
            echo "Invalid lunch combo: $answer"
            return 1
        fi
    
        echo 
    fi
    
    ############################select board type
    build_list $selected_ic/boards/$selected_os/
    echo "Select board type:"
    print_menu
    echo -n "Which would you like? [${MENU_CHOICES_ARRAY[0]}] "
    read answer
	
    local selected_board=

    if [ -z "$answer" ]
    then
        selected_board=${MENU_CHOICES_ARRAY[0]}
    elif (echo -n $answer | grep -q -e "^[0-9][0-9]*$")
    then
        if [ $answer -le ${#MENU_CHOICES_ARRAY[@]} ]
        then
            selected_board=${MENU_CHOICES_ARRAY[$(($answer-1))]}
        fi
    elif (echo -n $answer | grep -q -e "^[^\-][^\-]*-[^\-][^\-]*$")
    then
        selected_board=$answer
    fi

    if [ -z "$selected_board" ]
    then
        echo
        echo "Invalid lunch combo: $answer"
        return 1
    fi
    
    ##############################################################
    cat $selected_ic/boards/$selected_os/$selected_board/config > .config
    echo IC_NAME=$selected_ic >> .config
		echo OS_NAME=$selected_os >> .config
		echo BOARD_NAME=$selected_board >> .config
		rm -f out/$selected_ic"_"$selected_os"_"$selected_board/kernel/.config
		echo "$selected_ic $selected_os $selected_board configured."
}


if [ $# -eq 3 ] && [ -d "$1" ] && [ -d "$1/boards/$2" ] && [ -d "$1/boards/$2/$3" ];then
	cat $1/boards/$2/$3/config > .config
	echo IC_NAME=$1 >> .config
	echo OS_NAME=$2 >> .config
	echo BOARD_NAME=$3 >> .config
	rm -f out/$1_$2_$3/kernel/.config

	echo "$1 $2 $3 configured."
else
	read_choice
fi


