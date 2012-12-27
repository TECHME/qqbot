#pragma once

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/function.hpp>

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
    IrcClient(boost::asio::io_service &io_service, const privmsg_cb &cb,const std::string& server, const std::string& port);
    ~IrcClient();
private:
    void handle_read_request(const boost::system::error_code& err, std::size_t readed);
    void handle_write_request(const boost::system::error_code& err, std::size_t writed);
    void handle_connect_request(const boost::system::error_code& err);
    void send_request(const std::string& msg);
    void process_request(std::size_t readed);
public:
    void login(const std::string& user,const std::string& ch,const std::string& user_pwd="",const std::string& ch_pwd="");
    void chat(const std::string& whom,const std::string& msg);
    void send_command(const std::string& cmd);
    void oper(const std::string& user,const std::string& pwd);

private:
    boost::asio::ip::tcp::resolver  resolver_;
    boost::asio::ip::tcp::socket    socket_;
    boost::asio::streambuf          request_;
    boost::asio::streambuf          response_;
    privmsg_cb                      cb_;

};