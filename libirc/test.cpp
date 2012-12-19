#include "irc.h"
#include <string>
#include <iostream>
#include <process.h>
#include <vector>

#ifdef WIN32
#pragma comment (lib,"ws2_32.lib")
#endif
// 
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


void my_cb(const Irc::IrcMessage* pMsg)
{
    for (size_t i=0;i<pMsg->pcount;i++)
        cout << pMsg->param[i] <<endl;

}

Irc irc(my_cb);

// unsigned int __stdcall input_func(void* p)
// {
// 
//     string str;
//     while(true)
//     {
//         cin >> str;
//         {
//             char *msg=(char*)malloc((str.length()*2)+1);
//             n2u(str.c_str(),str.length(),msg,(str.length()*2)+1);
//             irc.privmsg("#avplayer",msg);
//             free(msg);
//         }
//     }
//     return 0;
//

int
main(int argc, char **argv) 
{
    //_beginthreadex(NULL,0,input_func,NULL,0,0);

	irc.connect("irc.freenode.net");
	irc.nick("qqbot");
	irc.user("qqbot","0","qqbot");
    irc.join("#avplayer");

    //this is block for blocking
	irc.eventLoop();
		
	return 0;
}
