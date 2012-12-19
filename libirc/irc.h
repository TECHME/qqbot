/* $ID$ */
#ifndef _IRC_H
#define _IRC_H

#include <boost/function.hpp>

#ifndef WIN32
#include <sys/socket.h>
#include <netdb.h>
#else
#include <WinSock.h>
#endif
#include <stdio.h>
#include <string.h>
#include <string>

using namespace std;


class Irc {
	private:
		struct sockaddr_in *addr;
		int sock;
		string crlf;
		char *buffer;
		int buffer_index;
    public:
		struct IrcMessage {
			bool have_prefix;
			string prefix;
			string command;
			string param[15];
			size_t pcount;
		};
        typedef boost::function<void(const IrcMessage & pMsg)> privmsg_cb;
        privmsg_cb cb;

	protected:
		struct hostent *host;
		void sendMessage(const string cmd, const char *param, ...) const;
		IrcMessage parseMessage(const string msg);
		IrcMessage waitForMessage(const string command);
		void processMessage(IrcMessage m);
		/** finds CTCP data in msg
		 * \param msg second parameter from a privmsg
		 * \param ctcp will hold the CTCP data in string if any
		 * \return true if CTCP data was present */
		bool findCTCP(string msg, string &ctcp);
		/** ctcp low-level quoting of a message */
		string loQuote(const string msg);
		/** ctcp low-lewel un-quoting of a message\
		 * \param msg message to unquote
		 * \param result reference to string to hold the unqouted result
		 * \return true if no errors accured, false otherwise (message should
		 * be dropped then) */
		bool loUnQuote(const string msg, string &result);

		/** reads and blocks until the next IRC message can be read
		 * from the current connection
		 * \throws ConnectionClosed
		 * \return IRC message read <b>not</b> including linefeed/newline */
		string nextMessage();
		
        //add by invxp
        string host2ip(const char* host);
	public:
		Irc(privmsg_cb cb);
		~Irc();

		/** attempts to establish a connection to an IRC server
		 * \param host hostname or ip of the server to connect to
		 * \param port port number of server */
		void connect(const string host, unsigned short port = 6667);

		/** disconnects from IRC server */
		void disconnect();
		
		void eventLoop();
	
		/** changes nickname to nick */
		void nick(const string nk);
		/** set connection password */
		void pass(const string password);
		/** 
		 * The USER command is used at the beginning of connection 
		 * to specify the username, hostname and realname of a new 
		 * user. 
		 * \param user username
		 * \param mode automatically set mode on connection (bitmask)
		 * \param realname real name, can contain spacesb */
		void user(const string u, const string mode, const string realname);
		
		/** used by normal users to gain IRC operator status */
		void oper(const string name, const string password);

		void userMode(const string nick, const string modestring);

		/** terminate client session with server 
		 * \param message optional message/reason to send the IRC network */
		void quit(const string message = "");

		/** starts listening to channed.
		 * \param channel name of channel to listen to
		 * \param key key/password to join channel (optional) */
		void join(const string channel, const string key = "");

		/** respond to PING
		 * \param id parameter received from PING command */
		void pong(const string id);

		/** sends message to whom
		 * \param whom irc nickname / channelname
		 * \param msg message to send */
		void privmsg(const string whom, const string msg);
	
		/** sends notice to whom
		 * \param whom receiver of notice
		 * \param msg message to send */
		void notice(const string whom, const string msg);
		
		/** split irc hostmask into nickname, username and
		 * hostname
		 * \param whom string to parse
		 * \param nick string to receive nickname
		 * \param user string to receive username
		 * \param host string to receive hostname
		 * \return true on succes, false otherwise */
		static bool extractFromHostmask(const string whom, string &nick, 
				string &user, string &host);
	
		/** gives/takes channel operator status
		 * \param channel channel to give/take op
		 * \param nick irc nickname to give op
		 * \param give true = gives op; false = takes op */
		void channelOp(const string channel, const string nick, bool give);

		/** sets topic of an irc channel
		 * \param channel channel subject to change
		 * \param topic new topic string */
		void topic(const string channel, const string top);

		/** queries IRC server for current topic on channel
		 * \throws NotOnChannelException
		 * \throws ChanopPrivsNeededException
		 * \throws NoChanModesException
		 * \return topic for channel */
		string getChannelTopic(const string channel) ;

		/** parts a channel
		 * \param channel channel to leave
		 * \param reason optional reason for leaving */
		void part(const string channel, const string reason = "");

		/* ---- exceptions ---- */
		class IrcException { };
		class UnknownHostException : public IrcException { };
		class NotOnChannelException : public IrcException { };
		class ChanopPrivsNeededException : public IrcException { };
		class NoChanModesException : public IrcException { };
		class ConnectionClosedException : public IrcException { };
};

#endif
