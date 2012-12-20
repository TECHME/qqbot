
#include "irc.h"
#include <string>
#include <iostream>

using namespace std;

// int n2u(const char *in, size_t in_len, char *out, size_t out_len)
// {
//     std::vector<wchar_t> wbuf(in_len+1);
//     int wlen = MultiByteToWideChar(CP_ACP, 0, in, (int)in_len, &wbuf[0], (int)in_len);
//     if( wlen < 0) return -1;
//     wbuf[wlen] = 0;
// 
//     int len = WideCharToMultiByte(CP_UTF8, 0, &wbuf[0], (int)wlen, out, (int)out_len, NULL, FALSE);
//     if(len < 0)   return -1;
//     out[len] = 0;
// 
//     return len;
// };


void my_cb(const IrcMsg pMsg)
{

}

void input_func(void* p)
{

    while(true)
    {
        char buf[512]={0};    
        int i=0;
        while((buf[i++]=cin.get())!='\n');
        {
            
            buf[--i]='\0';
            char *msg=(char*)malloc((strlen(buf)*2)+1);
            //n2u(buf,strlen(buf),msg,(strlen(buf)*2)+1);
            IrcClient* pClient=(IrcClient*)p;
            pClient->chat("#avplayer",msg);
            free(msg);
        }
    }

}

int
main(int argc, char **argv) 
{

    IrcClient client(my_cb,"irc.freenode.net","6667");
    client.login("qqbothaha","#avplayer");
    boost::thread t(boost::bind(&input_func,&client));

    while (1)
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    }
	return 0;
}
