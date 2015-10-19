for f in `/bin/ls L_CAMERA_UC_*.py  2> /dev/null`
do
    if [ -f $f ]; then
        testid=`echo $f|sed -e 's/L_CAMERA_UC_//' -e 's/\.py//'`
        echo "-------------START TESTCASE ID:"$testid"------------------------"
        ./mk_scriptrun.sh $f 
    fi
done      
