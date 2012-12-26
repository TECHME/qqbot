#include <clocale>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include "webqq.h"
#include "logger.h"

static void on_group_msg(std::string group_code, std::string who, const std::vector<qqMsg> & msg, webqq & qqclient)
{
	qqBuddy * buddy = NULL;
	qqGroup * group = qqclient.get_Group_by_gid(group_code);
	std::string	groupname = group_code;
	if (group)
		groupname = group->name;
	buddy = group? group->get_Buddy_by_uin(who):NULL;
	std::string nick = who;
	if (buddy)
		nick = buddy->nick;

	std::wcout <<  L"(群 :" <<  groupname.c_str() <<  L"), " << nick.c_str() <<  L"说：" ;
	std::wcout.flush();

	BOOST_FOREACH(qqMsg qqmsg, msg)
	{
		if (qqmsg.type==qqMsg::LWQQ_MSG_TEXT)
			std::wcout <<  qqmsg.text <<  std::endl;
	}
}

int main()
{
	setlocale(LC_ALL, "");
	boost::asio::io_service io_service;
	webqq qqclient(io_service, "2664046919", "jackbotwgm123");

	qqclient.on_group_msg(boost::bind(&on_group_msg, _1, _2, _3, boost::ref(qqclient)));

	io_service.run();
}