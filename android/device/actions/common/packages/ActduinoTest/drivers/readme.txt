##ActduinoTest drivers

1.将ActduinoTest\drivers\actduinotest拷贝到kernel\drivers目录
2.修改编译文件
  kernel\drivers\Kconfig,添加source "drivers/actduinotest/Kconfig"
  kernel\drivers\Makefile添加obj-y    += actduinotest/
3.将kernel\arch\arm\configs\actduino_s500_defconfig拷贝到kernel/.config
4.编译
  在kernel目录下
  $make menuconfig
  $make modules
  
  将会生成
  kernel/drivers/actduinotest/i2c_test/i2c_owl_test.ko
  
kernel/drivers/actduinotest/spi_test/spi_owl_test.ko

  kernel/drivers/actduinotest/gpio_test/gpio_owl_test.ko
  
kernel/drivers/actduinotest/adc_test/adc_owl_test.ko

  