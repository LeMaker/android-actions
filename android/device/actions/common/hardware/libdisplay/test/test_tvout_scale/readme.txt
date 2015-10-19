test_tvout_scale测试用例使用说明
    1.test_tvout_scale测试用例可实现对TV端输出尺寸进行调整。
    2.使用方法
    (1)将test_tvout_scale.apk(所在位置参考Android.mk)通过adp push到小机/data/app目录;
    (2)点击小机页面的test_tvout_scale图标，启动测试用例，在显示界面的文本框内输入
    “x方向缩放比，y方向缩放比”，然后点击确定即可完成设置，其中x方向缩放比和y方向缩放比
    均为整形数据，且取值范围均为0~50.