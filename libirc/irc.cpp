/* $ID$ */
#include "irc.h"
#include <assert.h>
#include <iostream>
#ifndef WIN32
#include <arpa/inet.h>
#endif
#include <stdarg.h>
#include <string>
#include <errno.h>

/********************************************************************/

Irc::Irc(privmsg_cb callback)
	: crlf("\r\n"), buffer_index(0),cb(callback)
{
#ifdef WIN32
    WSAData dd;
    WSAStartup(MAKEWORD(2,2),&dd);
#endif

	buffer = new char[512+1];
	memset(buffer, 0, 512+1);
	
	addr = reinterpret_cast<sockaddr_in*> (new sockaddr_in) ;
	addr->sin_family = AF_INET;
    sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

/********************************************************************/

Irc::~Irc() {
	disconnect();
	delete [] addr;
	delete [] buffer;
}

/********************************************************************/

void
Irc::sendMessage(const string cmd, const char *param, ...) const {
	string msg = cmd;
	
	va_list arg;
	
	if(param)	va_start(arg, param);
	while(param) {
		string tmp = param;
		param = va_arg(arg, char *);
		if(!param)	msg += " :";
		else msg += " ";
		msg += tmp;
	}
	va_end(arg);

	msg += crlf;
#ifdef DEBUG
	cout << msg;
#endif
	send(sock, msg.c_str(), msg.length(), 0);	
}


/********************************************************************/

string
Irc::host2ip(const char* host){

    HOSTENT *lpHostEnt=0;
    lpHostEnt=gethostbyname(host);
    if(lpHostEnt==0)
        return "";

    char* lpAddr = lpHostEnt->h_addr_list[0];
    if(lpAddr)
    {
        struct in_addr inAddr;
        inAddr.s_addr = *(u_long *)lpAddr;
        char* p=inet_ntoa(inAddr);
        if(!p)
            return "";
        else
            return (std::string)p;
    }else
        return "";

}

void
Irc::connect(const string host, unsigned short port) {

	// first, look up hostname
    string ip=host2ip(host.c_str());

	struct sockaddr_in *in = reinterpret_cast<sockaddr_in*>(addr);
    in->sin_addr.s_addr= inet_addr(ip.c_str());
	in->sin_port = htons(port);
    in->sin_family = AF_INET;

    int ret = ::connect(sock, (sockaddr*)addr, sizeof(sockaddr_in));
	if (ret == -1)		
        perror("connect");
}

/********************************************************************/

void
Irc::disconnect() {
	shutdown(sock,2);
}

/********************************************************************/

bool Irc::findCTCP(string msg, string &ctcp) {
	size_t start, end;
	
	start = msg.find('\001');
	if(start == string::npos) return false;
	end = msg.find('\001', start+1);
	if(end == string::npos) return false;
	ctcp = msg.substr(start+1, end-start-1);
	return true;
}

/********************************************************************/

void
Irc::processMessage(IrcMessage *m) {
	// PING
	if(m->command == "PING" && m->pcount == 1)
        this->pong(m->param[0]);

	if(m->have_prefix) {
		// JOIN
		if(m->command == "JOIN" && m->pcount == 1);
			//onJoin(this, m->prefix, m->param[0]);
		
		// PRIVMSG and CTCP
		if(m->command == "PRIVMSG" && m->pcount == 2) {
			string ctcp;
			if(findCTCP(m->param[1], ctcp)) {
				// this is a CTCP data string
				// TODO: embedded CTCP?
				//if(ctcp == "VERSION") onCTCPVersion(this,m->prefix);
				//if(ctcp == "FINGER") onCTCPFinger(this,m->prefix);
				//if(ctcp == "SOURCE") onCTCPSource(this,m->prefix);
				//if(ctcp == "USERINFO") onCTCPUserInfo(this,m->prefix);
				//if(ctcp == "CLIENTINFO") onCTCPClientInfo(this,m->prefix);
				//if(ctcp == "PING") onCTCPPing(this,m->prefix);
			} else {
				// normal PRIVMSG
				//onPrivMsg(this, m->prefix, m->param[0], m->param[1]);
                cb(m);
                //cout << m->prefix + "::" + m->param[0]+"||"+m->param[1] <<endl;
			}
		} else if(m->command == "PRIVMSG" && m->pcount != 2) {
			cout << "TODO: privmsg with pcount != 2" << endl;
		}
	}
		
	// RPL_WELCOME
	if(m->command == "001") {
		string welcome_msg;
		for(size_t i = 0; i < m->pcount; i++) {
			welcome_msg += " ";
			welcome_msg += m->param[i];
		}
		//onRegistered(this, welcome_msg);
	}
}

/********************************************************************/

Irc::IrcMessage *
Irc::parseMessage(const string msg) {

	IrcMessage *m = new IrcMessage;
	for(int i = 0; i < 15; i++)
		m->param[i] = "";
	m->command = "";
	m->prefix = "";
	
	m->have_prefix = (msg[0] == ':');
	
	// extract prefix from message if present
	size_t msgstart = 0;
	if(m->have_prefix) {
		size_t pos = msg.find(" ");
		m->prefix = msg.substr(1, pos - 1);
#ifdef DEBUG
		cout << "[prefix] " << m->prefix << endl;
#endif
		msgstart = pos+1;
	}

	// extract irc command
	size_t paramstart = msg.find(" ", msgstart) + 1;
	m->command = msg.substr(msgstart, paramstart - 1 - msgstart);
#ifdef DEBUG
	cout << "[cmd] " << m->command << endl;
#endif

	m->pcount = 0;
	while(m->pcount < 15) {
		// first check for ':' param prefix
		// this means that this is the last parameter
		if((msg[paramstart] == ':')) {
			m->param[m->pcount] = msg.substr(
					paramstart+1,msg.length()-1-paramstart);
			m->pcount++;
			break;
		}
		// try to localise next parameter
		// if not present ... we know this is the last
		size_t next = next = msg.find(" ", paramstart);
		if((next == string::npos)) {
			m->param[m->pcount] = msg.substr(
					paramstart,
					msg.length() - paramstart
				);
			m->pcount++;
			break;
		}
		// there are more than this parameter so
		// set paramstart to the next parameter
		m->param[m->pcount] = msg.substr(paramstart, next-paramstart);
		paramstart = next+1;
		m->pcount++;
	}
#ifdef DEBUG
	cout << "[pcount] " << m->pcount << endl;
	for(size_t i = 0; i < m->pcount; i++)
		cout << "[param][" << i << "] " << m->param[i] << endl;
#endif

	return m;
}

/********************************************************************/

string
Irc::nextMessage() {
	string tmp = buffer;
	size_t start = 0;	
	size_t end = tmp.find(crlf);
	
	/* first, check if we already have an IRC message in the buffer */
	if(end != string::npos) {
		// extract the message
		string msg = tmp.substr(start,end-start);
		// set start to point 2 bytes after the message
		// we just extracted
		start = end + 2; 
		
		// now check whether the buffer was holding more 
		// data than that message extracted
		if(start < tmp.length()) { // yes
			// calculate new size of buffer where the 
			// message extracted is but out
			size_t newsize = tmp.length() - start;
			// create a temporary string that holds 
			// the contents of the new buffer
			string newbuf = tmp.substr(start,newsize);
			// copy the contents of the new buffer to
			// the buffer variable and reset remaining
			// characters to 0 bytes
			strncpy(buffer, newbuf.c_str(), newsize);
			memset(buffer+newsize, 0, 512-newsize+1);
			cout << "buffer = '" << buffer << "'" << endl;
			// update the buffer index to reflect the
			// changes
			buffer_index = static_cast<int>(newsize);
		} else { // no
			// update buffer index to point to beginning
			// of buffer
			buffer_index = 0;
			// and reset the contents of the buffer
			// to 0 bytes
			memset(buffer, 0x0, 512+1);
		}
		// return the message extracted
		return msg;
	}
	
	int read = 0;  // used to test whether we are at EOF (socket closed)
	while((buffer_index += read = 
				recv(sock, buffer+buffer_index, 512-buffer_index, 0)) != -1) {
		tmp = buffer;
		start = 0;
		end = tmp.find(crlf);
		
		/* check if we have an IRC message in the buffer */
		if(end != string::npos) {
			// extract the message
			string msg = tmp.substr(start,end-start);
			// set start to point 2 bytes after the message
			// we just extracted
			start = end + 2; 
			
			// now check whether the buffer was holding more 
			// data than that message extracted
			if(start < tmp.length()) { // yes
				// calculate new size of buffer where the 
				// message extracted is but out
				size_t newsize = tmp.length() - start;
				// create a temporary string that holds 
				// the contents of the new buffer
				string newbuf = tmp.substr(start,newsize);
				// copy the contents of the new buffer to
				// the buffer variable and reset remaining
				// characters to 0 bytes
				strncpy(buffer, newbuf.c_str(), newsize);
				memset(buffer+newsize, 0, 512-newsize+1);
				cout << "buffer = '" << buffer << "'" << endl;
				// update the buffer index to reflect the
				// changes
				buffer_index = static_cast<int>(newsize);
			} else { // no
				// update buffer index to point to beginning
				// of buffer
				buffer_index = 0;
				// and reset the contents of the buffer
				// to '\0' bytes
				memset(buffer, 0x0, 512+1);
			}
			// return the message extracted
			return msg;
		}
		/* if the socket was closed read will be 0 (indicates EOF) */
		if(read == 0) throw ConnectionClosedException();
	} // while
	// TODO error handling
	// the line below will cause segmentation fault
	return NULL;
}

/********************************************************************/
void
Irc::eventLoop() {
	while(1) {
		try {
			// read next IRC message from server
			string msg = nextMessage();
			#ifdef DEBUG
			cout << "[msg] " << msg << endl;
			#endif 
			// parse the message into a IrcMessage struct
			IrcMessage *m = parseMessage(msg);
			// examine/process the message and 
			// emit appropriate signals
			processMessage(m);
			delete m;
		} catch (ConnectionClosedException) {
			throw ConnectionClosedException();
		}
	}
}

/********************************************************************/

string Irc::loQuote(const string msg) {
	const string q("\0\r\n\020");

	// TODO optimize (important?)
	
	string tmp("");
	for(size_t i = 0; i < msg.length(); i++) {
		if(q.find(msg[i]) != string::npos)
			tmp += '\020';
		tmp += msg[i];
	}
	return tmp;
}

/********************************************************************/

bool 
Irc::loUnQuote(const string msg, string &result) {
	size_t last = 0;
	size_t f = 0;
	
	result = "";
	while((f = msg.find('\020', last)) != string::npos) {
		switch(msg[f+1]) {
			case '\0':
			case '\r':
			case '\n':
			case '\020': {
				result += msg.substr(last, f - last);
				result += msg[f+1];
				last = f+2;
				break;
			}
			default:
				return false;
		}
	}
	result += msg.substr(last, msg.length() - last + 1);
}

/********************************************************************/

void Irc::notice(const string whom, const string msg) {
	sendMessage("NOTICE", whom.c_str(), msg.c_str(), 0);
}

/********************************************************************/

void 
Irc::nick(const string nick) {
	sendMessage("NICK", nick.c_str(), 0);
}

/********************************************************************/

void 
Irc::pass(const string password) {
	sendMessage("PASS", password.c_str(), 0);
}

/********************************************************************/

void 
Irc::user(const string user, const string mode, const string realname) {
	sendMessage("USER", user.c_str(), mode.c_str(), "*", realname.c_str(), 0);
}

/********************************************************************/
void 
Irc::oper(const string name, const string password) {
	sendMessage("OPER", name.c_str(), password.c_str(), 0);
}

/********************************************************************/

void 
Irc::userMode(const string nick, const string modestring) {
	sendMessage("MODE", nick.c_str(), modestring.c_str(), 0);
}

/********************************************************************/

void 
Irc::quit(const string message) {
	if(message != "")	sendMessage("QUIT", message.c_str(), 0);
	else sendMessage("QUIT", 0);
}

/********************************************************************/

void 
Irc::join(const string channel, const string key) {
	if(key != "") sendMessage("JOIN", channel.c_str(), key.c_str(), 0);
	else sendMessage("JOIN", channel.c_str(), 0);
}

/********************************************************************/

void 
Irc::pong(const string id) {
	sendMessage("PONG", id.c_str(), 0);
}

/********************************************************************/

void 
Irc::privmsg(const string whom, const string msg) {
	sendMessage("PRIVMSG", whom.c_str(), msg.c_str(), 0);
}

/********************************************************************/

void 
Irc::channelOp(const string channel, const string nick, bool op) {
	sendMessage("MODE", channel.c_str(), (op ? "+o" : "-o"), 
			nick.c_str(), 0);	
}

/********************************************************************/
bool
Irc::extractFromHostmask(const string whom, string &nick, 
				string &user, string &host) 
{
	// a hostmask is composed like
	// nick!username@hostname
	size_t sep1 = whom.find("!");
	if(sep1 == string::npos) return false;
	size_t sep2 = whom.find("@", sep1+1);
	if(sep2 == string::npos) return false;

	nick = whom.substr(0, sep1);
	user = whom.substr(sep1+1, sep2-sep1-1);
	host = whom.substr(sep2+1, whom.length()-sep2);

	return true;
}

/********************************************************************/

Irc::IrcMessage *
Irc::waitForMessage(const string command) {
	// maybe put a threshold here .. like give
	// up when read 100 messages ?
	while(1) {
		string msg = nextMessage();
		#ifdef DEBUG
		cout << "[msg] " << msg << endl;
		#endif 
		IrcMessage *m = parseMessage(msg);
		if(command.find(m->command)) return m;
		else processMessage(m);
		delete m;
	}
}

/********************************************************************/

void 
Irc::topic(const string channel, const string topic) {
	sendMessage("TOPIC", channel.c_str(), topic.c_str(), NULL);
}

/********************************************************************/

string 
Irc::getChannelTopic(const string channel) {
/*
	331    RPL_NOTOPIC "<channel> :No topic is set"
	332    RPL_TOPIC "<channel> :<topic>"
	442    ERR_NOTONCHANNEL "<channel> :You're not on that channel
	482    ERR_CHANOPRIVSNEEDED "<channel> :reason"
	477    ERR_NOCHANMODES "<channel> :Channel doesn't support modes"
*/
	sendMessage("TOPIC", channel.c_str(), 0);
	IrcMessage *m = waitForMessage("331 332 442 482 477");
	if(m->param[1] == channel) {
		string topic( m->param[2]); 
		if(m->command == "332")	return topic;
		if(m->command == "331") return "";
		if(m->command == "442") throw NotOnChannelException();
		if(m->command == "482") throw ChanopPrivsNeededException();
		if(m->command == "477") throw NoChanModesException();
	}
	processMessage(m);
	return "";
}

/********************************************************************/

void Irc::part(const string channel, const string reason) {
	if(reason != "")
		sendMessage("PART", channel.c_str(), reason.c_str(), 0);
	else
		sendMessage("PART", channel.c_str(), 0);
}

/********************************************************************/

