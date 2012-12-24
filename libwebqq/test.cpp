
#include <boost/bind.hpp>
#include "webqq.h"

static void on_group_msg(const pt::ptree & msg, qq::webqq & qqclient)
{
	std::string group_code =  msg.get<std::string>("group_code");
	std::cout <<  group_code <<  std::endl;
}

int main()
{
	try
	{
		boost::asio::io_service ios;
		qq::webqq qqclient(ios, "2664046919", "jackbotwgm123");

		qqclient.siggroupmessage.connect(boost::bind(&on_group_msg, _1, boost::ref(qqclient)));

		ios.run();
	}
	catch (std::exception& e)
	{
		return -1;
	}

	return 0;
}