/**
 * @file   main.cpp
 * @author microcai <microcaicai@gmail.com>
 * @origal_author mathslinux <riegamaths@gmail.com>
 * 
 */

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>

extern "C" {
#include <lwqq/login.h>
#include <lwqq/logger.h>
#include <lwqq/info.h>
#include <lwqq/smemory.h>
#include <lwqq/msg.h>
#include <lwqq/async.h>
};

#define QQBOT_VERSION "0.0.1"

static int help_f(int argc, char **argv);
static int quit_f(int argc, char **argv);
static int list_f(int argc, char **argv);
static int send_f(int argc, char **argv);

typedef int (*cfunc_t)(int argc, char **argv);

typedef struct CmdInfo {
	const char	*name;
	const char	*altname;
	cfunc_t		cfunc;
} CmdInfo;

static boost::shared_ptr<LwqqClient> lc;

static char vc_image[128];
static char vc_file[128];

static std::string progname;
static std::string logdir;

static CmdInfo cmdtab[] = {
    {"help", "h", help_f},
    {"quit", "q", quit_f},
    {"list", "l", list_f},
    {"send", "s", send_f},
    {NULL, NULL, NULL},
};

static int help_f(int argc, char **argv)
{
    printf(
        "\n"
        " Excute a command\n"
        "\n"
        " help/h, -- Output help\n"
        " list/l, -- List buddies\n"
        "            You can use \"list all\" to list all buddies\n"
        "            or use \"list uin\" to list certain buddy\n"
        " send/s, -- Send message to buddy\n"
        "            You can use \"send uin message\" to send message\n"
        "            to buddy"
        "\n");
    
    return 0;
}

static int quit_f(int argc, char **argv)
{
    return 1;
}

static int list_f(int argc, char **argv)
{
    char buf[1024] = {0};

    /** argv may like:
     * 1. {"list", "all"}
     * 2. {"list", "244569070"}
     */
    if (argc != 2) {
        return 0;
    }

    if (!strcmp(argv[1], "all")) {
        /* List all buddies */
        LwqqBuddy *buddy;
        LIST_FOREACH(buddy, &lc->friends, entries) {
            if (!buddy->uin) {
                /* BUG */
                return 0;
            }
            snprintf(buf, sizeof(buf), "uin:%s, ", buddy->uin);
            if (buddy->nick) {
                strcat(buf, "nick:");
                strcat(buf, buddy->nick);
                strcat(buf, ", ");
            }
            printf("Buddy info: %s\n", buf);
        }
        LwqqGroup * group;
        LIST_FOREACH(group, &lc->groups, entries) {
            if (!group->gid) {
                /* BUG */
                return 0;
            }
            snprintf(buf, sizeof(buf), "uin:%s, ", group->gid);
            if (group->name) {
                strcat(buf, "nick:");
                strcat(buf, group->name);
                strcat(buf, ", ");
            }
            printf("Group info: %s\n", buf);
        }
	} else {
        /* Show buddies whose uin is argv[1] */
        LwqqBuddy *buddy;
        LIST_FOREACH(buddy, &lc->friends, entries) {
            if (buddy->uin && !strcmp(buddy->uin, argv[1])) {
                snprintf(buf, sizeof(buf), "uin:%s, ", argv[1]);
                if (buddy->nick) {
                    strcat(buf, "nick:");
                    strcat(buf, buddy->nick);
                    strcat(buf, ", ");
                }
                if (buddy->markname) {
                    strcat(buf, "markname:");
                    strcat(buf, buddy->markname);
                }
                printf("Buddy info: %s\n", buf);
                break;
            }
        }
    }

    return 0;
}

static int send_f(int argc, char **argv)
{
    /* argv look like: {"send", "74357485" "hello"} */
    if (argc != 3) {
        return 0;
    }
    
    //lwqq_msg_send2(lc.get(), argv[1], argv[2]);
    
    return 0;
}

