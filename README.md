#  qqbot = 基于webqq协议的QQ群聊天记录机器人

qqbot 连通 IRC 和  QQ群，并能实时记录聊天信息。每日自动生成新的日志文件。


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
