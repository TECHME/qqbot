/**
 * @file   main.cpp
 * @author microcai <microcaicai@gmail.com>
 * @origal_author mathslinux <riegamaths@gmail.com>
 * 
 */

#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/program_options.hpp>
namespace po = boost::program_options;
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/noncopyable.hpp>
#include <boost/foreach.hpp>

#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <wchar.h>

#include "libirc/irc.h"
#include "libwebqq/webqq.h"
#include "utf8/utf8.h"

#define QQBOT_VERSION "0.0.1"

class qqlog : public boost::noncopyable
{
public:
	typedef boost::shared_ptr<std::ofstream> ofstream_ptr;
	typedef std::map<std::wstring, ofstream_ptr> loglist;

public:
	qqlog() : m_path(L".")
	{}

	~qqlog()
	{}

public:
	// 设置日志保存路径.
	void log_path(const std::wstring &path)
	{
		m_path = path;
	}
	// 设置日志保存路径.
	void log_path(const std::string &path)
	{
		m_path = utf8_wide(path);
	}
	// 添加日志消息.
	bool add_log(const std::wstring &groupid, const std::string &msg)
	{
		boost::mutex::scoped_lock l(m_mutex);

		// 在qq群列表中查找已有的项目, 如果没找到则创建一个新的.
		loglist::iterator finder = m_group_list.find(groupid);
		if (m_group_list.find(groupid) == m_group_list.end())
		{
			// 创建文件.
			ofstream_ptr file_ptr = create_file(groupid);

			m_group_list[groupid] = file_ptr;
			finder = m_group_list.find(groupid);
		}

		// 如果没有找到日期对应的文件.
		if (!fs::exists(fs::path(make_filename(make_path(groupid)))))
		{
			// 创建文件.
			ofstream_ptr file_ptr = create_file(groupid);
			if (finder->second->is_open())
				finder->second->close();	// 关闭原来的文件.
			// 重置为新的文件.
			finder->second = file_ptr;
		}

		// 得到文件指针.
		ofstream_ptr &file_ptr = finder->second;

		// 构造消息, 添加消息时间头.
		std::string data = "<p>" + current_time() + " " + msg + "</p>\n";
		file_ptr->write(data.c_str(), data.length());
		// 刷新写入缓冲, 实时写到文件.
		file_ptr->flush();

		return true;
	}

protected:

	// 构造路径.
	std::string make_path(const std::wstring &groupid) const
	{
		return (m_path / groupid).string();
	}

	// 构造文件名.
	std::string make_filename(const std::string &p = "") const
	{
		std::ostringstream oss;
		boost::posix_time::time_facet* _facet = new boost::posix_time::time_facet("%Y%m%d");
		oss.imbue(std::locale(std::locale::classic(), _facet));
		oss << boost::posix_time::second_clock::local_time();
		std::string filename = (fs::path(p) / (oss.str() + ".html")).string();
		return filename;
	}

	// 创建对应的日志文件, 返回日志文件指针.
	ofstream_ptr create_file(const std::wstring &groupid) const
	{
		// 生成对应的路径.
		std::string save_path = make_path(groupid);

		// 如果不存在, 则创建目录.
		if (!fs::exists(fs::path(save_path)))
		{
			if (!fs::create_directory(fs::path(save_path)))
			{
				std::cerr << "create directory " << save_path.c_str() << " failed!" << std::endl;
				return ofstream_ptr();
			}
		}

		// 按时间构造文件名.
		save_path = make_filename(save_path);

		// 创建文件.
		ofstream_ptr file_ptr(new std::ofstream(save_path.c_str(), 
			fs::exists(save_path) ? std::ofstream::app : std::ofstream::out));
		if (file_ptr->bad() || file_ptr->fail())
		{
			std::cerr << "create file " << save_path.c_str() << " failed!" << std::endl;
			return ofstream_ptr();
		}

		// 写入信息头.
		std::string info = "<head><meta http-equiv=\"Content-Type\" content=\"text/plain; charset=UTF-8\">\n";
		file_ptr->write(info.c_str(), info.length());

		return file_ptr;
	}

	// 得到当前时间字符串, 对应printf格式: "%04d-%02d-%02d %02d:%02d:%02d"
	std::string current_time() const
	{
		std::ostringstream oss;
		boost::posix_time::time_facet* _facet = new boost::posix_time::time_facet("%Y-%m-%d %H:%M:%S%F");
		oss.imbue(std::locale(std::locale::classic(), _facet));
		oss << boost::posix_time::second_clock::local_time();
		std::string time = oss.str();
		return time;
	}

private:
	boost::mutex m_mutex;
	loglist m_group_list;
	fs::wpath m_path;
};


