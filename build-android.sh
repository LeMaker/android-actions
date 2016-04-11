rm android/out
rm owl/out
cd android
ln -s out-master out
cd ../owl/
ln -s out-master out
cd ..
./autobuild.sh config
./autobuild.sh u-boot
./autobuild.sh kernel
cd android/
source build/envsetup.sh
lunch
#make -j8
cd ../
./autobuild.sh
 

