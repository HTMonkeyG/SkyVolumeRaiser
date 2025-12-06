# 光遇音量保持器 (Sky Volume Keep)
<p align="center">
<img src="./res/skycol-volrst-ico.ico">
</p>
&emsp;一个为光遇网易云提供软音量设置、防止音量合成器被篡改的插件。

## 开发目的
&emsp;由于国服光遇PC端`NcmAudioPlayer.dll`网易云播放器的特性，在游戏内网易云音乐修改音量时，游戏事实上修改的是系统的音量合成器设置，导致游戏整体的音量被修改。<br>
&emsp;即使手动将合成器复位，下一次打开共享音符、音乐桌等物品时，合成器仍然会被修改为游戏内网易云音量值。

## 插件特性
&emsp;本插件为光遇网易云播放器提供了软件音量设置，同时阻止网易云播放器修改游戏进程的音频会话音量。

## 使用方式
&emsp;将`sky-volume-keep.dll`注入至游戏进程即可使用。**暂时不提供可执行文件，请自行下载编译。**
- 若使用[HTModLoader](https://www.github.com/HTMonkeyG/HTML-Sky)注入，则下载`sky-volume-keep.dll`与`manifest.json`，并按照HTModLoader的教程操作即可。
- 若使用远端线程直接注入（如RemoteDLL64），则只需下载`sky-volume-keep.dll`，直接注入即可。
- 若使用反射式注入，下载`sky-volume-keep.exe`，在游戏运行后双击并赋予管理员权限即可。

## 编译方式
&emsp;使用MinGW-x64进行编译。克隆并切换至仓库根目录，在cmd中运行mingw-make.exe即可。
