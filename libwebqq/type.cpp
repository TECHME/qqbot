/**
 * @file   msg.c
 * @author mathslinux <riegamaths@gmail.com>
 * @date   Thu Jun 14 14:42:17 2012
 * 
 * @brief  Message receive and send API
 * 
 * 
 */
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include "webqq.h"
#include "url.hpp"

extern "C" {
#include "json.h"
#include "logger.h"
}

using namespace qq;

static void lwqq_msg_message_free(void *opaque);
static void lwqq_msg_statufree(void *opaque);/*
static int msg_send_back(LwqqHttpRequest* req,void* data);
static int upload_cface_back(LwqqHttpRequest *req,void* data);
static int upload_offline_pic_back(LwqqHttpRequest* req,void* data);
static int upload_offline_file_back(LwqqHttpRequest* req,void* data);
static int send_offfile_back(LwqqHttpRequest* req,void* data);
static void insert_recv_msg_with_order(LwqqRecvMsgList* list,LwqqMsg* msg);*/

LwqqMsg::LwqqMsg(qq::LwqqMsgType type)
{
    LwqqMsgMessage* mmsg;

    this->type = type;

    switch (type) {
    case LWQQ_MT_BUDDY_MSG:
    case LWQQ_MT_GROUP_MSG:
    case LWQQ_MT_DISCU_MSG:
    case LWQQ_MT_SESS_MSG:
        mmsg = (LwqqMsgMessage*)malloc(sizeof(LwqqMsgMessage));
        mmsg->type = type;
        this->opaque = mmsg;
        break;
//     case LWQQ_MT_STATUS_CHANGE:
//         this->opaque = malloc(sizeof(LwqqMsgStatusChange));
//         break;
//     case LWQQ_MT_KICK_MESSAGE:
//         this->opaque = malloc(sizeof(LwqqMsgKickMessage));
//         break;
//     case LWQQ_MT_SYSTEM:
//         this->opaque = malloc(sizeof(LwqqMsgSystem));
//         break;
//     case LWQQ_MT_BLIST_CHANGE:
//         this->opaque = malloc(sizeof(LwqqMsgBlistChange));
//         break;
//     case LWQQ_MT_SYS_G_MSG:
//         this->opaque = malloc(sizeof(LwqqMsgSysGMsg));
//         break;
//     case LWQQ_MT_OFFFILE:
//         this->opaque = malloc(sizeof(LwqqMsgOffFile));
//         break;
//     case LWQQ_MT_FILETRANS:
//         this->opaque = malloc(sizeof(LwqqMsgFileTrans));
//         break;
//     case LWQQ_MT_FILE_MSG:
//         this->opaque = malloc(sizeof(LwqqMsgFileMessage));
//         break;
//     case LWQQ_MT_NOTIFY_OFFFILE:
//         this->opaque = malloc(sizeof(LwqqMsgNotifyOfffile));
//         break;
//     case LWQQ_MT_INPUT_NOTIFY:
//         this->opaque = malloc(sizeof(LwqqMsgInputNotify));
//         break;
    default:
        lwqq_log(LOG_ERROR, "No such message type\n");
        break;
    }
}

