
#include "webqq.h"

int main()
{
	boost::asio::io_service io_service;
	qq::webqq qqclient(io_service, "2664046919", "jackbotwgm123");

	io_service.run();
}