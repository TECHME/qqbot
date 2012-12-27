/*
 * <one line to give the program's name and a brief idea of what it does.>
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
#include <vector>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/property_tree/ptree.hpp>

namespace pt = boost::property_tree;

namespace qq{
class WebQQ;
};

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

struct qqBuddy{
	// 号码，每次登录都变化的.
	std::wstring uin;

	// 昵称.
	std::wstring nick;

	// qq号码，不一定有，需要调用 get_qqnumber后才有.
	std::wstring qqnum;
};

// 群.
struct qqGroup{
	// 群ID, 不是群的QQ号，每次登录都变化的.
	std::wstring gid;
	// 群名字.
	std::wstring name;

	// 群代码，可以用来获得群QQ号.
	std::wstring code;
	// 群QQ号.
	std::wstring qqnum;

	std::map<std::wstring, qqBuddy>	memberlist;

	qqBuddy * get_Buddy_by_uin(std::wstring uin){
		std::map<std::wstring, qqBuddy>::iterator it = memberlist.find(uin);
		if (it!=memberlist.end())
			return &it->second;
		return NULL;
	}
};

struct qqMsg{
	enum {
		LWQQ_MSG_FONT, 
		LWQQ_MSG_TEXT, 
		LWQQ_MSG_FACE, 
		LWQQ_MSG_CFACE, 
	}type;
 	std::wstring font;//font name, size color.
	std::wstring text;
	std::wstring face;
	std::wstring cface;
};

class webqq
{
public:
	webqq(boost::asio::io_service & asioservice, std::string qqnum, std::string passwd, LWQQ_STATUS status = LWQQ_STATUS_ONLINE);
	void on_group_msg(boost::function<void (std::wstring group_code, std::wstring who, const std::vector<qqMsg> & )> cb);

	void start();

	void send_group_message(std::wstring group, std::string msg, boost::function<void (const boost::system::error_code& ec)> donecb);
	void send_group_message(qqGroup &  group, std::string msg, boost::function<void (const boost::system::error_code& ec)> donecb);
	
	qqGroup * get_Group_by_gid(std::wstring);
	qqGroup * get_Group_by_qq(std::wstring qq);
private:
    class qq::WebQQ * const impl;
};

#endif // WEBQQ_H
