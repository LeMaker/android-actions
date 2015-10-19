for f in `/bin/ls L_CAMERAHAL_*.txt  2> /dev/null`
do
    if [ -f $f ]; then
        testid=`echo $f|sed -e 's/L_CAMERAHAL_//' -e 's/\.txt//'`
        echo "-------------START TESTCASE ID:"$id"------------------------"
        ./camerahal_scriptrun.sh $testid 
    fi
done      
