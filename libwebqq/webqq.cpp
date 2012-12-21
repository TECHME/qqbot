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
#include <boost/bind.hpp>
#include <urdl/read_stream.hpp>
#include <urdl/http.hpp>
#include <boost/format.hpp>

#include "webqq.h"
#include "defer.hpp"

extern "C"{
#include "logger.h"
};

using namespace qq;

/* URL for webqq login */
#define LWQQ_URL_LOGIN_HOST "http://ptlogin2.qq.com"
#define LWQQ_URL_CHECK_HOST "http://check.ptlogin2.qq.com"
#define LWQQ_URL_VERIFY_IMG "http://captcha.qq.com/getimage?aid=%s&uin=%s"
#define VCCHECKPATH "/check"
#define APPID "1003903"
#define LWQQ_URL_SET_STATUS "http://d.web2.qq.com/channel/login2"
/* URL for get webqq version */
#define LWQQ_URL_VERSION "http://ui.ptlogin2.qq.com/cgi-bin/ver"

// ptui_checkVC('0','!IJG, ptui_checkVC('0','!IJG', '\x00\x00\x00\x00\x54\xb3\x3c\x53');
static std::string parse_verify_uin(const char *str)
{
    char *start;
    char *end;

    start = strchr(str, '\\');
    if (!start)
        return "";

    end = strchr(start, '\'');
    if (!end)
        return "";

    return std::string(start, end - start);
}

static std::string get_cookie(std::string httpheader,std::string key)
{
	std::string searchkey = key + "=";
	std::string::size_type keyindex = httpheader.find(searchkey);
	if (keyindex == std::string::npos)
		return "";
	keyindex +=searchkey.length();
	std::string::size_type valend = httpheader.find("; ", keyindex);
	return httpheader.substr(keyindex , valend-keyindex);
}

static void update_cookies(LwqqCookies *cookies, std::string httpheader,
                           std::string key, int update_cache)
{
	std::string value = get_cookie(httpheader, key);
    if (value.empty())
        return ;
    
#define FREE_AND_STRDUP(a, b)    a = b
    
    if (key ==  "ptvfsession") {
        FREE_AND_STRDUP(cookies->ptvfsession, value);
    } else if ((key== "ptcz")) {
        FREE_AND_STRDUP(cookies->ptcz, value);
    } else if ((key== "skey")) {
        FREE_AND_STRDUP(cookies->skey, value);
    } else if ((key== "ptwebqq")) {
        FREE_AND_STRDUP(cookies->ptwebqq, value);
    } else if ((key== "ptuserinfo")) {
        FREE_AND_STRDUP(cookies->ptuserinfo, value);
    } else if ((key== "uin")) {
        FREE_AND_STRDUP(cookies->uin, value);
    } else if ((key== "ptisp")) {
        FREE_AND_STRDUP(cookies->ptisp, value);
    } else if ((key== "pt2gguin")) {
        FREE_AND_STRDUP(cookies->pt2gguin, value);
    } else if ((key== "verifysession")) {
        FREE_AND_STRDUP(cookies->verifysession, value);
    } else {
        lwqq_log(LOG_WARNING, "No this cookie: %s\n", key.c_str());
    }
#undef FREE_AND_STRDUP

    if (update_cache) {
		
		cookies->lwcookies.clear();

        if (cookies->ptvfsession.length()) {
            cookies->lwcookies += "ptvfsession="+cookies->ptvfsession+"; ";
        }
        if (cookies->ptcz.length()) {
			cookies->lwcookies += "ptcz="+cookies->ptcz+"; ";
        }
        if (cookies->skey.length()) {
            cookies->lwcookies += "skey="+cookies->skey+"; ";
        }
        if (cookies->ptwebqq.length()) {
            cookies->lwcookies += "ptwebqq="+cookies->ptwebqq+"; ";
        }
        if (cookies->ptuserinfo.length()) {
			cookies->lwcookies += "ptuserinfo="+cookies->ptuserinfo+"; ";
        }
        if (cookies->uin.length()) {
            cookies->lwcookies += "uin="+cookies->uin+"; ";
        }
        if (cookies->ptisp.length()) {
            cookies->lwcookies += "ptisp="+cookies->ptisp+"; ";
        }
        if (cookies->pt2gguin.length()) {
			cookies->lwcookies += "pt2gguin="+cookies->pt2gguin+"; ";
        }
        if (cookies->verifysession.length()) {
			cookies->lwcookies += "verifysession="+cookies->verifysession+"; ";
        }
    }
}

