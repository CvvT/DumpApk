# DumpApk
* Xposed Module to automatically unpack Apk
* Intuition: reside in the app's context to collect info from Dalvik Data Structure.

----------update 2015.10.19--------------------<br>
分享一份自己做的ppt，仅供学习交流，侵删<br>
PS: 最近在整理一些资料，结合自己写的这些工具以及自己所学知识，出一份关于脱壳的的ppt，大家尽情期待我会传到这上来的。<br>
<br>
----------update 2015.9.30--------------------<br>
<br>
终于明白tecent加固脱壳失败的原因了。试了一下没有加固过的apk发现将odex转成dex同样报错，才知道原来是手机的问题，测试用的是小米。
考虑换一部测试手机，或者移植baksmali。<br>
<br>
----------update 2015.9.26--------------------<br>
修复了一个terrible bug，现在可以完全dump阿里的odex文件并通过其他工具还原成java代码，腾讯的还有一点问题需要处理，并不能还原完整的java代码，继续调试<br>
同时上传一些简单的脚本<br>
usage：<br>
./auto push :将桌面的libdump.so push到手机的/system/lib/<br>
./auto pull :将dump出来的dex pull到桌面<br>
注：其中路径、包名等记得修改喔<br>
-------update 2015.9.26-----------------<br>
基于Xposed的不那么通用的脱壳机<br>
首先声明需要安装Xposed，其次仅测试了腾讯加固宝和阿里聚。因为只逆向过这两个，知道原理，调试起来也比较容易。<br>
测试环境：Android 4.1<br>

需要感谢<br>
http://bbs.pediy.com/showthread.php?t=203776<br>
http://bbs.pediy.com/showthread.php?t=190494&highlight=zjdroid<br>
两位大大<br>
部分代码结构完全来源于两位<br>
<br>
原理挺简单，大伙直接看代码就一清二楚了。<br>
<br>
使用步骤：<br>
1.点击启动应用后可以在log输出中找到cookie<br>
adb logcat -s cc<br>
<br>
2.dump指定cookie对应的dex文件（直接从odex中扣取dex文件）<br>
adb shell am broadcast -a com.cc.dumpapk --ei cmd 1 --ei cookie xxxxxxx<br>
注：我将需要hook的应用包名com.cc.test硬编码到程序中，只能dump这个应用，有需要的请自行修改,以及编译好的so文件需要放置到/system/lib/目录下<br>
<br>
3.dump出的dex文件：/data/data/com.cc.test/whole.dex\<br>
<br>
测试腾讯的时候直接dump的dex代码中含有odex opcode，本来想那就直接dump整个odex文件，但是不知道为何dump出来的odex经过
baksmali.jar处理报了一堆错误，调试多天无果。卒。<br>
还没来得及测试其他加固产品，脱壳原理可能比较有针对性，因而不适合其他产品。<br>
