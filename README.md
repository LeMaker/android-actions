The Source Code for the S500 SOC is based on the Actions android SDK.The "owl" directory that includes some tools, scripts and configure of the different boards.
## Usage
        1. ./autobuild.sh config           # choose different android's boards type according to your needs
        2. ./autobuild.sh                  # make android's platform firmware package

## Help informaiton
For more informaiton, please type the command , as follows:

        make                 - Builds android's platform firmware package
        make kernel          - Builds android kernel
        make uboot           - Builds u-boot
        make modules         - Builds modules
        make rootfs          - Builds android rootfs

## Firmware Package
The source code will generate a Android Platform Firmware with "xxx.fw" when you run the command **'./autobuild.sh'**, and the package will be saved to the directory **"owl/out"**.
## Attentions
If you compile with the Source Code with Root, may be fail in this process.So you must be a normal user to build this process.

## Feedback and Improvements
If you meet some bugs, you can fix it and feel free to contribute your code to the Repository. Of course, you can also report bugs to <http://bugs.lemaker.org/>