static char *get_prompt(void)
{
	static char	prompt[256];

	if (!prompt[0])
		snprintf(prompt, sizeof(prompt), "%s> ", progname.c_str());
	return prompt;
}

static char *get_vc()
{
    char vc[128] = {0};
    int vc_len;
    FILE *f;

    if ((f = fopen(vc_file, "r")) == NULL) {
        return NULL;
    }

    if (!fgets(vc, sizeof(vc), f)) {
        fclose(f);
        return NULL;
    }
    
    vc_len = strlen(vc);
    if (vc[vc_len - 1] == '\n') {
        vc[vc_len - 1] = '\0';
    }
    return s_strdup(vc);
}

static LwqqErrorCode cli_login()
{
    LwqqErrorCode err;

    lwqq_login(lc.get(),LWQQ_STATUS_ONLINE, &err);
    if (err == LWQQ_EC_LOGIN_NEED_VC) {
        snprintf(vc_image, sizeof(vc_image), "/tmp/lwqq_%s.jpeg", lc->username);
        snprintf(vc_file, sizeof(vc_file), "/tmp/lwqq_%s.txt", lc->username);
        /* Delete old verify image */
        unlink(vc_file);

        lwqq_log(LOG_NOTICE, "Need verify code to login, please check "
                 "image file %s, and input what you see to file %s\n",
                 vc_image, vc_file);
        while (1) {
            if (!access(vc_file, F_OK)) {
                sleep(1);
                break;
            }
            sleep(1);
        }
        lc->vc->str = get_vc();
        if (!lc->vc->str) {
            goto failed;
        }
        lwqq_log(LOG_NOTICE, "Get verify code: %s\n", lc->vc->str);
        lwqq_login(lc.get(),LWQQ_STATUS_ONLINE, &err);
    } else if (err != LWQQ_EC_OK) {
        goto failed;
    }

    return err;

failed:
    return LWQQ_EC_ERROR;
}

static void cli_logout(LwqqClient *lc)
{
    LwqqErrorCode err;
    
    lwqq_logout(lc, &err);
    if (err != LWQQ_EC_OK) {
        lwqq_log(LOG_DEBUG, "Logout failed\n");        
    } else {
        lwqq_log(LOG_DEBUG, "Logout sucessfully\n");
    }
}

void signal_handler(int signum)
{
	if (signum == SIGINT) {
        cli_logout(lc.get());
        exit(0);
	}
}

//TODO
//将聊天信息写入日志文件！
static void log_message(LwqqClient  *lc, LwqqMsgMessage *mmsg)
{
	LwqqMsgContent *c;
	std::string buf;
	LwqqErrorCode err;

	TAILQ_FOREACH(c, &mmsg->content, entries) {
		if (c->type == LwqqMsgContent::LWQQ_CONTENT_STRING) {
			buf += c->data.str;
        } else {
			printf ("Receive face msg: %d\n", c->data.face);
		}
	}
	//log to disk file
	//get gid
	LwqqGroup* group =  lwqq_group_find_group_by_gid(lc, mmsg->from);
	if (group){
		const char * nick = mmsg->group.send;
		LwqqSimpleBuddy* by = lwqq_group_find_group_member_by_uin(group, mmsg->group.send);
		if (by)
			nick = by->nick;
		printf("Receive message: (%s:%s) (%s), %s\n", group->name, group->account , nick, buf.c_str());
	}else{
		printf("Receive message: %s -> %s , %s\n", mmsg->from, mmsg->to, buf.c_str());	
	}	
}

