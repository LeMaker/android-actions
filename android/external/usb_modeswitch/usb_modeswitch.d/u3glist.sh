#!/bin/sh

U3GFILE=u3glist.txt
TMPFILE=tmp.log

# read 3g config
for file in *
do
    if [ -f "$file" -a "${file:4:1}" = "_" ]; then    
        # save vid_pid before switch
        newfile=${file:0:9}
        echo "$newfile" >> "$TMPFILE"
        
        # parse vid_pid after switch
        cat "$file" | while read line
        do
            # remove space
            newline=${line//\ /}
            
            # parse vid
            if [ x${newline:0:15} = x"TargetVendor=0x" ] ; then
                vid=${newline:15:4}

                # read next line
                while [ ${#line2} -le 0 ]
                do
                    read line2
                done
                
                # remove space
                newline=${line2//\ /}
                
                if [ x${newline:0:16} = x"TargetProduct=0x" ] ; then
                    pid=${newline:16:4}
                    echo "$vid"_"$pid" >> "$TMPFILE"
                elif [ x${newline:0:18} = x"TargetProductList=" ] ; then
                    # parse pid list
                    pids=${newline:18}
                    pids=${pids//\"/}
                                        
                    while [ ${#pids} -gt 0 ]
                    do
                        pid=${pids:0:4}
                        pids=${pids:5}
                        echo "$vid"_"$pid" >> "$TMPFILE"
                    done
                fi
                break
            fi
        done
    fi
done

# sort & remove reduplicate item
sort -k2n "$TMPFILE" | uniq > "$U3GFILE"

# remove tmp file
rm "$TMPFILE"