LwqqMsg::~LwqqMsg()
{
	if(this->type<=LWQQ_MT_SESS_MSG)
        lwqq_msg_message_free(this->opaque);
	else {
		lwqq_log(LOG_ERROR, "message type not know\n");
	}
//     else if(this->type==LWQQ_MT_STATUS_CHANGE)
//         lwqq_msg_statufree(this->opaque);
//     else if(this->type==LWQQ_MT_KICK_MESSAGE){
//         LwqqMsgKickMessage* kick = (LwqqMsgKickMessage*) this->opaque;
//         if(kick)
//             free(kick->reason);
//         free(kick);
//     }
//     else if(this->type==LWQQ_MT_SYSTEM){
//         lwqq_msg_system_free(this->opaque);
//     }
//     else if(this->type==LWQQ_MT_BLIST_CHANGE){
//         LwqqMsgBlistChange* blist;
//         LwqqBuddy* buddy;
//         LwqqBuddy* next;
//         LwqqSimpleBuddy* simple;
//         LwqqSimpleBuddy* simple_next;
//         blist = (LwqqMsgBlistChange*) this->opaque;
//         if(blist){
//             simple = LIST_FIRST(&blist->added_friends);
//             while(simple){
//                 simple_next = LIST_NEXT(simple,entries);
//                 lwqq_simple_buddy_free(simple);
//                 simple = simple_next;
//             }
//             buddy = LIST_FIRST(&blist->removed_friends);
//             while(buddy){
//                 next = LIST_NEXT(buddy,entries);
//                 lwqq_buddy_free(buddy);
//                 buddy = next;
//             }
//         }
//         free(blist);
//     }
//     else if(this->type==LWQQ_MT_SYS_G_MSG){
//         LwqqMsgSysGMsg* gmsg = (LwqqMsgSysGMsg*) this->opaque;
//         if(gmsg){
//             free(gthis->gcode);
//         }
//         free(gmsg);
//     }else if(this->type==LWQQ_MT_OFFFILE){
//         lwqq_msg_offfile_free(this->opaque);
//     }else if(this->type==LWQQ_MT_FILETRANS){
//         LwqqMsgFileTrans* trans = (LwqqMsgFileTrans*) this->opaque;
//         FileTransItem* item;
//         FileTransItem* item_next;
//         if(trans){
//             free(trans->from);
//             free(trans->to);
//             free(trans->lc_id);
//             item = LIST_FIRST(&trans->file_infos);
//             while(item!=NULL){
//                 item_next = LIST_NEXT(item,entries);
//                 free(item->file_name);
//                 free(item);
//                 item = item_next;
//             }
//         }
//         free(trans);
//     }else if(this->type==LWQQ_MT_FILE_MSG){
//         LwqqMsgFileMessage* file = (LwqqMsgFileMessage*) this->opaque;
//         if(file){
//             free(file->from);
//             free(file->to);
//             free(file->reply_ip);
//             if(file->mode == LwqqMsgFileMessage::MODE_RECV){
//                 free(file->recv.name);
//             };
//         }
//         free(file);
//     }else if(this->type == LWQQ_MT_NOTIFY_OFFFILE){
//         LwqqMsgNotifyOfffile* notify = (LwqqMsgNotifyOfffile*) this->opaque;
//         if(notify){
//             free(notify->from);
//             free(notify->to);
//             free(notify->filename);
//         }
//         free(notify);
//     }else if(this->type == LWQQ_MT_INPUT_NOTIFY){
//         LwqqMsgInputNotify * input = (LwqqMsgInputNotify*) this->opaque;
//         if(input){
//             free(input->from);
//             free(input->to);
//         }
//         free(input);
//     }else{
//         lwqq_log(LOG_ERROR, "No such message type\n");
//     }
}

static void lwqq_msg_message_free(void *opaque)
{
    LwqqMsgMessage *msg = (LwqqMsgMessage*) opaque;
    if (!msg) {
        return ;
    }

    free(msg->from);
    free(msg->to);
    free(msg->msg_id);
    if(msg->type == LWQQ_MT_GROUP_MSG){
        free(msg->group.send);
        free(msg->group.group_code);
    }else if(msg->type == LWQQ_MT_SESS_MSG){
        free(msg->sess.id);
        free(msg->sess.group_sig);
    }else if(msg->type == LWQQ_MT_DISCU_MSG){
        free(msg->discu.send);
        free(msg->discu.did);
    }
    
    for (int i = 0 ; i < msg->content.size() ;  i ++ )
	{
		LwqqMsgContent *c = & msg->content[i];
        switch(c->type){
            case LwqqMsgContent::LWQQ_CONTENT_STRING:
                free(c->data.str);
                break;
            case LwqqMsgContent::LWQQ_CONTENT_OFFPIC:
                free(c->data.img.file_path);
                free(c->data.img.name);
                free(c->data.img.data);
                break;
            case LwqqMsgContent::LWQQ_CONTENT_CFACE:
                free(c->data.cface.data);
                free(c->data.cface.name);
                free(c->data.cface.file_id);
                free(c->data.cface.key);
                break;
            default:
                break;
        }		
	}
    free(msg);
}
