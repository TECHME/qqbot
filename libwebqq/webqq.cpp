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
#include <boost/foreach.hpp>
#include <boost/property_tree/json_parser.hpp>
namespace js = boost::property_tree::json_parser;

#include "webqq.h"
#include "defer.hpp"
#include "url.hpp"

extern "C"{
#include "logger.h"
#include "md5.h"
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

static unsigned int hex2char(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }

    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }

    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }

    return 0;
}

/** 
 * decode UCS-4 to utf8
 *      UCS-4 code              UTF-8
 * U+00000000–U+0000007F 0xxxxxxx
 * U+00000080–U+000007FF 110xxxxx 10xxxxxx
 * U+00000800–U+0000FFFF 1110xxxx 10xxxxxx 10xxxxxx
 * U+00010000–U+001FFFFF 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 * U+00200000–U+03FFFFFF 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 * U+04000000–U+7FFFFFFF 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 * 
 * Chinese are all in  U+00000800–U+0000FFFF.
 * So only decode to the second and third format.
 * 
 * @param to 
 * @param from 
 */
static std::string do_ucs4toutf8(const char *from)
{
    char str[5] = {0};
    int i;
    
    static unsigned int E = 0xe0;
    static unsigned int T = 0x02;
    static unsigned int sep1 = 0x80;
    static unsigned int sep2 = 0x800;
    static unsigned int sep3 = 0x10000;
    static unsigned int sep4 = 0x200000;
    static unsigned int sep5 = 0x4000000;
        
    unsigned int tmp[4];
    char re[6];
    unsigned int ivalue = 0;
        
    for (i = 0; i < 4; ++i) {
        tmp[i] = 0xf & hex2char(from[i + 2]);
        ivalue *= 16;
        ivalue += tmp[i];
    }
    
    /* decode */
    if (ivalue < sep1) {
        /* 0xxxxxxx */
        re[0] = 0x7f & tmp[3];
        str[0] = re[0];
    } else if (ivalue < sep2) {
        /* 110xxxxx 10xxxxxx */
        re[0] = (0x3 << 6) | ((tmp[1] & 7) << 2) | (tmp[2] >> 2);
        re[1] = (0x1 << 7) | ((tmp[2] & 3) << 4) | tmp[3];
        str[0] = re[0];
        str[1] = re[1];
    } else if (ivalue < sep3) {
        /* 1110xxxx 10xxxxxx 10xxxxxx */
        re[0] = E | (tmp[0] & 0xf);
        re[1] = (T << 6) | (tmp[1] << 2) | ((tmp[2] >> 2) & 0x3);
        re[2] = (T << 6) | (tmp[2] & 0x3) << 4 | tmp[3];
        /* copy to @to. */
        for (i = 0; i < 3; ++i){
            str[i] = re[i];
        }
    } else if (ivalue < sep4) {
        /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
        re[0] = 0xf0 | ((tmp[1] >> 2) & 0x7);
        re[1] = 0x2 << 6 | ((tmp[1] & 0x3) << 4) | ((tmp[2] >> 4) & 0xf);
        re[2] = 0x2 << 6 | ((tmp[2] & 0xf) << 4) | ((tmp[3] >> 6) & 0x3);
        re[3] = 0x2 << 6 | (tmp[2] & 0x3f);
        /* copy to @to. */
        for (i = 0; i < 4; ++i) {
            str[i] = re[i];
        }
    } else if (ivalue < sep5) {
        /* 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
        return NULL;
    } else {
        /* 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
        return NULL;
    }
    
    return str;
}

static std::string ucs4toutf8(const char *from)
{
    if (!from) {
        return NULL;
    }
    
    std::string out;
    const char *c;
    
    for (c = from; *c != '\0'; ++c) {
        std::string s;
        if (*c == '\\' && *(c + 1) == 'u') {
            out += do_ucs4toutf8(c);
            c += 5;
        } else {
            out += *c ;
            continue;
        }
    }
    return out;
}

static void upcase_string(char *str, int len)
{
    int i;
    for (i = 0; i < len; ++i) {
        if (islower(str[i]))
            str[i]= toupper(str[i]);
    }
}
static std::string generate_clientid()
{
    int r;
    struct timeval tv;
    long t;
    char buf[20] = {0};
    
    srand(time(NULL));
    r = rand() % 90 + 10;
    if (gettimeofday(&tv, NULL)) {
        return NULL;
    }
    t = tv.tv_usec % 1000000;
    snprintf(buf, sizeof(buf), "%d%ld", r, t);
    return buf;
}

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

static void sava_cookie(LwqqCookies * cookies,std::string  httpheader)
{
	update_cookies(cookies, httpheader, "ptcz", 0);
    update_cookies(cookies, httpheader, "skey",  0);
    update_cookies(cookies, httpheader, "ptwebqq", 0);
    update_cookies(cookies, httpheader, "ptuserinfo", 0);
    update_cookies(cookies, httpheader, "uin", 0);
    update_cookies(cookies, httpheader, "ptisp", 0);
    update_cookies(cookies, httpheader, "pt2gguin", 1);
}


/**
 * I hacked the javascript file named comm.js, which received from tencent
 * server, and find that fuck tencent has changed encryption algorithm
 * for password in webqq3 . The new algorithm is below(descripted with javascript):
 * var M=C.p.value; // M is the qq password 
 * var I=hexchar2bin(md5(M)); // Make a md5 digest
 * var H=md5(I+pt.uin); // Make md5 with I and uin(see below)
 * var G=md5(H+C.verifycode.value.toUpperCase());
 * 
 * @param pwd User's password
 * @param vc Verify Code. e.g. "!M6C"
 * @param uin A string like "\x00\x00\x00\x00\x54\xb3\x3c\x53", NB: it
 *        must contain 8 hexadecimal number, in this example, it equaled
 *        to "0x0,0x0,0x0,0x0,0x54,0xb3,0x3c,0x53"
 * 
 * @return Encoded password on success, else NULL on failed
 */
static std::string lwqq_enc_pwd(const char *pwd, const char *vc, const char *uin)
{
    int i;
    int uin_byte_length;
    char buf[128] = {0};
    char _uin[9] = {0};

    /* Calculate the length of uin (it must be 8?) */
    uin_byte_length = strlen(uin) / 4;

    /**
     * Ok, parse uin from string format.
     * "\x00\x00\x00\x00\x54\xb3\x3c\x53" -> {0,0,0,0,54,b3,3c,53}
     */
    for (i = 0; i < uin_byte_length ; i++) {
        char u[5] = {0};
        char tmp;
        strncpy(u, uin + i * 4 + 2, 2);

        errno = 0;
        tmp = strtol(u, NULL, 16);
        if (errno) {
            return NULL;
        }
        _uin[i] = tmp;
    }

    /* Equal to "var I=hexchar2bin(md5(M));" */
    lutil_md5_digest((unsigned char *)pwd, strlen(pwd), (char *)buf);

    /* Equal to "var H=md5(I+pt.uin);" */
    memcpy(buf + 16, _uin, uin_byte_length);
    lutil_md5_data((unsigned char *)buf, 16 + uin_byte_length, (char *)buf);
    
    /* Equal to var G=md5(H+C.verifycode.value.toUpperCase()); */
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", vc);
    upcase_string(buf, strlen(buf));

    lutil_md5_data((unsigned char *)buf, strlen(buf), (char *)buf);
    upcase_string(buf, strlen(buf));

    /* OK, seems like every is OK */
    lwqq_puts(buf);
    return buf;
}

static pt::ptree json_parse(const char * doc)
{
	pt::ptree jstree;
	std::stringstream stream;
	stream <<  doc ;
	js::read_json(stream, jstree);
	return jstree;
}

static pt::wptree json_parse(const wchar_t * doc)
{
	pt::wptree jstree;
	std::wstringstream stream;
	stream <<  doc ;
	js::read_json(stream, jstree);
	return jstree;
}

// build webqq and setup defaults
webqq::webqq(boost::asio::io_service& _io_service,
	std::string _qqnum, std::string _passwd, LWQQ_STATUS _status)
	:io_service(_io_service), qqnum(_qqnum), passwd(_passwd), status(_status)
{
	//开始登录!

	//首先获得版本.
	read_streamptr stream(new urdl::read_stream(io_service));
    lwqq_log(LOG_DEBUG, "Get webqq version from %s\n", LWQQ_URL_VERSION);

	stream->async_open(LWQQ_URL_VERSION, boost::bind(&webqq::cb_get_version, this, stream,  boost::asio::placeholders::error) );	
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
		stream->async_open(url,boost::bind(&webqq::cb_get_vc,this, stream, boost::asio::placeholders::error) );
	}
}

void webqq::cb_got_vc(read_streamptr stream, char* response, const boost::system::error_code& ec, std::size_t length)
{
	defer(boost::bind(operator delete, response));
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
        update_cookies(&cookies, stream->headers(), "ptvfsession", 1);
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
    std::string md5 = lwqq_enc_pwd(passwd.c_str(), vc.str.c_str(), vc.uin.c_str());

    // do login !
    std::string url = boost::str(
		boost::format(
		    "%s/login?u=%s&p=%s&verifycode=%s&"
             "webqq_type=%d&remember_uin=1&aid=1003903&login2qq=1&"
             "u1=http%%3A%%2F%%2Fweb.qq.com%%2Floginproxy.html"
             "%%3Flogin2qq%%3D1%%26webqq_type%%3D10&h=1&ptredirect=0&"
             "ptlang=2052&from_ui=1&pttype=1&dumy=&fp=loginerroralert&"
             "action=2-11-7438&mibao_css=m_webqq&t=1&g=1") 
             % LWQQ_URL_LOGIN_HOST
             % qqnum
             % md5
             % vc.str
             % status
	);

	read_streamptr loginstream(new urdl::read_stream(io_service));
	loginstream->set_option(urdl::http::cookie(cookies.lwcookies));
	loginstream->async_open(url,boost::bind(&webqq::cb_do_login,this, loginstream, boost::asio::placeholders::error) );

}

void webqq::cb_done_login(read_streamptr stream, char* response, const boost::system::error_code& ec, std::size_t length)
{
	defer(boost::bind(operator delete, response));
    char *p = strstr(response, "\'");
    if (!p) {
        return;
    }
    char buf[4] = {0};
    int status;
    strncpy(buf, p + 1, 1);
    status = atoi(buf);

    switch (status) {
    case 0:
        sava_cookie(&cookies, stream->headers());
        lwqq_log(LOG_NOTICE, "login success!\n");
        break;
        
    case 1:
        lwqq_log(LOG_WARNING, "Server busy! Please try again\n");

        status = LWQQ_STATUS_OFFLINE;
    case 2:
        lwqq_log(LOG_ERROR, "Out of date QQ number\n");
        status = LWQQ_STATUS_OFFLINE;


    case 3:
        lwqq_log(LOG_ERROR, "Wrong password\n");
        status = LWQQ_STATUS_OFFLINE;


    case 4:
        lwqq_log(LOG_ERROR, "Wrong verify code\n");
        status = LWQQ_STATUS_OFFLINE;


    case 5:
        lwqq_log(LOG_ERROR, "Verify failed\n");
        status = LWQQ_STATUS_OFFLINE;


    case 6:
        lwqq_log(LOG_WARNING, "You may need to try login again\n");
        status = LWQQ_STATUS_OFFLINE;

    case 7:
        lwqq_log(LOG_ERROR, "Wrong input\n");
        status = LWQQ_STATUS_OFFLINE;


    case 8:
        lwqq_log(LOG_ERROR, "Too many logins on this IP. Please try again\n");
        status = LWQQ_STATUS_OFFLINE;


    default:
        status = LWQQ_STATUS_OFFLINE;
        lwqq_log(LOG_ERROR, "Unknow error");
    }

    //set_online_status(lc, lwqq_status_to_str(lc->stat), err);
    if (status == LWQQ_STATUS_ONLINE){
		siglogin();
	}
	clientid = generate_clientid();

	//change status,  this is the last step for login
	set_online_status();
}

void webqq::set_online_status()
{
	std::string msg = boost::str(
		boost::format("{\"status\":\"%s\",\"ptwebqq\":\"%s\","
             "\"passwd_sig\":""\"\",\"clientid\":\"%s\""
             ", \"psessionid\":null}")
		% lwqq_status_to_str(status)
		% cookies.ptwebqq
		% clientid
	);

	std::string buf = url_encode(msg.c_str());
	msg = boost::str(boost::format("r=%s") % buf);

    read_streamptr stream(new urdl::read_stream(io_service));
	stream->set_option(urdl::http::cookie(cookies.lwcookies));
	stream->set_option(urdl::http::cookie2("$Version=1"));
	stream->set_option(urdl::http::request_content_type("application/x-www-form-urlencoded"));
	stream->set_option(urdl::http::request_referer("http://d.web2.qq.com/proxy.html?v=20101025002"));
	stream->set_option(urdl::http::request_content(msg));
	stream->set_option(urdl::http::request_method("POST"));

	stream->async_open(LWQQ_URL_SET_STATUS,boost::bind(&webqq::cb_online_status,this, stream, boost::asio::placeholders::error) );
}

void webqq::do_poll_one_msg()
{
    /* Create a POST request */
    std::string url = boost::str(
		boost::format("%s/channel/poll2") % "http://d.web2.qq.com"
	);
	
	std::string msg = boost::str(
		boost::format("{\"clientid\":\"%s\",\"psessionid\":\"%s\"}")
		% clientid 
		% psessionid		
	);

	msg = boost::str(boost::format("r=%s") %  url_encode(msg.c_str()));

    read_streamptr pollstream(new urdl::read_stream(io_service));
	pollstream->set_option(urdl::http::cookie(cookies.lwcookies));
	pollstream->set_option(urdl::http::cookie2("$Version=1"));
	pollstream->set_option(urdl::http::request_content_type("application/x-www-form-urlencoded"));
	pollstream->set_option(urdl::http::request_referer("http://d.web2.qq.com/proxy.html?v=20101025002"));
	pollstream->set_option(urdl::http::request_content(msg));
	pollstream->set_option(urdl::http::request_method("POST"));

	pollstream->async_open(url,boost::bind(&webqq::cb_poll_msg,this, pollstream, boost::asio::placeholders::error) );
}

void webqq::cb_online_status(read_streamptr stream, char* response, const boost::system::error_code& ec, std::size_t length)
{
	defer(boost::bind(operator delete, response));
	//处理!
	try{
		pt::ptree json = json_parse(response);
		js::write_json(std::cout, json);


		if ( json.get<std::string>("retcode") == "0")
		{
			psessionid = json.get_child("result").get<std::string>("psessionid");
			//start polling messages, 2 connections!
			lwqq_log(LOG_DEBUG, "start polling messages\n");
			do_poll_one_msg();
			do_poll_one_msg();
		}
	}catch (const pt::json_parser_error & jserr){
		printf("parse json error :  %s\n", response);
	}catch (const pt::ptree_bad_path & jserr){
		printf("parse bad path error :  %s\n", jserr.what());
	}
}

void webqq::cb_poll_msg(read_streamptr stream, char* response, const boost::system::error_code& ec, std::size_t length, size_t goten)
{
	if (!ec)
	{
		goten += length;
		boost::asio::async_read(*stream, boost::asio::buffer(response + goten, 16384 - goten),
		boost::bind(&webqq::cb_poll_msg, this, stream, response, boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred, goten));
		return ;
	}
	if (ec != boost::asio::error::eof){
		delete response;
		return ;
	}
	goten += length;

	response[goten]=0;
	defer(boost::bind(operator delete, response));
	//开启新的 poll	
	do_poll_one_msg();

	pt::wptree	jsonobj;
	std::wstringstream jsondata;
	jsondata <<  response;

	//处理!
	try{
		pt::json_parser::read_json(jsondata, jsonobj);
		process_msg(jsonobj);
	}catch (const pt::json_parser_error & jserr){
		printf("parse json error :  %s\n", response);
	}
}

void webqq::process_msg(pt::wptree jstree)
{
	//TODO,  在这里解析json数据。
	std::wstring retcode =  jstree.get<std::wstring>(L"retcode");
	try{
		BOOST_FOREACH(pt::wptree::value_type result, jstree.get_child(L"result"))
		{
			std::wstring polltype = result.second.get<std::wstring>(L"poll_type");

			if (polltype == L"group_message")
				siggroupmessage(result.second.get_child(L"value"));
		}
	}
	catch (const pt::ptree_bad_path & badpath){
	 	pt::json_parser::write_json(std::wcout, jstree);
	}
}

void webqq::cb_get_version(read_streamptr stream, const boost::system::error_code& ec)
{
	char * data = new char[8192];
	boost::asio::async_read(*stream, boost::asio::buffer(data, 8192),
		boost::bind(&webqq::cb_got_version, this, data, boost::asio::placeholders::error ,  boost::asio::placeholders::bytes_transferred) );
}

void webqq::cb_get_vc(read_streamptr stream, const boost::system::error_code& ec)
{
	char * data = new char[8192];
	boost::asio::async_read(*stream, boost::asio::buffer(data, 8192),
		boost::bind(&webqq::cb_got_vc, this,stream, data, boost::asio::placeholders::error,  boost::asio::placeholders::bytes_transferred) );
}


void webqq::cb_do_login(read_streamptr stream, const boost::system::error_code& ec)
{
	char * data = new char[8192];
	boost::asio::async_read(*stream, boost::asio::buffer(data, 8192),
		boost::bind(&webqq::cb_done_login, this,stream, data, boost::asio::placeholders::error,  boost::asio::placeholders::bytes_transferred) );
}

void webqq::cb_online_status(read_streamptr stream, const boost::system::error_code& ec)
{
	char * data = new char[8192];
	memset(data, 0, 8192);
	boost::asio::async_read(*stream, boost::asio::buffer(data, 8192),
		boost::bind(&webqq::cb_online_status, this, stream, data, boost::asio::placeholders::error,  boost::asio::placeholders::bytes_transferred) );
}

void webqq::cb_poll_msg(read_streamptr stream, const boost::system::error_code& ec)
{
	char * data = new char[16384];
	boost::asio::async_read(*stream, boost::asio::buffer(data, 16384),
		boost::bind(&webqq::cb_poll_msg, this, stream, data, boost::asio::placeholders::error,  boost::asio::placeholders::bytes_transferred, 0) );
}

std::string webqq::lwqq_status_to_str(LWQQ_STATUS status)
{
    switch(status){
        case LWQQ_STATUS_ONLINE: return "online";break;
        case LWQQ_STATUS_OFFLINE: return "offline";break;
        case LWQQ_STATUS_AWAY: return "away";break;
        case LWQQ_STATUS_HIDDEN: return "hidden";break;
        case LWQQ_STATUS_BUSY: return "busy";break;
        case LWQQ_STATUS_CALLME: return "callme";break;
        case LWQQ_STATUS_SLIENT: return "slient";break;
        default: return "unknow";break;
    }
}
