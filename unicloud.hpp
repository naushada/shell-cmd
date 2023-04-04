#ifndef __unicloud_hpp__
#define __unicloud_hpp__

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <array>
#include <iostream>
#include <cstdio>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <variant>
#include <memory>


#include "ace/Reactor.h"
#include "ace/Basic_Types.h"
#include "ace/Event_Handler.h"
#include "ace/Task.h"
#include "ace/INET_Addr.h"
#include "ace/SOCK_Stream.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/Task_T.h"
#include "ace/Timer_Queue_T.h"
#include "ace/Reactor.h"
#include "ace/OS_Memory.h"
#include "ace/Thread_Manager.h"
#include "ace/Get_Opt.h"
#include "ace/Signal.h"
#include "ace/SSL/SSL_SOCK.h"
#include "ace/SSL/SSL_SOCK_Stream.h"
#include "ace/SSL/SSL_SOCK_Connector.h"
#include "ace/Semaphore.h"
#include "ace/Barrier.h"


/**
 * @brief assakeena - the Spirit of Tranquility, or Peace of Reassurance
 * 
 * @date 31st Mar 2023
 */
namespace assakeena {

    enum class FDs : std::uint16_t {
        READ = 0,
        WRITE = 1,
        INVALID = 2
    };

    enum class Role: std::uint32_t {
        UdpClient = 0,
        TcpClient = 1,
        UnixClient = 2,
        UdpServer = 3,
        TcpServer = 4,
        UnixServer = 5,
        ShellCommandHandler = 6,
        All = 7
    };

    enum class CommandArgument: std::int32_t {
        Role = 1,
        Protocol,
        ServerIp,
        ServerPort,
        ConnectionRetryInterval,
        ResponseTimeout,
        ConnectionRetryTimeout,
        ConnectionRetryCount,
        ConnectionCloseTimeout,
        SelfIp,
        SelfPort,
        Help
    };

    class UdpClient;
    class TcpClient;
    class UnixClient;
    class TcpServer;
    class UdpServer;
    class UnixServer;
    class CommandOptions;
    class ServiceHandler;
    class UnicloudMgr;
    class ShellCommandHandler;

    class ShellCommandHandler {
        public:
            ShellCommandHandler() {
                m_fds = {{0, 0}, {0, 0}};
            }

            ~ShellCommandHandler() {

            }

            void fds(std::array<std::int32_t, 2> fds) {
                m_fds.push_back(fds);
            }

            auto& fds() {
                return(m_fds);
            }
            auto fds(auto vectorOffset, auto arrayOffset) {
                return(m_fds.at(vectorOffset).at(arrayOffset));
            }

            void fds(auto vectorOffset, auto arrayOffset, auto value) {
                m_fds.at(vectorOffset).at(arrayOffset) = value;
            }

            void display_fds() const {
                for(auto const &elm: m_fds) {
                    std::cout << "["<< std::get<0>(elm) << ", " << std::get<1>(elm) << "]" << std::endl;
                    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [ShellCommandHandler:%t] %M %N:%l m_fds %d m_fds %d\n"), std::get<0>(elm), std::get<1>(elm)));
                }
            }
            /**
             * @brief 
             * 
             * @param channel 
             * @return auto 
             */
            auto rx(const auto& channel) {

                std::array<char, 1024> arr;
                std::vector<std::array<char, 1024>> response;
                ssize_t len = -1;
                fd_set fdset;

                ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [ShellCommandHandler:%t] %M %N:%l reception thread\n")));
                response.clear();
                FD_ZERO(&fdset);
                arr.fill(0);
                
                while(true) {
                    FD_SET(channel, &fdset);
                    struct timeval to = {0,100};
                    auto ret = select((channel + 1), &fdset, NULL, NULL, &to);

                    if(ret <= 0) {
                        break;
                    }

                    if(FD_ISSET(channel, &fdset)) {
                        len = read(channel, (void *)arr.data(), (size_t)arr.size());

                        if(len > 0) {
                            ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [ShellCommandHandler:%t] %M %N:%l pushing into vector\n")));
                            response.push_back(arr);
                        }
                    }
                }
                if(response.size()) {
                    for(auto const &elm: response) {
                        std::string data(reinterpret_cast<const char *>(elm.data()), elm.size());
                        ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [ShellCommandHandler:%t] %M %N:%l command output is: %s\n"), data.c_str()));
                    }
                } else {
                    //std::cout << "read is failed " << std::endl;
                }
                return(response);
            }

