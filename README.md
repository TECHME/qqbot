#  qqbot = 基于webqq协议的QQ群聊天记录机器人

qqbot 连通 IRC 和  QQ群，并能实时记录聊天信息。每日自动生成新的日志文件。

## 功能介绍

### 登录QQ，记录群消息
### 登录IRC，记录IRC消息
### 将群消息转发到IRC
### 将IRC消息转发到QQ群
### QQ图片转成 url 链接给 IRC

# 编译办法

项目使用 cmake 编译。编译办法很简单

	mkdir build
	cd build
	cmake [qqbot的路径]
	make -j8

# 使用

读取配置文件 /etc/qqbotrc

配置文件的选项就是去掉 -- 的命令行选项。比如命令行接受 --user 
配置文件就写

	user=qq号码

就可以了。
命令行选项看看 --help 输出


## 频道组

使用 --map 功能将频道和QQ群绑定成一组。被绑定的组内消息互通。

用法：  --map=qq:123456,irc:avplayer;qq:3344567,irc:otherircchannel

频道名不带 \# 

也可以在 /etc/qqbotrc 或者 ~/.qqbotrc 写，每行一个，不带 --。
如 map=qq:123456,irc:avplayer;qq:3344567,irc:otherircchannel


频道组用 ; 隔开。组成份间用,隔离。