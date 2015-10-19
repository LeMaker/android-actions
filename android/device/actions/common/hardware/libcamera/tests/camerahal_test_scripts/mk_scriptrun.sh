#!/bin/sh


curpath=`pwd`
case `uname` in
    CYGWIN*)
    curpath=`cygpath -d $curpath`
    monkeyrunner=monkeyrunner.bat

    curpath=`echo $curpath|sed -e 's/\\\\/\\\\\\\\/g'`

    $monkeyrunner $curpath\\\\mk_scriptloader.py $@
    ;;

    *)
    monkeyrunner=monkeyrunner
    $monkeyrunner $curpath/mk_scriptloader.py $@
    ;;
esac