            /**
             * @brief 
             * 
             * @param cmd 
             * @return auto 
             */
            auto tx(const auto& cmd) {
            
                std::cout <<std::endl;
                ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [ShellCommandHandler:%t] %M %N:%l child process\n")));

                std::string cmd;
                std::getline(std::cin, cmd);
                std::stringstream ss;

                ss << cmd.data();
                if(ss.str().empty()) {
                    continue;
                }

                ss << "\n";
                std::int32_t len = write(fds(assakeena::FDs::WRITE, assakeena::FDs::WRITE), reinterpret_cast<const char *>(ss.str().c_str()), ss.str().length());
                if(len <= 0) {
                    ACE_ERROR((LM_ERROR, ACE_TEXT("%D [ShellCommandHandler:%t] %M %N:%l failed to send the command to executable\n")));
                    exit(0);
                }
            }

        private:
            std::vector<std::array<std::int32_t, 2>> m_fds;
    };

    class CommandOptions {
        public:
            CommandOptions(std::int32_t argc, char* argv[]) {
                config(argc, argv);
            }
            ~CommandOptions() {

            }

            /**
             * @brief 
             * 
             * @param argc 
             * @param argv 
             */
            void config(std::int32_t argc, char* argv[]) {
                ACE_Get_Opt opts(argc, argv, ACE_TEXT ("r:f:i:p:c:o:n:e:s:l:h:"), 1);
                //opts(argc, argv, ACE_TEXT ("r:f:i:p:c:o:n:e:s:l:h:"), 1);

                opts.long_option(ACE_TEXT("role"),                            'r', ACE_Get_Opt::ARG_REQUIRED);
                opts.long_option(ACE_TEXT("protocol"),                        'f', ACE_Get_Opt::ARG_REQUIRED);
                opts.long_option(ACE_TEXT("server-ip"),                       'i', ACE_Get_Opt::ARG_REQUIRED);
                opts.long_option(ACE_TEXT("server-port"),                     'p', ACE_Get_Opt::ARG_REQUIRED);
                opts.long_option(ACE_TEXT("self-ip"),                         's', ACE_Get_Opt::ARG_REQUIRED);
                opts.long_option(ACE_TEXT("self-port"),                       'l', ACE_Get_Opt::ARG_REQUIRED);
                opts.long_option(ACE_TEXT("connection-retry-count"),          'n', ACE_Get_Opt::ARG_REQUIRED);
                opts.long_option(ACE_TEXT("response-timeout-in-ms"),          'e', ACE_Get_Opt::ARG_REQUIRED);
                opts.long_option(ACE_TEXT("connection-retry-timeout-in-ms"),  'c', ACE_Get_Opt::ARG_REQUIRED);
                opts.long_option(ACE_TEXT("connection-close-timeout-in-ms"),  'o', ACE_Get_Opt::ARG_REQUIRED);
                opts.long_option(ACE_TEXT("help"),                            'h', ACE_Get_Opt::ARG_REQUIRED);

                //m_opts = opts;
                processOptions(opts);
            }

