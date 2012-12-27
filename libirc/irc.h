#pragma once

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>

struct IrcMsg
{
    std::string whom;
    std::string from;
    std::string locate;
    std::string msg;
};

typedef boost::function<void(const IrcMsg& msg)> privmsg_cb;

class IrcClient
{
public:
    IrcClient(boost::asio::io_service &io_service,const std::string& user,const std::string& user_pwd="",const std::string& server="irc.freenode.net", const std::string& port="6667");
    ~IrcClient();
private:
    void handle_read_request(const boost::system::error_code& err, std::size_t readed);
    void handle_write_request(const boost::system::error_code& err, std::size_t writed);
    void handle_connect_request(const boost::system::error_code& err);
    void send_request(const std::string& msg);
    void process_request(std::size_t readed);
public:

    void login(const privmsg_cb &cb);
    void join(const std::string& ch,const std::string &pwd="");
    void chat(const std::string& whom,const std::string& msg);
    void send_command(const std::string& cmd);
    void oper(const std::string& user,const std::string& pwd);

private:
    boost::asio::ip::tcp::resolver  resolver_;
    boost::asio::ip::tcp::socket    socket_;
    boost::asio::streambuf          request_;
    boost::asio::streambuf          response_;
    privmsg_cb                      cb_;
    std::string                     user_;
    std::string                     pwd_;
    std::string                     server_;
    std::string                     port_;
    bool                            login_;
    std::list<std::string>          msg_queue_;
    boost::recursive_mutex          msg_queue_lock_;
};