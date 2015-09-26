PS: 最近在整理一些资料，结合自己写的这些工具以及自己所学知识，出一份关于脱壳的的ppt，大家尽情期待我会传到这上来的。

----------update 2015.9.26--------------------
修复了一个terrible bug，现在可以完全dump阿里的odex文件并通过其他工具还原成java代码，腾讯的还有一点问题需要处理，并不能还原完整的java代码，继续调试
同时上传一些简单的脚本
usage：
./auto push :将桌面的libdump.so push到手机的/system/lib/
./auto pull :将dump出来的dex pull到桌面
注：其中路径、包名等记得修改喔

基于Xposed的不那么通用的脱壳机

首先声明需要安装Xposed，其次仅测试了腾讯加固宝和阿里聚。因为只逆向过这两个，知道原理，调试起来也比较容易。
测试环境：Android 4.1

需要感谢
http://bbs.pediy.com/showthread.php?t=203776
http://bbs.pediy.com/showthread.php?t=190494&highlight=zjdroid
两位大大
部分代码结构完全来源于两位

原理挺简单，大伙直接看代码就一清二楚了。

使用步骤：
1.点击启动应用后可以在log输出中找到cookie
adb logcat -s cc

2.dump指定cookie对应的dex文件（直接从odex中扣取dex文件）
adb shell am broadcast -a com.cc.dumpapk --ei cmd 1 --ei cookie xxxxxxx
注：我将需要hook的应用包名com.cc.test硬编码到程序中，只能dump这个应用，有需要的请自行修改,以及编译好的so文件需要放置到/system/lib/目录下

3.dump出的dex文件：/data/data/com.cc.test/whole.dex

测试腾讯的时候直接dump的dex代码中含有odex opcode，本来想那就直接dump整个odex文件，但是不知道为何dump出来的odex经过
baksmali.jar处理报了一堆错误，调试多天无果。卒。
还没来得及测试其他加固产品，脱壳原理可能比较有针对性，因而不适合其他产品。
