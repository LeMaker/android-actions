./autobuild.sh config
cd android/
source build/envsetup.sh
lunch
make -j8
cd ../
./autobuild.sh
 

