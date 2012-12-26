/*
 * Copyright (C) 2012  微蔡 <microcai@fedoraproject.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef WEBQQ_IMPL_H
#define WEBQQ_IMPL_H

#include <string>
#include <map>
#include <boost/asio.hpp>
#include <boost/signal.hpp>
#include <boost/concept_check.hpp>
#include <urdl/read_stream.hpp>
#include <boost/property_tree/ptree.hpp>
namespace pt = boost::property_tree;

typedef boost::shared_ptr<urdl::read_stream> read_streamptr;

#include "webqq.h"

namespace qq{

typedef enum LwqqMsgType {
    LWQQ_MT_BUDDY_MSG = 0,
    LWQQ_MT_GROUP_MSG,
    LWQQ_MT_DISCU_MSG,
    LWQQ_MT_SESS_MSG, //group whisper message
    LWQQ_MT_STATUS_CHANGE,
    LWQQ_MT_KICK_MESSAGE,
    LWQQ_MT_SYSTEM,
    LWQQ_MT_BLIST_CHANGE,
    LWQQ_MT_SYS_G_MSG,
    LWQQ_MT_OFFFILE,
    LWQQ_MT_FILETRANS,
    LWQQ_MT_FILE_MSG,
    LWQQ_MT_NOTIFY_OFFFILE,
    LWQQ_MT_INPUT_NOTIFY,
    LWQQ_MT_UNKNOWN,
} LwqqMsgType;

typedef enum {
    LWQQ_MC_OK = 0,
    LWQQ_MC_TOO_FAST = 108,             //< send message too fast
    LWQQ_MC_LOST_CONN = 121
}LwqqMsgRetcode;

// 好友
class Buddy{
	
};

// 群成员啊，临时聊天之类的
class SimpleBuddy{
	
};

typedef struct LwqqVerifyCode {
    std::string str;
    std::string type;
    std::string img;
    std::string uin;
    std::string data;
    size_t size;
} LwqqVerifyCode;

typedef struct LwqqCookies {
    std::string ptvfsession;          /**< ptvfsession */
    std::string ptcz;
    std::string skey;
    std::string ptwebqq;
    std::string ptuserinfo;
    std::string uin;
    std::string ptisp;
    std::string pt2gguin;
    std::string verifysession;
    std::string lwcookies;
} LwqqCookies;

class WebQQ
{
public:
	WebQQ(boost::asio::io_service & asioservice, std::string qqnum, std::string passwd, LWQQ_STATUS status = LWQQ_STATUS_ONLINE);

	void send_simple_message( );
	void update_group_list();
    void update_group_member(qqGroup &  group);

public:// signals
	// 登录成功激发.
	boost::signal< void ()> siglogin;
	// 验证码, 需要返回验证码。图片内容在 const_buffer
	boost::signal< std::string ( boost::asio::const_buffer )> signeedvc;
	// 断线的时候激发.
	boost::signal< void ()> sigoffline;

	// 发生错误的时候激发, 返回 false 停止登录，停止发送，等等操作。true则重试.
	boost::signal< bool (int stage, int why)> sigerror;
	
	// 有群消息的时候激发.
	boost::signal< void (std::string group, std::string who, const pt::wptree & )> siggroupmessage;
	static std::string lwqq_status_to_str(LWQQ_STATUS status);

private:
	void cb_get_version(read_streamptr stream, const boost::system::error_code& ec);
	void cb_got_version(char * response, const boost::system::error_code& ec, std::size_t length);

	void cb_get_vc(read_streamptr stream, const boost::system::error_code& ec);
	void cb_got_vc(read_streamptr stream, char* response, const boost::system::error_code& ec, std::size_t length);

	void cb_do_login(read_streamptr stream, const boost::system::error_code& ec);
	void cb_done_login(read_streamptr stream, char * response, const boost::system::error_code& ec, std::size_t length);

	//last step for login
	void set_online_status();
	void cb_online_status(read_streamptr stream, const boost::system::error_code& ec);
	void cb_online_status(read_streamptr stream, char * response, const boost::system::error_code& ec, std::size_t length);


	void do_poll_one_msg();
	void cb_poll_msg(read_streamptr stream, const boost::system::error_code& ec);
	void cb_poll_msg(read_streamptr stream, char * response, const boost::system::error_code& ec, std::size_t length, size_t goten);

	void process_msg(pt::wptree jstree);

	void cb_group_list(read_streamptr stream, const boost::system::error_code& ec);
	void cb_group_list(read_streamptr stream, char * response, const boost::system::error_code& ec, std::size_t length, size_t goten);

	void cb_group_member(qqGroup &, read_streamptr stream, const boost::system::error_code& ec);
	void cb_group_member(qqGroup &,read_streamptr stream, char * response, const boost::system::error_code& ec, std::size_t length, size_t goten);

private:
    boost::asio::io_service & io_service;

	std::string qqnum, passwd;
    LWQQ_STATUS status;

	std::string	version;
	std::string clientid, psessionid, vfwebqq;

	LwqqVerifyCode vc;
	LwqqCookies cookies;

	typedef std::map<std::string, qqGroup>	grouplist;
	grouplist	groups;
};

};
#endif // WEBQQ_H