static void handle_new_msg(LwqqClient  *lc, LwqqRecvMsg *recvmsg)
{
    LwqqMsg *msg = recvmsg->msg;

    printf("Receive message type: %d\n", msg->type);
    if (msg->type == LWQQ_MT_BUDDY_MSG) {
        char buf[1024] = {0};
        LwqqMsgContent *c;
        LwqqMsgMessage *mmsg =(LwqqMsgMessage *) msg->opaque;
        TAILQ_FOREACH(c, &mmsg->content, entries) {
            if (c->type == LwqqMsgContent::LWQQ_CONTENT_STRING) {
                strcat(buf, c->data.str);
            } else {
                printf ("Receive face msg: %d\n", c->data.face);
            }
        }
//         printf("Receive message: %s\n", buf);
    } else if (msg->type == LWQQ_MT_GROUP_MSG) {
        LwqqMsgMessage *mmsg =(LwqqMsgMessage *) msg->opaque;
        char buf[1024] = {0};
        LwqqMsgContent *c;
		log_message(lc, mmsg);

		TAILQ_FOREACH(c, &mmsg->content, entries) {
            if (c->type == LwqqMsgContent::LWQQ_CONTENT_STRING) {
                strcat(buf, c->data.str);
            } else {
                printf ("Receive face msg: %d\n", c->data.face);
            }
        }
//         printf("Receive message: %s\n", buf);
    } else if (msg->type == LWQQ_MT_STATUS_CHANGE) {
        LwqqMsgStatusChange *status = (LwqqMsgStatusChange*)msg->opaque;
        printf("Receive status change: %s - > %s\n", 
               status->who,
               status->status);
    } else {
        printf("unknow message\n");
    }
    
    lwqq_msg_free(recvmsg->msg);
    s_free(recvmsg);
}

static void recvmsg_thread(boost::shared_ptr<LwqqClient> lc)
{
	LwqqRecvMsgList *list = lc->msg_list;
    /* Poll to receive message */
    list->poll_msg(list);

    /* Need to wrap those code so look like more nice */
    while (1) {
        LwqqRecvMsg *recvmsg;
        pthread_mutex_lock(&list->mutex);
        if (TAILQ_EMPTY(&list->head)) {
            /* No message now, wait 100ms */
            pthread_mutex_unlock(&list->mutex);
            boost::this_thread::sleep(boost::posix_time::milliseconds(1));
            continue;
        }
        recvmsg =TAILQ_FIRST(&list->head);
        TAILQ_REMOVE_HEAD(&list->head, entries);
        pthread_mutex_unlock(&list->mutex);
        handle_new_msg(lc.get(), recvmsg);
    }
}

static void got_group_detail_info(LwqqAsyncEvent* event,void* data)
{
	LwqqGroup * group = (LwqqGroup*)data;

	// create log file
	if (!logdir.empty()){//yes my lord, I will log messages
		if (!fs::exists(fs::path(logdir) / group->account)){
			if (!fs::create_directory(fs::path(logdir) / group->account)){
				logdir.clear();
				return ;
			}
		}
		if (!fs::exists(fs::path(logdir) / group->name)){
			fs::create_symlink(fs::path(logdir) / group->account, fs::path(logdir) / group->name);			
		}
	}
}

static void get_group_detail_info(LwqqAsyncEvent* event,void* data)
{
	LwqqErrorCode err;
	LwqqGroup * group;
	LwqqClient * lc = (LwqqClient*)data;
	
	LIST_FOREACH(group, &lc->groups, entries) {
		if (!group->gid) {
			/* BUG */
			continue ;
		}
		lwqq_log(LOG_DEBUG, "get group info %s\n", group->name);
		
		lwqq_async_add_event_listener(lwqq_info_get_qqnumber(lc, 1, group),got_group_detail_info, group );
		lwqq_info_get_group_detail_info(lc,group, &err);
	}
}

static char **breakline(char *input, int *count)
{
    int c = 0;
    char **rval = (char **)calloc(sizeof(char *), 1);
    char **tmp;
    char *token, *save_ptr;

    token = strtok_r(input, " ", &save_ptr);
	while (token) {
        c++;
        tmp = (char **)realloc(rval, sizeof(*rval) * (c + 1));
        rval = tmp;
        rval[c - 1] = token;
        rval[c] = NULL;
        token = strtok_r(NULL, " ", &save_ptr);
	}
    
    *count = c;

    if (c == 0) {
        free(rval);
        return NULL;
    }
    
    return rval;
}

