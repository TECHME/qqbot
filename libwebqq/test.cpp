#include <clocale>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/json_parser.hpp>
namespace js = boost::property_tree::json_parser;
#include "webqq.h"

static void on_group_msg(std::string group_code, std::string who, const pt::wptree & msg, webqq & qqclient)
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

	std::cout <<  "(群 :" <<  groupname <<  "), " << nick <<  "说：" ;
	
	BOOST_FOREACH(pt::wptree::value_type content, msg.get_child(L"content"))
	{
		if (content.second.count(L"")==0)
			std::wcout <<  content.second.data();
			
// 		std::wcout <<  content.second.get<std::wstring>(L"") <<  std::endl;
	}
	std::cout <<  std::endl;
// 	js::write_json(std::wcout, msg);
}

int main()
{
	std::setlocale(LC_ALL, "");
	boost::asio::io_service io_service;
	webqq qqclient(io_service, "2664046919", "jackbotwgm123");

	qqclient.on_group_msg(boost::bind(&on_group_msg, _1, _2, _3, boost::ref(qqclient)));

	io_service.run();
}