            std::int32_t processOptions(ACE_Get_Opt& opts) {
                int c = 0;
                while((c = opts()) != EOF) {
                    switch(c) {
                        case 'r': //Role
                        {
                            auto role_value = std::string(opts.opt_arg());
                            if(!role_value.compare("client") || !role_value.compare("server") || !role_value.compare("all") ||
                               !role_value.compare("shell-command")) {
                                m_commandArgumentValue.insert(std::make_pair(CommandArgument::Role, std::string(opts.opt_arg())));
                                ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [config:%t] %M %N:%l Role %s\n"), m_commandArgumentValue[CommandArgument::Role]));
                            }
                        }
                        break;

                        case 'p': //PORT
                        {
                            m_commandArgumentValue.insert(std::make_pair(CommandArgument::ServerPort, std::string(opts.opt_arg())));
                            ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [config:%t] %M %N:%l Server Port %s\n"), m_commandArgumentValue[CommandArgument::ServerPort]));
                        }
                        break;

                        case 'i': //IP
                        {
                            m_commandArgumentValue.insert(std::make_pair(CommandArgument::ServerIp, std::string(opts.opt_arg())));
                            ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [config:%t] %M %N:%l Server IP: %s\n"), m_commandArgumentValue[CommandArgument::ServerIp]));
                        }
                        break;

                        case 'f': //Protocol
                        {
                            auto protocol = std::string(opts.opt_arg());
                            if(!protocol.compare("tcp") || !protocol.compare("udp") || !protocol.compare("unix") || !protocol.compare("all")) {
                                m_commandArgumentValue.insert(std::make_pair(CommandArgument::Protocol, std::string(m_opts.opt_arg())));
                                ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [config:%t] %M %N:%l Protocol: %s\n"), m_commandArgumentValue[CommandArgument::Protocol]));
                            }
                        }
                        break;

                        case 'c': //Connection-retry-interval
                        {
                            m_commandArgumentValue.insert(std::make_pair(CommandArgument::ConnectionRetryInterval, std::string(opts.opt_arg())));
                            ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [config:%t] %M %N:%l Connection Retry Interval: %s\n"), m_commandArgumentValue[CommandArgument::ConnectionRetryInterval]));
                        }
                        break;

                        case 'o': //Connection-close-timeout
                        {
                            m_commandArgumentValue.insert(std::make_pair(CommandArgument::ConnectionCloseTimeout, std::string(opts.opt_arg())));
                            ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [config:%t] %M %N:%l Connection Close Timeout: %s\n"), m_commandArgumentValue[CommandArgument::ConnectionCloseTimeout]));
                        }
                        break;

                        case 'n': //connection-retry-count
                        {
                        
                            m_commandArgumentValue.insert(std::make_pair(CommandArgument::ConnectionRetryCount, std::string(opts.opt_arg())));
                            ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [config:%t] %M %N:%l Connection Retry Count: %s\n"), m_commandArgumentValue[CommandArgument::ConnectionRetryCount]));
                        }
                        break;

                        case 'e': //response-timeout
                        {
                            m_commandArgumentValue.insert(std::make_pair(CommandArgument::ResponseTimeout, std::string(opts.opt_arg())));
                            ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [config:%t] %M %N:%l Connection Retry Interval: %s\n"), m_commandArgumentValue[CommandArgument::ResponseTimeout]));
                        }
                        break;

                        case 's': //self-ip
                        {
                            m_commandArgumentValue.insert(std::make_pair(CommandArgument::SelfIp, std::string(opts.opt_arg())));
                            ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [config:%t] %M %N:%l Self IP: %s\n"), m_commandArgumentValue[CommandArgument::SelfIp]));
                        }
                        break;

                        case 'l': //self-port
                        {
                            m_commandArgumentValue.insert(std::make_pair(CommandArgument::SelfPort, std::string(opts.opt_arg())));
                            ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [config:%t] %M %N:%l Self PORT: %s\n"), m_commandArgumentValue[CommandArgument::SelfPort]));
                        }
                        break;

                        case 'h': //Help
                        default:
                        {
                            ACE_ERROR_RETURN ((LM_ERROR,
                                ACE_TEXT("%D [Config:%t] %M %N:%l usage: %s\n"
                                " [-i --server-ip]\n"
                                " [-p --server-port]\n"
                                " [-r --role {client|server|shell-command-handler|all}]\n"
                                " [-f --protocol {tcp|udp|unix|all}\n"
                                " [-c --connection-retry-interval {For client role}\n"
                                " [-o --connection-close-timeout-in-ms {For tcp server}\n"
                                " [-n --connection-retry-count {For tcp client}\n"
                                " [-e --response-timeout-in-ms {Applicable to all}\n"
                                " [-s --self-ip   {}\n"
                                " [-l --self-port {}\n"
                                " [-h --help]\n"), opts.argv[0]), -1);
                        }
                    }
                }
                return(0);
            }

            std::string role() {
                try {
                    return(m_commandArgumentValue[CommandArgument::Role]);
                } catch(...) {
                    return(std::string());
                }
            }

            std::string ip() {
                try {
                    return(m_commandArgumentValue[CommandArgument::ServerIp]);
                } catch(...) {
                    return(std::string());
                }
            }

            std::uint16_t port() {
                try {
                    return(std::stoi(m_commandArgumentValue[CommandArgument::ServerPort]));
                } catch(...) {
                    return(0);
                }
            }

