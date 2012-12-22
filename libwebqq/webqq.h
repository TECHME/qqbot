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

#ifndef WEBQQ_H
#define WEBQQ_H

#include <string>
#include <map>
#include <boost/asio.hpp>
#include <boost/signal.hpp>
#include <boost/concept_check.hpp>
#include <urdl/read_stream.hpp>

typedef boost::shared_ptr<urdl::read_stream> read_streamptr;

namespace qq{

typedef enum LWQQ_STATUS{
    LWQQ_STATUS_UNKNOW = 0,
    LWQQ_STATUS_ONLINE = 10,
    LWQQ_STATUS_OFFLINE = 20,
    LWQQ_STATUS_AWAY = 30,
    LWQQ_STATUS_HIDDEN = 40,
    LWQQ_STATUS_BUSY = 50,
    LWQQ_STATUS_CALLME = 60,
    LWQQ_STATUS_SLIENT = 70
}LWQQ_STATUS;

struct message{
	std::string content;
};

// 好友
class Buddy{
	
};

// 群成员啊，临时聊天之类的
class SimpleBuddy{
	
};

// 群
class Group{
	

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

class webqq
{
public:
	webqq(boost::asio::io_service & asioservice, std::string qqnum, std::string passwd, LWQQ_STATUS status = LWQQ_STATUS_ONLINE);

	void send_simple_message( );

public:// signals
	//登录成功激发
	boost::signal< void ()> siglogin;
	//验证码, 需要返回验证码。图片内容在 const_buffer
	boost::signal< std::string ( boost::asio::const_buffer )> signeedvc;
	//断线的时候激发
	boost::signal< void ()> sigoffline;

	//发生错误的时候激发, 返回 false 停止登录，停止发送，等等操作。true则重试
	boost::signal< bool (int stage, int why)> sigerror;
	
	//有群消息的时候激发
	boost::signal< void ( qq::message)> siggroupmessage;

private:
	void cb_get_version(read_streamptr stream, const boost::system::error_code& ec);
	void cb_got_version(char * response, const boost::system::error_code& ec, std::size_t length);

	void cb_get_vc(read_streamptr stream, const boost::system::error_code& ec);
	void cb_got_vc(read_streamptr stream, char* response, const boost::system::error_code& ec, std::size_t length);

	void cb_do_login(read_streamptr stream, const boost::system::error_code& ec);
	void cb_done_login(read_streamptr stream, char * response, const boost::system::error_code& ec, std::size_t length);
	
	void do_poll_one_msg();
	void cb_poll_msg(read_streamptr stream, const boost::system::error_code& ec);
	void cb_poll_msg(char * response, const boost::system::error_code& ec, std::size_t length);

private:
    boost::asio::io_service & io_service;

	std::string qqnum, passwd;
    LWQQ_STATUS status;

	std::string	version;
	std::string clientid, psessionid;

	LwqqVerifyCode vc;
	LwqqCookies cookies;
};

};
#endif // WEBQQ_H
