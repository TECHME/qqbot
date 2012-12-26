#include <clocale>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include "webqq.h"
#include "logger.h"

static void on_group_msg(std::wstring group_code, std::wstring who, const std::vector<qqMsg> & msg, webqq & qqclient)
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

	std::wcout <<  L"(群 :";
	std::wcout <<  groupname;
	std::wcout <<  L"), ";
	std::wcout << nick;
	std::wcout <<  L"说：";

	BOOST_FOREACH(qqMsg qqmsg, msg)
	{
		if (qqmsg.type == qqMsg::LWQQ_MSG_TEXT)
			printf("%ls\n", qqmsg.text.c_str());
	}
}

int main()
{
	setlocale(LC_ALL, "");
	boost::asio::io_service io_service;
	webqq qqclient(io_service, "2664046919", "jackbotwgm123");

	qqclient.start();
	qqclient.on_group_msg(boost::bind(&on_group_msg, _1, _2, _3, boost::ref(qqclient)));

	io_service.run();
}