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

#include "webqq.h"
#include "webqq_impl.h"

webqq::webqq(boost::asio::io_service& asioservice, std::string qqnum, std::string passwd, LWQQ_STATUS status)
  : impl(new qq::WebQQ(asioservice, qqnum, passwd, status))
{
	
}

void webqq::on_group_msg(boost::function< void(std::string, std::string, const pt::wptree & )> cb)
{
	this->impl->siggroupmessage.connect(cb);
}

qqGroup * webqq::get_Group_by_gid(std::string gid)
{
	qq::WebQQ::grouplist::iterator it = impl->groups.find(gid);
	if (it != impl->groups.end())
		return & it->second;
	return NULL;
}