            std::string protocol() {
                try {
                    return(m_commandArgumentValue[CommandArgument::Protocol]);
                } catch(...) {
                    return(std::string());
                }
            }

            std::uint32_t connectionRetryInterval() {
                try {
                    return(std::stoi(m_commandArgumentValue[CommandArgument::ConnectionRetryInterval]));
                } catch(...) {
                    return(0);
                }
            }

            std::uint32_t connectionRetryCount() {
                try {
                    return(std::stoi(m_commandArgumentValue[CommandArgument::ConnectionRetryCount]));
                } catch(...) {
                    return(0);
                }
            }

            std::uint32_t connectionRetryTimeout() {
                try {
                    return(std::stoi(m_commandArgumentValue[CommandArgument::ConnectionRetryTimeout]));
                } catch(...) {
                    return(0);
                }
            }

            std::uint32_t responseTimeout() {
                try {
                    return(std::stoi(m_commandArgumentValue[CommandArgument::ResponseTimeout]));
                } catch(...) {
                    return(0);
                }
            }

            std::uint32_t connectionCloseTimeout() {
                try {
                    return(std::stoi(m_commandArgumentValue[CommandArgument::ConnectionCloseTimeout]));
                } catch(...) {
                    return(0);
                }
            }

            std::uint32_t selfIP() {
                try {
                    return(std::stoi(m_commandArgumentValue[CommandArgument::SelfIp]));
                } catch(...) {
                    return(0);
                }
            }

            std::uint32_t selfPORT() {
                try {
                    return(std::stoi(m_commandArgumentValue[CommandArgument::SelfPort]));
                } catch(...) {
                    return(0);
                }
            }

        private:
            /* The last argument tells from where to start in argv - offset of argv array */
            ACE_Get_Opt m_opts;
            std::unordered_map<CommandArgument, std::string> m_commandArgumentValue;
    };

#if 0
    class UdpClient: public ACE_Task<ACE_MT_SYNCH> {
        public:
            int svc(void) override;
            int open(void *args=0) override;
            int close(u_long flags=0) override;

            ACE_INT32 handle_signal(int signum, siginfo_t *s, ucontext_t *u) override;
        private:

    };

    class TcpClient {
        public:
            TcpClient() {

            }
            ~TcpClient() {

            }

            auto rx(const std::string& in);
            auto start_timer(auto in);
            auto stop_timer(auto in);
            auto start(const std::string& IP, const std::uint32_t &port);
            auto stop();

            //ACE_INT32 handle_timeout(const ACE_Time_Value &tv, const void *act=0) override;
            //ACE_INT32 handle_input(ACE_HANDLE handle) override;
            //ACE_INT32 handle_signal(int signum, siginfo_t *s = 0, ucontext_t *u = 0) override;
            //ACE_INT32 handle_close (ACE_HANDLE = ACE_INVALID_HANDLE, ACE_Reactor_Mask = 0) override;
            //ACE_HANDLE get_handle() const override;

            void handle(ACE_HANDLE channel) {
                m_handle = channel;
            }

            ACE_HANDLE handle() const {
                return(m_handle);
            }
            
            auto peer_address(auto addr) {
                m_addr = addr;
            }

            ACE_INET_Addr addr() const {
                return(m_addr);
            }

        private:    
            ACE_HANDLE m_handle;
            ACE_INET_Addr m_addr;
            long m_timerId;
    };

    class UnixClient: public ACE_Task<ACE_MT_SYNCH> {
        public:
            int svc(void) override;
            int open(void *args=0) override;
            int close(u_long flags=0) override;

            ACE_INT32 handle_signal(int signum, siginfo_t *s, ucontext_t *u) override;

        private:
    };

    class UdpServer: public ACE_Task<ACE_MT_SYNCH> {
        public:
            int svc(void) override;
            int open(void *args=0) override;
            int close(u_long flags=0) override;

            ACE_INT32 handle_signal(int signum, siginfo_t *s, ucontext_t *u) override;
        private:

    };
#endif

    class TcpConnectionTask : public ACE_Task<ACE_MT_SYNCH> {
        public:
            TcpConnectionTask(std::int32_t thread_num=3) : m_thread_count(thread_num), 
                m_rx_byte(0), 
                m_rx_count(0), 
                m_tx_byte(0),
                m_tx_count(0),
                m_barrier(nullptr) {
                    //m_uri_map = {{"/v1/api/exec-command", process_exec_command}};
                }
                