const CmdInfo *find_command(const char *cmd)
{
	CmdInfo	*ct;

	for (ct = cmdtab; ct->name; ct++) {
		if (!strcmp(ct->name, cmd) || !strcmp(ct->altname, cmd)) {
			return (const CmdInfo *)ct;
        }
	}
	return NULL;
}

static void command_loop()
{
    static char command[1024];
    int done = 0;

    while (!done) {
        char **v;
        char *p;
        int c = 0;
        const CmdInfo *ct;
        fprintf(stdout, "%s", get_prompt());
        fflush(stdout);
        memset(&command, 0, sizeof(command));
        if (!fgets(command, sizeof(command), stdin)) {
            /* Avoid gcc warning */
            continue;
        }
        p = command + strlen(command);
        if (p != command && p[-1] == '\n') {
            p[-1] = '\0';
        }
        
        v = breakline(command, &c);
        if (v) {
            ct = find_command(v[0]);
            if (ct) {
                done = ct->cfunc(c, v);
            } else {
                fprintf(stderr, "command \"%s\" not found\n", v[0]);
            }
            free(v);
        }
    }
}

fs::path configfilepath()
{
	if ( fs::exists(fs::path(progname) / "qqbotrc"))
		return fs::path(progname) / "qqbotrc";
	if (getenv("USERPROFILE"))
	{
		if ( fs::exists(fs::path(getenv("USERPROFILE")) / ".qqbotrc"))
			return fs::path(getenv("USERPROFILE")) / ".qqbotrc";
	}
	if (getenv("HOME")){
		if ( fs::exists(fs::path(getenv("HOME")) / ".qqbotrc"))
			return fs::path(getenv("HOME")) / ".qqbotrc";
	}
	if ( fs::exists("/etc/qqbotrc"))
		return fs::path("/etc/qqbotrc");
	throw "not configfileexit";
}

int main(int argc, char *argv[])
{
    std::string qqnumber, password;
    std::string cfgfile;

    LwqqErrorCode err;
    int i, c, e = 0;

    progname = fs::basename(argv[0]);

	po::options_description desc("qqbot options");
	desc.add_options()
	    ( "version,v", "output version" )
		( "help,h", "produce help message" )
        ( "user,u", po::value<std::string>(&qqnumber), "QQ 号" )
		( "pwd,p", po::value<std::string>(&password), "password" )
		( "logdir", po::value<std::string>(&logdir), "password" )
		;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help"))
	{
		std::cerr <<  desc <<  std::endl;
		return 1;
	}
	if (vm.size() ==0 ){
		po::store(po::parse_config_file<char>(configfilepath().c_str(), desc), vm);
		po::notify(vm);
	}
	if (vm.count("version"))
	{
		printf("qqbot version %s \n", QQBOT_VERSION);
	}
	
	if (!logdir.empty()){
		if (!fs::exists(logdir))
			fs::create_directory(logdir);
	}

    /* Lanuch signal handler when user press down Ctrl-C in terminal */
    signal(SIGINT, signal_handler);

    lc.reset( lwqq_client_new(qqnumber.c_str(), password.c_str()), lwqq_client_free );
    if (!lc) {
        lwqq_log(LOG_NOTICE, "Create lwqq client failed\n");
        return -1;
    }

    /* Login to server */
    err = cli_login();
    if (err != LWQQ_EC_OK) {
        lwqq_log(LOG_ERROR, "Login error, exit\n");
        return -1;
    }

    lwqq_log(LOG_NOTICE, "Login successfully\n");

    /* Create a thread to receive message */
    boost::thread(boost::bind(&recvmsg_thread, lc));
    /* update group info */
    lwqq_info_get_friends_info(lc.get(), &err);
    LwqqAsyncEvent* getgroups = lwqq_info_get_group_name_list(lc.get(), &err);
	lwqq_async_add_event_listener(getgroups, get_group_detail_info ,  lc.get() );
	
    /* Enter command loop  */
    command_loop();
    
    /* Logout */
    cli_logout(lc.get());
    return 0;
}
