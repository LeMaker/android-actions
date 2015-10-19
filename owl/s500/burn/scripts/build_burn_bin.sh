#!/bin/sh

P=`pwd`
MAKER_DIR=${P}/../../../tools/fw_maker


if [ -f $MAKER_DIR/maker_install.run ]; then
	mkdir -p $MAKER_DIR/Output
	cd $MAKER_DIR && ./maker_install.run && cd $P
	python -O $MAKER_DIR/Output/PyMaker.pyo -c $1 --forceunextract -o $2 --mf 1
else
    wine $MAKER_DIR/Maker.exe -c $1 -forceunextract -o $2
fi