            ~TcpConnectionTask() {
                m_barrier.reset(nullptr);
            }
            int svc(void) override;
            int open(void *args=0) override;
            int close(u_long flags=0) override;

            std::int32_t rx(ACE_HANDLE channel);
            std::int32_t tx(const std::string &rsp, ACE_HANDLE handle);
            //std::double start_timer(std::int32_t to);
            //std::double stop_timer();
            std::string process_request(const std::string& req);
            //std::string process_exec_command(const std::string& req);
        private:
            std::int32_t m_thread_count;
            std::int32_t m_rx_byte;
            std::int32_t m_rx_count;
            std::int32_t m_tx_byte;
            std::int32_t m_tx_count;
            std::unique_ptr<ACE_Barrier> m_barrier;
            std::unordered_map<std::string, decltype([&](const std::string&))>m_uri_map;


    };

    class TcpServer : public ACE_Event_Handler {
        public:
            ~TcpServer() {
                m_task.reset(nullptr);
            }

            TcpServer(auto config) {
                std::string addr("");
                m_task_num = 3;
                if(config.ip()) {

                    addr = config.ip();
                    addr += ":";
                    addr += std::to_string(config.port());
                    m_listen.set_address(addr.c_str(), addr.length());

                } else {

                    m_listen.set_port_number(config.port());

                }

                /* Start listening for incoming connection */
                int reuse_addr = 1;
                if(m_server.open(m_listen, reuse_addr)) {
                    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [TcpServer:%t] %M %N:%l Starting of WebServer failed - opening of port: %d hostname: %s\n"), 
                                m_listen.get_port_number(), m_listen.get_host_name()));
                }

                handle(m_server.get_handle());
                m_connected_clients.clear();

                m_task = std::make_unique<TcpConnectionTask>(m_task_num);
                m_task->open();
            }

            ACE_HANDLE handle() const {
                return(m_handle);
            }

            void handle(ACE_HANDLE channel) {
                m_handle = channel;
            }

            ACE_INT32 handle_timeout(const ACE_Time_Value &tv, const void *act=0) override;
            ACE_INT32 handle_input(ACE_HANDLE handle) override;
            ACE_INT32 handle_signal(int signum, siginfo_t *s = 0, ucontext_t *u = 0) override;
            ACE_INT32 handle_close (ACE_HANDLE = ACE_INVALID_HANDLE, ACE_Reactor_Mask = 0) override;
            
            auto start();
            auto stop();

        private:
            ACE_HANDLE m_handle;
            ACE_Message_Block m_mb;
            ACE_SOCK_Stream m_stream;
            ACE_INET_Addr m_listen;
            ACE_SOCK_Acceptor m_server;
            //std::unordered_map<ACE_HANDLE, std::unique_ptr<TcpConnectionTask>> m_connections;
            std::unique_ptr<TcpConnectionTask> m_task;
            std::vector<std::int32_t> m_connected_clients;
            std::int32_t m_task_num;
    };

#if 0
    class UnixServer: public ACE_Task<ACE_MT_SYNCH> {
        public:
            UnixServer(auto config) {}
            ~UnixServer() {}

            int svc(void) override;
            int open(void *args=0) override;
            int close(u_long flags=0) override;

            ACE_INT32 handle_signal(int signum, siginfo_t *s, ucontext_t *u) override;

        private:
    };
