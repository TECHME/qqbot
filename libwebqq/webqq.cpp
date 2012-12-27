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

void webqq::on_group_msg(boost::function< void(std::wstring, std::wstring, const std::vector<qqMsg>& )> cb)
{
	this->impl->siggroupmessage.connect(cb);
}

qqGroup * webqq::get_Group_by_gid(std::wstring gid)
{
	qq::grouplist::iterator it = impl->m_groups.find(gid);
	if (it != impl->m_groups.end())
		return & it->second;
	return NULL;
}

qqGroup* webqq::get_Group_by_qq(std::wstring qq)
{
	qq::grouplist::iterator it = impl->m_groups.begin();
	for(;it != impl->m_groups.end();it ++){
		if ( it->second.qqnum == qq)
			return & it->second;
	}
	return NULL;
}

void webqq::start()
{
	impl->start();
}

void webqq::send_group_message(std::wstring group, std::wstring msg, boost::function<void (const boost::system::error_code& ec)> donecb)
{
	impl->send_group_message(group, msg, donecb);
}

void webqq::send_group_message(qqGroup& group, std::wstring msg, boost::function<void (const boost::system::error_code& ec)> donecb)
{
	impl->send_group_message(group, msg, donecb);
}
