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
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
namespace pt = boost::property_tree;
namespace json = pt::json_parser;

#include "webqq.h"
#include "defer.hpp"
#include "url.hpp"

extern "C"{
#include "logger.h"
#include "md5.h"
#include "json.h"
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
static char *do_ucs4toutf8(const char *from)
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
    
    return strdup(str);
}

static std::string ucs4toutf8(const char *from)
{
    if (!from) {
        return NULL;
    }
    
    char *out = NULL;
    int outlen = 0;
    const char *c;
    
    for (c = from; *c != '\0'; ++c) {
        char *s;
        if (*c == '\\' && *(c + 1) == 'u') {
            s = do_ucs4toutf8(c);
            out = (char*)realloc(out, outlen + strlen(s) + 1);
            snprintf(out + outlen, strlen(s) + 1, "%s", s);
            outlen = strlen(out);
            free(s);
            c += 5;
        } else {
            out = (char*)realloc(out, outlen + 2);
            out[outlen] = *c;
            out[outlen + 1] = '\0';
            outlen++;
            continue;
        }
    }

    std::string ret = out;
    free(out);
    return ret;
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

/**
 * Get the result object in a json object.
 *
 * @param str
 *
 * @return result object pointer on success, else NULL on failure.
 */
static json_t *get_result_json_object(json_t *json)
{
    json_t *json_tmp;
    char *value;

    /**
     * Frist, we parse retcode that indicate whether we get
     * correct response from server
     */
    value = json_parse_simple_value(json, "retcode");
    if (!value || strcmp(value, "0")) {
        return NULL;
    }

    /**
     * Second, Check whether there is a "result" key in json object
     */
    json_tmp = json_find_first_label_all(json, "result");
    if (!json_tmp) {
        return NULL;
    }

    return json_tmp;
}

static LwqqMsgType parse_recvmsg_type(json_t *json)
{
    LwqqMsgType type = LWQQ_MT_UNKNOWN;
    char *msg_type = json_parse_simple_value(json, "poll_type");
    if (!msg_type) {
        return type;
    }
    if (!strncmp(msg_type, "message", strlen("message"))) {
        type = LWQQ_MT_BUDDY_MSG;
    } else if (!strncmp(msg_type, "group_message", strlen("group_message"))) {
        type = LWQQ_MT_GROUP_MSG;
    }else if(!strncmp(msg_type,"discu_message",strlen("discu_message"))){
        type = LWQQ_MT_DISCU_MSG;
    }else if(!strncmp(msg_type,"sess_message",strlen("sess_message"))){
        type = LWQQ_MT_SESS_MSG;
    } else if (!strncmp(msg_type, "buddies_status_change",
                        strlen("buddies_status_change"))) {
        type = LWQQ_MT_STATUS_CHANGE;
    } else if(!strncmp(msg_type,"kick_message",strlen("kick_message"))){
        type = LWQQ_MT_KICK_MESSAGE;
    } else if(!strncmp(msg_type,"system_message",strlen("system_message"))){
        type = LWQQ_MT_SYSTEM;
    }else if(!strncmp(msg_type,"buddylist_change",strlen("buddylist_change"))){
        type = LWQQ_MT_BLIST_CHANGE;
    }else if(!strncmp(msg_type,"sys_g_msg",strlen("sys_g_msg"))){
        type = LWQQ_MT_SYS_G_MSG;
    }else if(!strncmp(msg_type,"push_offfile",strlen("push_offfile"))){
        type = LWQQ_MT_OFFFILE;
    }else if(!strncmp(msg_type,"filesrv_transfer",strlen("filesrv_transfer"))){
        type = LWQQ_MT_FILETRANS;
    }else if(!strncmp(msg_type,"file_message",strlen("file_message"))){
        type = LWQQ_MT_FILE_MSG;
    }else if(!strncmp(msg_type,"notify_offfile",strlen("file_message"))){
        type = LWQQ_MT_NOTIFY_OFFFILE;
    }else if(!strncmp(msg_type,"input_notify",strlen("input_notify"))){
        type = LWQQ_MT_INPUT_NOTIFY;
    }else
        type = LWQQ_MT_UNKNOWN;
    return type;
}

static int parse_content(json_t *json, void *opaque)
{
    json_t *tmp, *ctent;
    LwqqMsgMessage *msg =(LwqqMsgMessage *) opaque;

    tmp = json_find_first_label_all(json, "content");
    if (!tmp || !tmp->child || !tmp->child) {
        return -1;
    }
    tmp = tmp->child->child;
    for (ctent = tmp; ctent != NULL; ctent = ctent->next) {
        if (ctent->type == JSON_ARRAY) {
            /* ["font",{"size":10,"color":"000000","style":[0,0,0],"name":"\u5B8B\u4F53"}] */
            char *buf;
            /* FIXME: ensure NULL access */
            buf = ctent->child->text;
            if (!strcmp(buf, "font")) {
                const char *name, *color, *size;
                int sa, sb, sc;
                /* Font name */
                name = json_parse_simple_value(ctent, "name");
                name = name ?: "Arial";
                msg->f_name = ucs4toutf8(name);

                /* Font color */
                color = json_parse_simple_value(ctent, "color");
                msg->f_color = color ?: "000000";

                /* Font size */
                size = json_parse_simple_value(ctent, "size");
                size = size ?: "12";
                msg->f_size = atoi(size);

                /* Font style: style":[0,0,0] */
                tmp = json_find_first_label_all(ctent, "style");
                if (tmp) {
                    json_t *style = tmp->child->child;
                    const char *stylestr = style->text;
                    sa = (int)strtol(stylestr, NULL, 10);
                    style = style->next;
                    stylestr = style->text;
                    sb = (int)strtol(stylestr, NULL, 10);
                    style = style->next;
                    stylestr = style->text;
                    sc = (int)strtol(stylestr, NULL, 10);
                } else {
                    sa = 0;
                    sb = 0;
                    sc = 0;
                }
                msg->f_style.b = sa;
                msg->f_style.i = sb;
                msg->f_style.u = sc;
            } else if (!strcmp(buf, "face")) {
                /* ["face", 107] */
                /* FIXME: ensure NULL access */
                int facenum = (int)strtol(ctent->child->next->text, NULL, 10);
                LwqqMsgContent c;
                c.type =  LwqqMsgContent::LWQQ_CONTENT_FACE;
                c.data.face = facenum; 
                msg->content.push_back(c);
            } else if(!strcmp(buf, "offpic")) {
                //["offpic",{"success":1,"file_path":"/d65c58ae-faa6-44f3-980e-272fb44a507f"}]
                LwqqMsgContent c;
                c.type = LwqqMsgContent::LWQQ_CONTENT_OFFPIC;
                c.data.img.success = atoi(json_parse_simple_value(ctent,"success"));
                c.data.img.file_path = strdup(json_parse_simple_value(ctent,"file_path"));
                msg->content.push_back(c);
            } else if(!strcmp(buf,"cface")){
                //["cface",{"name":"0C3AED06704CA9381EDCC20B7F552802.jPg","file_id":914490174,"key":"YkC3WaD3h5pPxYrY","server":"119.147.15.201:443"}]
                //["cface","0C3AED06704CA9381EDCC20B7F552802.jPg",""]
                LwqqMsgContent c;
                c.type = LwqqMsgContent::LWQQ_CONTENT_CFACE;
                c.data.cface.name = strdup(json_parse_simple_value(ctent,"name"));
                if(c.data.cface.name!=NULL){
                    c.data.cface.file_id = strdup(json_parse_simple_value(ctent,"file_id"));
                    c.data.cface.key = strdup(json_parse_simple_value(ctent,"key"));
                    char* server = strdup(json_parse_simple_value(ctent,"server"));
                    char* split = strchr(server,':');
                    strncpy(c.data.cface.serv_ip,server,split-server);
                    strncpy(c.data.cface.serv_port,split+1,strlen(split+1));
                }else{
                    c.data.cface.name = strdup(ctent->child->next->text);
                }
                msg->content.push_back(c);
            }
        } else if (ctent->type == JSON_STRING) {
            LwqqMsgContent c;
            c.type = LwqqMsgContent::LWQQ_CONTENT_STRING;
            c.data.str = json_unescape(ctent->text);
            msg->content.push_back(c);
        }
    }

    /* Make msg valid */
    if (msg->f_name.empty() || msg->f_color.empty() || msg->content.empty()) {
        return -1;
    }
    if (msg->f_size < 8) {
        msg->f_size = 8;
    }

    return 0;
}

/**
 * {"poll_type":"message","value":{"msg_id":5244,"from_uin":570454553,
 * "to_uin":75396018,"msg_id2":395911,"msg_type":9,"reply_ip":176752041,
 * "time":1339663883,"content":[["font",{"size":10,"color":"000000",
 * "style":[0,0,0],"name":"\u5B8B\u4F53"}],"hello\n "]}}
 * 
 * @param json
 * @param opaque
 * 
 * @return
 */
static int parse_new_msg(json_t *json, void *opaque)
{
    LwqqMsgMessage *msg = (LwqqMsgMessage*)opaque;
    const char *time;
    
    msg->from = strdup(json_parse_simple_value(json, "from_uin"));
    if (!msg->from) {
        return -1;
    }

    time = json_parse_simple_value(json, "time");
    time = time ?: "0";
    msg->time = (time_t)strtoll(time, NULL, 10);
    msg->to = strdup(json_parse_simple_value(json, "to_uin"));
    msg->msg_id = strdup(json_parse_simple_value(json,"msg_id"));
    msg->msg_id2 = atoi(json_parse_simple_value(json, "msg_id2"));

    //if it failed means it is not group message.
    //so it equ NULL.
    if(msg->type == LWQQ_MT_GROUP_MSG){
        msg->group.send = strdup(json_parse_simple_value(json, "send_uin"));
        msg->group.group_code = strdup(json_parse_simple_value(json,"group_code"));
    }else if(msg->type == LWQQ_MT_SESS_MSG){
        msg->sess.id = strdup(json_parse_simple_value(json,"id"));
    }else if(msg->type == LWQQ_MT_DISCU_MSG){
        msg->discu.send = strdup(json_parse_simple_value(json, "send_uin"));
        msg->discu.did = strdup(json_parse_simple_value(json,"did"));
    }

    if (!msg->to) {
        return -1;
    }
    
    if (parse_content(json, opaque)) {
        return -1;
    }

    return 0;
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
	loginstream->async_open(url,boost::bind(&webqq::cb_do_login,this, loginstream, boost::asio::placeholders::error()) );

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
        sava_cookie(&cookies, stream->get_httpheader());
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

	stream->async_open(LWQQ_URL_SET_STATUS,boost::bind(&webqq::cb_online_status,this, stream, boost::asio::placeholders::error()) );
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

	pollstream->async_open(url,boost::bind(&webqq::cb_poll_msg,this, pollstream, boost::asio::placeholders::error()) );
}

void webqq::cb_online_status(read_streamptr stream, char* response, const boost::system::error_code& ec, std::size_t length)
{
    json_t *json = NULL;
	defer(boost::bind(operator delete, response));
	
	//psessionid
	json_error ret = json_parse_document(&json, response);
	if (!json)
	 return;

	defer(boost::bind(json_free_value, &json));
	
	char* value = json_parse_simple_value(json, "retcode");
	psessionid = json_parse_simple_value(json, "psessionid");
	//start polling messages, 2 connections!
	do_poll_one_msg();
	do_poll_one_msg();
}

void webqq::cb_poll_msg(char* response, const boost::system::error_code& ec, std::size_t length)
{
	defer(boost::bind(operator delete, response));
	//开启新的 poll	
	do_poll_one_msg();
	
	//处理！
	std::cout << response <<  std::endl;


	const char* retcode_str;
	int ret;
    int retcode = 0;
    json_t *json = NULL, *json_tmp, *cur;

    ret = json_parse_document(&json, (char *)response);
    if (!json)
		return ;

	defer(boost::bind(json_free_value, &json));

    char* dbg_str = json_unescape((char*)response);
    lwqq_puts(dbg_str);
    free(dbg_str);
    
    if (ret != JSON_OK) {
        lwqq_log(LOG_ERROR, "Parse json object of friends error: %s\n", response);
        return;
    }
    retcode_str = json_parse_simple_value(json,"retcode");
    if(retcode_str)
        retcode = atoi(retcode_str);

    json_tmp = get_result_json_object(json);
    if (!json_tmp) {
        lwqq_log(LOG_ERROR, "Parse json object error: %s\n", response);
        return;
    }

    if (!json_tmp->child || !json_tmp->child->child) {
        return;
    }

    /* make json_tmp point to first child of "result" */
    json_tmp = json_tmp->child->child_end;
    for (cur = json_tmp; cur != NULL; cur = cur->previous) {
		int ret;
		LwqqMsgType msg_type = parse_recvmsg_type(cur);

		LwqqMsg msg(msg_type);
//         LwqqAsyncEvset* ev;
//         if (!msg) {
//             continue;
//         }

        switch (msg_type) {
        case LWQQ_MT_BUDDY_MSG:
        case LWQQ_MT_GROUP_MSG:
        case LWQQ_MT_DISCU_MSG:
        case LWQQ_MT_SESS_MSG:
			lwqq_log(LOG_DEBUG, "group_message\n");
            ret = parse_new_msg(cur, msg.opaque);
//             ev = lwqq_msg_request_picture(list->lc, (int)msg->type,(LwqqMsgMessage*) msg->opaque);
//             if(ev){
//                 ret = -1;
//                 void **d = (void**) s_malloc0(sizeof(void*)*2);
//                 d[0] = list;
//                 d[1] = msg;
//                 lwqq_async_add_evset_listener(ev,insert_msg_delay_by_request_content,d);
//                 //this jump the case
//                 continue;
// 			}
            break;
        case LWQQ_MT_STATUS_CHANGE:
//             ret = parse_status_change(cur, msg->opaque);
            break;
        case LWQQ_MT_KICK_MESSAGE:
//             ret = parse_kick_message(cur,msg->opaque);
            break;
        case LWQQ_MT_SYSTEM:
//             ret = parse_system_message(cur,msg->opaque,list->lc);
            break;
        case LWQQ_MT_BLIST_CHANGE:
//             ret = parse_blist_change(cur,msg->opaque,list->lc);
            break;
        case LWQQ_MT_SYS_G_MSG:
//             ret = parse_sys_g_msg(cur,msg->opaque);
            break;
        case LWQQ_MT_OFFFILE:
//             ret = parse_push_offfile(cur,msg->opaque);
            break;
        case LWQQ_MT_FILETRANS:
//             ret = parse_file_transfer(cur,msg->opaque);
            break;
        case LWQQ_MT_FILE_MSG:
//             ret = parse_file_message(cur,msg->opaque);
            break;
        case LWQQ_MT_NOTIFY_OFFFILE:
//             ret = parse_notify_offfile(cur,msg->opaque);
            break;
        case LWQQ_MT_INPUT_NOTIFY:
//             ret = parse_input_notify(cur,msg->opaque);
            break;
        default:
            ret = -1;
            lwqq_log(LOG_ERROR, "No such message type\n");
            break;
        }

        if (ret == 0) {
//             insert_recv_msg_with_order(list,msg);
			sigmessage(msg);
        } else {
//             lwqq_msg_free(msg);
        }
    }
}

void webqq::cb_get_version(read_streamptr stream, const boost::system::error_code& ec)
{
	char * data = new char[1024];
	boost::asio::async_read(*stream, boost::asio::buffer(data, 1024),
		boost::bind(&webqq::cb_got_version, this, data, boost::asio::placeholders::error() ,  boost::asio::placeholders::bytes_transferred()) );
}

void webqq::cb_get_vc(read_streamptr stream, const boost::system::error_code& ec)
{
	char * data = new char[1024];
	boost::asio::async_read(*stream, boost::asio::buffer(data, 1024),
		boost::bind(&webqq::cb_got_vc, this,stream, data, boost::asio::placeholders::error(),  boost::asio::placeholders::bytes_transferred()) );
}


void webqq::cb_do_login(read_streamptr stream, const boost::system::error_code& ec)
{
	char * data = new char[8192];
	boost::asio::async_read(*stream, boost::asio::buffer(data, 8192),
		boost::bind(&webqq::cb_done_login, this,stream, data, boost::asio::placeholders::error(),  boost::asio::placeholders::bytes_transferred()) );
}

void webqq::cb_online_status(read_streamptr stream, const boost::system::error_code& ec)
{
	char * data = new char[8192];
	memset(data, 0, 8192);
	boost::asio::async_read(*stream, boost::asio::buffer(data, 8192),
		boost::bind(&webqq::cb_online_status, this, stream, data, boost::asio::placeholders::error(),  boost::asio::placeholders::bytes_transferred()) );
}

void webqq::cb_poll_msg(read_streamptr stream, const boost::system::error_code& ec)
{
	char * data = new char[16384];
	boost::asio::async_read(*stream, boost::asio::buffer(data, 16384),
		boost::bind(&webqq::cb_poll_msg, this, data, boost::asio::placeholders::error(),  boost::asio::placeholders::bytes_transferred()) );
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