static qqlog logfile;
static bool resend_img = true;

static std::string progname;

/*
 * 用来配置是否是在一个组里的，一个组里的群和irc频道相互转发.
 */
static std::vector< std::vector<std::string> > channelgroups;

static bool in_group(const std::vector<std::string> & group, std::string id)
{
	BOOST_FOREACH(const std::string & i ,  group)
	{
		if (i == id )
			return true;
	}
	return false;
}

/*
 * 查找同组的其他聊天室和qq群.
 */
static std::vector<std::string> & find_group(std::string id)
{
	static std::vector<std::string> empty;
	BOOST_FOREACH(std::vector<std::string> & g ,  channelgroups)
	{
		if (in_group(g, id))
			return g;
	}
	return empty;
}

static bool same_group(std::string id1, std::string id2)
{
	std::vector< std::string > g1 = find_group(id1);
	if (g1.empty())
		return false;
	return in_group(g1, id2);
}

//从命令行或者配置文件读取组信息.
static void build_group(std::string chanelmapstring)
{
	std::vector<std::string> gs;
	boost::split(gs, chanelmapstring, boost::is_any_of(";"));
	BOOST_FOREACH(std::string  pergroup, gs)
	{
		std::vector<std::string> group;
	 	boost::split(group, pergroup, boost::is_any_of(","));
	 	channelgroups.push_back(group);
	}
}


// 简单的消息命令控制.
static void qqbot_control(const std::string msg)
{
	boost::regex ex("(.*)?说：(.*)");
	boost::cmatch what;

	if(boost::regex_match(msg.c_str(), what, ex))
	{
		std::string name = what[1];
		if (name == "水手(Jack)" || name == "Cai==天马博士")
		{
			std::string cmd = what[2];
			boost::trim(cmd);
			if (cmd == ".stop resend img")
			{
				resend_img = false;
			}
			else if (cmd == ".start resend img")
			{
				resend_img = true;
			}
		}
	}
}

static void qq_msg_sended(const boost::system::error_code& ec)
{
	
}

static void irc_message_got(const IrcMsg pMsg,  webqq & qqclient)
{
	std::cout <<  pMsg.msg<< std::endl;

	std::string from = std::string("irc:") + pMsg.from.substr(1);
	
	BOOST_FOREACH(std::string groupmember, find_group(from))
	{
		if (groupmember != from){
			if (groupmember[0]=='q' && groupmember[1]=='q')
			{
				qqGroup* group = qqclient.get_Group_by_qq(utf8_wide(groupmember.substr(3)));
				if (group){
					std::string forwarder = boost::str(boost::format("%s 说：%s") % pMsg.whom % pMsg.msg);
					qqclient.send_group_message(*group, forwarder , qq_msg_sended);
					// log into
					logfile.add_log(group->qqnum, std::string("[irc]") + forwarder );
				}
			}else if (groupmember[0]=='i' && groupmember[1]=='r'&&groupmember[1]=='c'){
				//TODO, irc频道之间转发.
				;
			}
		}
	}
}

static void on_group_msg(std::wstring group_code, std::wstring who, const std::vector<qqMsg> & msg, webqq & qqclient, IrcClient & ircclient)
{
	qqBuddy * buddy = NULL;
	qqGroup * group = qqclient.get_Group_by_gid(group_code);
	std::wstring	groupname = group_code;
	if (group)
		groupname = group->name;
	buddy = group? group->get_Buddy_by_uin(who):NULL;
	std::wstring nick = who;
	if (buddy)
		nick = buddy->nick;
		
	std::wstring message;
	std::string ircmsg;

	message += nick;
	message += L" 说：";
	
	ircmsg = boost::str(boost::format("qq(%s): ") % wide_utf8(nick));

	BOOST_FOREACH(qqMsg qqmsg, msg)
	{
		std::wstring buf;
		switch (qqmsg.type)
		{
			case qqMsg::LWQQ_MSG_TEXT:
			{
				buf = qqmsg.text;
				ircmsg += wide_utf8(buf);
				if (!buf.empty()) {
					boost::replace_all(buf, L"&", L"&amp;");
					boost::replace_all(buf, L"<", L"&lt;");
					boost::replace_all(buf, L">", L"&gt;");
					boost::replace_all(buf, L"  ", L"&nbsp;");
				}
			}
			break;
			case qqMsg::LWQQ_MSG_CFACE:			
			{
				buf = boost::str(boost::wformat(
				L"<img src=\"http://w.qq.com/cgi-bin/get_group_pic?pic=%s\" > ")
				% qqmsg.cface);
				std::string imgurl = boost::str(boost::format("http://w.qq.com/cgi-bin/get_group_pic?pic=%s")				% wide_utf8(qqmsg.cface));
				if (resend_img){
					//TODO send it
					qqclient.send_group_message(group_code, imgurl, qq_msg_sended);
				}
				ircmsg += imgurl;
			}break;
			case qqMsg::LWQQ_MSG_FACE:
			{
				buf = boost::str(boost::wformat(
					L"<img src=\"http://0.web.qstatic.com/webqqpic/style/face/%d.gif\" >") % qqmsg.face);
				ircmsg += wide_utf8(buf);
			}break;
		}
		message += buf;
	}
	// 记录.
	printf("%ls\n", message.c_str());
	// qq消息控制.	
	qqbot_control(wide_utf8(message));

	if (group)
		logfile.add_log(group->qqnum, wide_utf8(message));
	// send to irc
	if (group)
	{
		std::string from = std::string("qq:") + wide_utf8(group->qqnum);

	 	BOOST_FOREACH(std::string groupmember, find_group(from))
		{
			if (groupmember != from){
				if (groupmember[0]=='i' && groupmember[1]=='r'&&groupmember[2]=='c'){
					ircclient.chat(std::string("#") + groupmember.substr(4), ircmsg);
				}
			}
		}
	}
}