// build webqq and setup defaults
webqq::webqq(boost::asio::io_service& _io_service,
	std::string _qqnum, std::string _passwd, LWQQ_STATUS status)
	:io_service(_io_service), qqnum(_qqnum), passwd(_passwd)
{
	//开始登录!

	//首先获得版本.
	read_streamptr stream(new urdl::read_stream(io_service));
    lwqq_log(LOG_DEBUG, "Get webqq version from %s\n", LWQQ_URL_VERSION);

	stream->async_open(LWQQ_URL_VERSION, boost::bind(&webqq::cb_get_version, this, stream,  boost::asio::placeholders::error()) );	
}

void webqq::cb_got_version(char* response, const boost::system::error_code& ec, std::size_t length)
{
	defer(boost::bind(operator delete, response));
    if (strstr(response, "ptuiV"))
	{
        char *s, *t;
        s = strchr(response, '(');
        t = strchr(response, ')');
        if (!s || !t  ) {
            sigerror( 0, 0);
            return ;
        }
        s++;
        char v[t - s + 1];
        memset(v, 0, t - s + 1);
        strncpy(v, s, t - s);
        this->version = v;
        //开始真正的登录
        std::cout << "Get webqq version: " <<  this->version <<  std::endl;

        //获取验证码
        std::string url = 
			boost::str(boost::format("%s%s?uin=%s&appid=%s") % LWQQ_URL_CHECK_HOST % VCCHECKPATH % qqnum % APPID);
        std::string cookie = 
			boost::str(boost::format("chkuin=%s") % qqnum);
		read_streamptr stream(new urdl::read_stream(io_service));

		stream->set_option(urdl::http::cookie(cookie));
		stream->async_open(url,boost::bind(&webqq::cb_get_vc,this, stream, boost::asio::placeholders::error()) );
	}
}

void webqq::cb_got_vc(read_streamptr stream, char* response, const boost::system::error_code& ec, std::size_t length)
{
	/**
	* 
	* The http message body has two format:
	*
	* ptui_checkVC('1','9ed32e3f644d968809e8cbeaaf2cce42de62dfee12c14b74');
	* ptui_checkVC('0','!LOB');
	* The former means we need verify code image and the second
	* parameter is vc_type.
	* The later means we don't need the verify code image. The second
	* parameter is the verify code. The vc_type is in the header
	* "Set-Cookie".
	*/
	
	char *s;
	char *c = strstr(response, "ptui_checkVC");
    c = strchr(response, '\'');
    c++;
    if (*c == '0') {
        /* We got the verify code. */
        
        /* Parse uin first */
        vc.uin = parse_verify_uin(response);
        if (vc.uin.empty())
		{
			sigerror(1, 0);
			return;
		}

        s = c;
        c = strstr(s, "'");
        s = c + 1;
        c = strstr(s, "'");
        s = c + 1;
        c = strstr(s, "'");
        *c = '\0';

        vc.type = "0";
        vc.str = s;

        /* We need get the ptvfsession from the header "Set-Cookie" */
        update_cookies(&cookies, stream->get_httpheader(), "ptvfsession", 1);
        lwqq_log(LOG_NOTICE, "Verify code: %s\n", vc.str.c_str());
    } else if (*c == '1') {
        /* We need get the verify image. */

        /* Parse uin first */
        vc.uin = parse_verify_uin(response);
        s = c;
        c = strstr(s, "'");
        s = c + 1;
        c = strstr(s, "'");
        s = c + 1;
        c = strstr(s, "'");
        *c = '\0';
        vc.type = "1";
        // ptui_checkVC('1','7ea19f6d3d2794eb4184c9ae860babf3b9c61441520c6df0', '\x00\x00\x00\x00\x04\x7e\x73\xb2');
        vc.str = s;
        
        //signeedvc();

        lwqq_log(LOG_NOTICE, "We need verify code image: %s\n", vc.str.c_str());
    }
    

}


void webqq::cb_get_version(read_streamptr stream, const boost::system::error_code& ec)
{
	char * data = new char[1024];
	stream->async_read_some(boost::asio::buffer(data, 1024),
		boost::bind(&webqq::cb_got_version, this, data, boost::asio::placeholders::error() ,  boost::asio::placeholders::bytes_transferred()) );
}

void webqq::cb_get_vc(read_streamptr stream, const boost::system::error_code& ec)
{
	char * data = new char[1024];
	stream->async_read_some(boost::asio::buffer(data, 1024),
		boost::bind(&webqq::cb_got_vc, this,stream, data, boost::asio::placeholders::error() ,  boost::asio::placeholders::bytes_transferred()) );
}
