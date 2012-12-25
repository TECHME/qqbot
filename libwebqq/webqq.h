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

#ifndef WEBQQ_H
#define WEBQQ_H

#include <string>
#include <map>
#include <boost/asio.hpp>
#include <boost/signal.hpp>
#include <boost/concept_check.hpp>
#include <urdl/read_stream.hpp>
#include <boost/property_tree/ptree.hpp>
namespace pt = boost::property_tree;

typedef boost::shared_ptr<urdl::read_stream> read_streamptr;

namespace qq{

typedef enum LWQQ_STATUS{
    LWQQ_STATUS_UNKNOW = 0,
    LWQQ_STATUS_ONLINE = 10,
    LWQQ_STATUS_OFFLINE = 20,
    LWQQ_STATUS_AWAY = 30,
    LWQQ_STATUS_HIDDEN = 40,
    LWQQ_STATUS_BUSY = 50,
    LWQQ_STATUS_CALLME = 60,
    LWQQ_STATUS_SLIENT = 70
}LWQQ_STATUS;

typedef enum LwqqMsgType {
    LWQQ_MT_BUDDY_MSG = 0,
    LWQQ_MT_GROUP_MSG,
    LWQQ_MT_DISCU_MSG,
    LWQQ_MT_SESS_MSG, //group whisper message
    LWQQ_MT_STATUS_CHANGE,
    LWQQ_MT_KICK_MESSAGE,
    LWQQ_MT_SYSTEM,
    LWQQ_MT_BLIST_CHANGE,
    LWQQ_MT_SYS_G_MSG,
    LWQQ_MT_OFFFILE,
    LWQQ_MT_FILETRANS,
    LWQQ_MT_FILE_MSG,
    LWQQ_MT_NOTIFY_OFFFILE,
    LWQQ_MT_INPUT_NOTIFY,
    LWQQ_MT_UNKNOWN,
} LwqqMsgType;


#include <vector>

typedef enum {
    LWQQ_MC_OK = 0,
    LWQQ_MC_TOO_FAST = 108,             //< send message too fast
    LWQQ_MC_LOST_CONN = 121
}LwqqMsgRetcode;

typedef struct LwqqMsgContent {
    enum {
        LWQQ_CONTENT_STRING,
        LWQQ_CONTENT_FACE,
        LWQQ_CONTENT_OFFPIC,
//custom face :this can send/recv picture
        LWQQ_CONTENT_CFACE
    }type;
    union {
        int face;
        char *str;
        //webqq protocol
        //this used in offpic format which may replaced by cface (custom face)
        struct {
            char* name;
            char* data;
            size_t size;
            int success;
            char* file_path;
        }img;
        struct {
            char* name;
            char* data;
            size_t size;
            char* file_id;
            char* key;
            char serv_ip[24];
            char serv_port[8];
        }cface;
    } data;
} LwqqMsgContent ;

typedef struct LwqqMsgMessage {
    qq::LwqqMsgType type;
    char *from;
    char *to;
    char *msg_id;
    int msg_id2;
    time_t time;
    union{
        struct {
            char *send; /* only group use it to identify who send the group message */
            char *group_code; /* only avaliable in group message */
        }group;
        struct {
            char *id;   /* only sess msg use it.means gid */
            char *group_sig; /* you should fill it before send */
        }sess;
        struct {
            char *send;
            char *did;
        }discu;
    };

    /* For font  */
    std::string f_name;
    int f_size;
    struct {
        int b, i, u; /* bold , italic , underline */
    } f_style;
    std::string f_color;

    std::vector<LwqqMsgContent> content;
} LwqqMsgMessage;

typedef struct LwqqMsgStatusChange {
    char *who;
    char *status;
    int client_type;
} LwqqMsgStatusChange;

typedef struct LwqqMsgKickMessage {
    int show_reason;
    char *reason;
    char *way;
} LwqqMsgKickMessage;
typedef struct LwqqMsgSystem{
    char* seq;
    enum {
        VERIFY_REQUIRED,
        VERIFY_PASS_ADD,
        VERIFY_PASS,
        ADDED_BUDDY_SIG,
        SYSTEM_TYPE_UNKNOW
    }type;
    char* from_uin;
    char* account;
    char* stat;
    char* client_type;
    union{
        struct{
            char* sig;
        }added_buddy_sig;
        struct{
            char* msg;
            char* allow;
        }verify_required;
        struct{
            char* group_id;
        }verify_pass;
    };
} LwqqMsgSystem;
typedef struct LwqqMsgSysGMsg{
    enum {
        GROUP_CREATE,
        GROUP_JOIN,
        GROUP_LEAVE,
        GROUP_UNKNOW
    }type;
    char* gcode;
}LwqqMsgSysGMsg;
typedef struct LwqqMsgBlistChange{
//     LIST_HEAD(,LwqqSimpleBuddy) added_friends;
//     LIST_HEAD(,LwqqBuddy) removed_friends;
} LwqqMsgBlistChange;
typedef struct LwqqMsgOffFile{
    char* msg_id;
    char* rkey;
    char ip[24];
    char port[8];
    char* from;
    char* to;
    size_t size;
    char* name;
    char* path;///< only used when upload
    time_t expire_time;
    time_t time;
}LwqqMsgOffFile;
enum _file_status {
	TRANS_OK=0,
	TRANS_ERROR=50,
	TRANS_TIMEOUT=51,
	REFUSED_BY_CLIENT=53,
};