fs::path configfilepath()
{
	if (fs::exists(fs::path(progname) / "qqbotrc"))
		return fs::path(progname) / "qqbotrc";
	if (getenv("USERPROFILE"))
	{
		if (fs::exists(fs::path(getenv("USERPROFILE")) / ".qqbotrc"))
			return fs::path(getenv("USERPROFILE")) / ".qqbotrc";
	}
	if (getenv("HOME")){
		if (fs::exists(fs::path(getenv("HOME")) / ".qqbotrc"))
			return fs::path(getenv("HOME")) / ".qqbotrc";
	}
        if (fs::exists("./qqbotrc/.qqbotrc"))
                return fs::path("./qqbotrc/.qqbotrc");
	if (fs::exists("/etc/qqbotrc"))
		return fs::path("/etc/qqbotrc");
	throw "not configfileexit";
}

int main(int argc, char *argv[])
{
    std::string qqnumber, password;
    std::string ircnick, ircroom;
    std::string cfgfile;
	std::string logdir;
	std::string chanelmap;

    bool isdaemon=false;

    progname = fs::basename(argv[0]);

    std::setlocale(LC_ALL, "");

	po::options_description desc("qqbot options");
	desc.add_options()
	    ( "version,v", "output version" )
		( "help,h", "produce help message" )
        ( "user,u", po::value<std::string>(&qqnumber), "QQ 号" )
		( "pwd,p", po::value<std::string>(&password), "password" )
		( "logdir", po::value<std::string>(&logdir), "dir for logfile" )
		( "daemon,d", po::value<bool>(&isdaemon), "go to background" )
		( "nick", po::value<std::string>(&ircnick), "irc nick" )
		( "room", po::value<std::string>(&ircroom), "irc room" )
		( "map", po::value<std::string>(&chanelmap), "map qqgroup to irc channel. eg: --map:qq:12345,irc:avplayer;qq:56789,irc:ubuntu-cn" )
		;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help"))
	{
		std::cerr <<  desc <<  std::endl;
		return 1;
	}
	if (vm.size() ==0 ){
		po::store(po::parse_config_file<char>(configfilepath().c_str(), desc), vm);
		po::notify(vm);
	}
	if (vm.count("version"))
	{
		printf("qqbot version %s \n", QQBOT_VERSION);
	}
	
	if (!logdir.empty()){
		if (!fs::exists(logdir))
			fs::create_directory(logdir);
	}

	// 设置日志自动记录目录.
	if (! logdir.empty())
		logfile.log_path(logdir);

	build_group(chanelmap);

    if (isdaemon)
		daemon(0, 0);

	boost::asio::io_service asio;

	webqq qqclient(asio, qqnumber, password);

	IrcClient ircclient(asio, ircnick);
	ircclient.login(boost::bind(&irc_message_got, _1, boost::ref(qqclient)));
	ircclient.join(std::string("#") + ircroom);

	qqclient.start();
	qqclient.on_group_msg(boost::bind(on_group_msg, _1, _2, _3, boost::ref(qqclient), boost::ref(ircclient)));

    boost::asio::io_service::work work(asio);
    asio.run();
    return 0;
}