#endif

    class ServiceHandler {
        public:
            ServiceHandler(auto role, auto config) {
                switch(role) {

                    case Role::UdpClient:
                        m_service = std::make_unique<UdpClient>(config);
                        m_handle = m_server->handle();
                    break;

                    case Role::TcpClient:
                        m_service = std::make_unique<TcpClient>(config);
                        m_handle = m_server->handle();
                    break;

                    case Role::UnixClient:
                        m_service = std::make_unique<UnixClient>(config);
                        m_handle = m_server->handle();
                    break;

                    case Role::TcpServer:
                        m_service = std::make_unique<TcpServer>(config);
                        m_handle = m_service->handle();
                    break;

                    case Role::UdpServer:
                        m_service = std::make_unique<UdpServer>(config);
                        m_handle = m_server->handle();
                    break;

                    case Role::UnixServer:
                        m_service = std::make_unique<UnixServer>(config);
                        m_handle = m_server->handle();
                    break;
                }
            }

            //ACE_INT32 handle_timeout(const ACE_Time_Value &tv, const void *act=0) override;
            //ACE_INT32 handle_input(ACE_HANDLE handle) override;
            //ACE_INT32 handle_signal(int signum, siginfo_t *s = 0, ucontext_t *u = 0) override;
            //ACE_INT32 handle_close (ACE_HANDLE = ACE_INVALID_HANDLE, ACE_Reactor_Mask = 0) override;
            //ACE_HANDLE get_handle() const override;
            
            ACE_HANDLE handle() const {
                return(m_handle);
            }

            ACE_INT32 tx(const std::string& response, ACE_HANDLE handle);
            ACE_INT32 start();
            ACE_INT32 stop();
            ACE_INT32 rx(ACE_HANDLE channel);

        private:

            long m_timerId;
            ACE_HANDLE m_handle;
            ACE_INET_Addr m_connAddr;
            std::variant<std::unique_ptr<TcpClient>, std::unique_ptr<UdpClient>, std::unique_ptr<UnixClient>, 
                        std::unique_ptr<TcpServer>, std::unique_ptr<UdpServer>, std::unique_ptr<UnixServer>> m_service;

    };

    class UnicloudMgr : public ACE_Event_Handler {
        public:
            UnicloudMgr(std::int32_t argc, char* argv[]) {
                m_config = std::make_shared<CommandOptions>(argc, argv);
                
                //arg1 = commandline arguments, arg2 = override arg1 <true|false>, arg3 = list ofservice <tcpserver, udpserver, etc>
                
                auto _role = m_config.role();
                if(_role.compare("all")) {
                    ServiceHandler svcHandler(_role, m_config);
                    auto result = m_services.insert_or_assign(std::make_pair(svcHandler.handle(), svcHandler));
                    if(result.second) {
                        //success 
                    }
                } else {
                    // configure all service Handler i.e. tcpClient/Server,UdpClient/Server,UnixClient/Server, Shell-Command-Handler 
                    
                }
            }

            ~UnicloudMgr() {}
            ACE_INT32 handle_timeout(const ACE_Time_Value &tv, const void *act=0) override;
            ACE_INT32 handle_input(ACE_HANDLE handle) override;
            ACE_INT32 handle_signal(int signum, siginfo_t *s = 0, ucontext_t *u = 0) override;
            ACE_INT32 handle_close (ACE_HANDLE = ACE_INVALID_HANDLE, ACE_Reactor_Mask = 0) override;
            ACE_INT32 start();
            ACE_INT32 stop();
            
        private:
            
            std::unordered_map<std::int32_t, ServiceHandler> m_services;
            std::shared_ptr<CommandOptions> m_config;
    }

    class Http {
        public:
            Http() {
                m_uri.clear();
                m_params.clear();
            }

            Http(const std::string& in) {
                m_uri.clear();
                m_params.clear();
                m_header.clear();
                m_body.clear();

                m_header = get_header(in);

                if(m_header.length()) {
                    parse_uri(m_header);
                    parse_header(m_header);
                }

                m_body = get_body(in);
            }

            ~Http() {
                m_tokenMap.clear();
            }

            std::string method() {
                return(m_method);
            }

            void method(std::string _method) {
                m_method = _method;
            }

            std::string uri() const {
                return(m_uri);
            }

            void uri(std::string _uri) {
                m_uri = _uri;
            }

            void add_element(std::string key, std::string value) {
                m_params.insert(std::pair(key, value));
            }

            std::string value(const std::string& key) {
                auto it = m_params.find(key);
                if(it != m_params.end()) {
                    return(it->second);
                }
                return std::string();
            }

            std::string body() {
                return m_body;
            }

            std::string header() {
                return m_header;
            }
            void format_value(const std::string& param);
            void parse_uri(const std::string& in);
            void parse_header(const std::string& in);

        private:
            std::unordered_map<std::string, std::string> m_params;
            std::string m_uri;
            std::string m_header;
            std::string m_body;
            std::string m_method;
    };
}















#endif /* __unicloud_hpp__*/