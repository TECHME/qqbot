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

// 群.
struct qqGroup{
    enum{
        LWQQ_GROUP_QUN,
        LWQQ_GROUP_DISCU,
    }type;
	// 群ID, 不是群的QQ号，每次登录都变化的.
	std::string gid;
	// 群名字.
	std::string name;

	// 群代码，可以用来获得群QQ号.
	std::string code;

	qqGroup(){type = LWQQ_GROUP_QUN;}
};

class webqq
{
public:
	webqq(boost::asio::io_service & asioservice, std::string qqnum, std::string passwd, LWQQ_STATUS status = LWQQ_STATUS_ONLINE);
	void on_group_msg(boost::function<void (std::string group_code, std::string who, const pt::wptree & )> cb);
private:
    class qq::WebQQ * const impl;
};

#endif // WEBQQ_H