    typedef struct FileTransItem{
    char* file_name;
	enum _file_status file_status;
    //int file_status;
    int pro_id;
//     LIST_ENTRY(FileTransItem) entries;
}FileTransItem;
typedef struct LwqqMsgFileTrans{
    int file_count;
    char* from;
    char* to;
    char* lc_id;
    size_t now;
    int operation;
    int type;
//     LIST_HEAD(,FileTransItem) file_infos;
}LwqqMsgFileTrans;
typedef struct LwqqMsgFileMessage{
    int msg_id;
    enum {
        MODE_RECV,
        MODE_REFUSE,
        MODE_SEND_ACK
    } mode;
    char* from;
    char* to;
    int msg_id2;
    int session_id;
    time_t time;
    int type;
    char* reply_ip;
    union{
        struct {
            int msg_type;
            char* name;
            int inet_ip;
        }recv;
        struct {
            enum{
                CANCEL_BY_USER=1,
                CANCEL_BY_OVERTIME=3
            } cancel_type;
        }refuse;
    };
}LwqqMsgFileMessage;

typedef struct LwqqMsgNotifyOfffile{
    int msg_id;
    char* from;
    char* to;
    enum {
        NOTIFY_OFFFILE_REFUSE = 2,
    }action;
    char* filename;
    size_t filesize;
}LwqqMsgNotifyOfffile;

typedef struct LwqqMsgInputNotify{
    char* from;
    char* to;
    int msg_type;
}LwqqMsgInputNotify;

struct LwqqMsg {
    /* Message type. e.g. buddy message or group message */
    qq::LwqqMsgType type;
    void *opaque;               /**< Message details */

    LwqqMsg(qq::LwqqMsgType type);
    ~LwqqMsg();
};

// 好友
class Buddy{
	
};

// 群成员啊，临时聊天之类的
class SimpleBuddy{
	
};

// 群
class Group{
	

};


typedef struct LwqqVerifyCode {
    std::string str;
    std::string type;
    std::string img;
    std::string uin;
    std::string data;
    size_t size;
} LwqqVerifyCode;

typedef struct LwqqCookies {
    std::string ptvfsession;          /**< ptvfsession */
    std::string ptcz;
    std::string skey;
    std::string ptwebqq;
    std::string ptuserinfo;
    std::string uin;
    std::string ptisp;
    std::string pt2gguin;
    std::string verifysession;
    std::string lwcookies;
} LwqqCookies;

class webqq
{
public:
	webqq(boost::asio::io_service & asioservice, std::string qqnum, std::string passwd, LWQQ_STATUS status = LWQQ_STATUS_ONLINE);

	void send_simple_message( );

public:// signals
	// 登录成功激发.
	boost::signal< void ()> siglogin;
	// 验证码, 需要返回验证码。图片内容在 const_buffer
	boost::signal< std::string ( boost::asio::const_buffer )> signeedvc;
	// 断线的时候激发.
	boost::signal< void ()> sigoffline;

	// 发生错误的时候激发, 返回 false 停止登录，停止发送，等等操作。true则重试.
	boost::signal< bool (int stage, int why)> sigerror;
	
	// 有群消息的时候激发.
	boost::signal< void (const pt::ptree & )> siggroupmessage;
	static std::string lwqq_status_to_str(LWQQ_STATUS status);

private:
	void cb_get_version(read_streamptr stream, const boost::system::error_code& ec);
	void cb_got_version(char * response, const boost::system::error_code& ec, std::size_t length);

	void cb_get_vc(read_streamptr stream, const boost::system::error_code& ec);
	void cb_got_vc(read_streamptr stream, char* response, const boost::system::error_code& ec, std::size_t length);

	void cb_do_login(read_streamptr stream, const boost::system::error_code& ec);
	void cb_done_login(read_streamptr stream, char * response, const boost::system::error_code& ec, std::size_t length);

	//last step for login
	void set_online_status();
	void cb_online_status(read_streamptr stream, const boost::system::error_code& ec);
	void cb_online_status(read_streamptr stream, char * response, const boost::system::error_code& ec, std::size_t length);


	void do_poll_one_msg();
	void cb_poll_msg(read_streamptr stream, const boost::system::error_code& ec);
	void cb_poll_msg(read_streamptr stream, char * response, const boost::system::error_code& ec, std::size_t length, size_t goten);

	void process_msg(pt::ptree jstree);

private:
    boost::asio::io_service & io_service;

	std::string qqnum, passwd;
    LWQQ_STATUS status;

	std::string	version;
	std::string clientid, psessionid;

	LwqqVerifyCode vc;
	LwqqCookies cookies;
};

};
#endif // WEBQQ_H
