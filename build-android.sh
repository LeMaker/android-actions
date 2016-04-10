rm android/out
rm owl/out
ln -s android/out-master android/out
ln -s owl/out-master owl/out
./autobuild.sh config
./autobuild.sh u-boot
./autobuild.sh kernel
cd android/
source build/envsetup.sh
lunch
#make -j8
cd ../
./autobuild.sh
 

