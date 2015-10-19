本工具提供了对“WIFI mac地址”和“蓝牙mac地址”的设置

==== 使用方法 ====
把本工具的目录copy到sd卡，插入升级后的机器中，用FileBroswer打开这个目录，点击“write_mac.sh”即可


==== 配置文件 ====
1. 文件格式
示例如下，
mac_type=wifi_mac_addr
wifi_addr_min=10:01:FF:00:00:00
wifi_addr_cur=10:01:FF:B1:00:2A
wifi_addr_max=10:01:FF:FF:FF:FF
mac_type=bt_mac_addr
bt_addr_min=00:01:BB:00:00:00
bt_addr_cur=00:01:BB:77:8C:20
bt_addr_max=00:01:BB:FF:FF:FF

2. 格式说明。
(01) mac_type 代表烧写mac地址的类型，可以是“wifi_mac_addr”或“btaddr”。本工具既支持同时烧写wifi mac地址和蓝牙mac地址，也支持单独烧写蓝牙mac地址或wifi mac地址。
     “wifi_mac_addr” -- wifi的mac地址。
     “btaddr”  -- 蓝牙的Mac地址。
     示例1) mac_type=wifi_mac_addr
            需要烧写wifi mac 地址
     示例2)  mac_type=bt_mac_addr
            需要烧写蓝牙 mac 地址

(02) wifi MAC地址(wifi_mac_addr)
wifi_addr_min=10:01:FF:00:00:00  -- 表示MAC的起始地址(16进制)，是10:01:FF:00:00:00。
wifi_addr_cur=10:01:FF:B1:00:2A  -- 表示当前会被写入机器的地址，是10:01:FF:B1:00:2A。
wifi_addr_max=10:01:FF:FF:FF:FF  -- 表示MAC的终止地址(16进制)，是10:01:FF:FF:FF:FF。
     a) wifi_mac_addr和btaddr的格式要求类似。
     b) min,cur和max的格式必须都满足xx:xx:xx:xx:xx:xx的形式。其中，x是16进制符号，即x属于0123456789ABCDEFabcdef ，如果xx中前面一个为0，支持直接省略前面一个0。
     c) cur的值必须介于[min,max]之间。
     d）wifi MAC地址前24位为厂商id，同一厂商使用的厂商id应该相同，所以要求wifi_addr_min、wifi_addr_cur、wifi_addr_max前面一半（MAC地址10:01:FF:00:00:00前面一半的内容为10:01:FF:）的内容应该相同。
     e) 要求第一个字节的后面两位必须为0。也就是aa:bb:cc:dd:ee:ff 中aa必须最低两位为0（能被4整除）。
     f) 地址不可以为全0。
     g) 执行一次写入操作之后，cur的值会+1。


(03) 蓝牙地址(btaddr)
bt_addr_min=00:01:BB:00:00:00  -- 表示蓝牙的起始地址(16进制)，是00:01:BB:00:00:00。
bt_addr_cur=00:01:BB:77:8C:20  -- 表示当前会被写入机器的地址，是00:01:BB:77:8C:20。
bt_addr_max=00:01:BB:FF:FF:FF  -- 表示蓝牙的结束地址(16进制)，是00:01:BB:FF:FF:FF。
     a) min,cur和max的格式必须都满足xx:xx:xx:xx:xx:xx的形式。其中，x是16进制符号，即x属于0123456789ABCDEFabcdef。
     b) cur的值必须介于[min,max]之间。
     c) BT Mac address第一个byte必须是0,最后三个字节不可以为 0x9e8b00 ~ 0x9e8b3f
     d) 地址不可以为全0
     e) 执行一次写入操作之后，cur的值会+1。



***如果“cur”已经等于“end”，会有警告提示已经达到最大值。
***如果“cur”已经大于“end”，会有报错提示，若没有提示具体错误原因，可查看该目录下log文件夹下的log文件。
***显示“Success”的时候表示执行已经结束，可以直接拔卡（不必非要等待显示消失）




