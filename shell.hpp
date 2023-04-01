#ifndef __shell_hpp__
#define __shell_hpp__

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
        UnixServer = 5
    };

    enum class CommandArgument: std::int32_t {
        Role = 1,
        ProtocolType,
        ServerIp,
        ServerPort,
        ConnectionRetryInterval,
        Help
    };

    class UdpClient;
    class TcpClient;
    class UnixClient;
    class CommandOptions;
    class ConnectionHandler;

    class Shell;

    class Shell {
        public:
            Shell() {
                m_fds = {{0, 0}, {0, 0}};
            }

            ~Shell() {

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

                std::cout << "Reception thread " << std::endl;
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
                            std::cout << "Pushing into vector " << std::endl;
                            response.push_back(arr);
                        }
                    }
                }
                if(response.size()) {
                    for(auto const &elm: response) {
                        std::string data(reinterpret_cast<const char *>(elm.data()), elm.size());
                        std::cout << "The Command output is ====>>>>> "<< std::endl << data.c_str() <<std::endl;
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
                std::cout << "Enter Command now " << std::endl;
                std::cout << "I am inside Child Process" << std::endl;

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
                    std::cout << "Failed to send Command to executable " << std::endl;
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
                ACE_Get_Opt opts(argc, argv, ACE_TEXT ("r:f:i:p:a:h:"), 1);

                opts.long_option(ACE_TEXT("role"),                      'r', ACE_Get_Opt::ARG_REQUIRED);
                opts.long_option(ACE_TEXT("protocol"),                  'f', ACE_Get_Opt::ARG_REQUIRED);
                opts.long_option(ACE_TEXT("server-ip"),                 'i', ACE_Get_Opt::ARG_REQUIRED);
                opts.long_option(ACE_TEXT("server-port"),               'p', ACE_Get_Opt::ARG_REQUIRED);
                opts.long_option(ACE_TEXT("connection-retry-interval"), 'c', ACE_Get_Opt::ARG_REQUIRED);
                opts.long_option(ACE_TEXT("help"),                      'h', ACE_Get_Opt::ARG_REQUIRED);

                m_opts = opts;
            }

            auto processOptions() {
                int c = 0;
                while((c = opts ()) != EOF) {
                    switch(c) {
                        case 'r': //Role
                        {
                            m_commandArgumentValue.insert(std::make_pair(CommandArgument::Role, std::string(opts.opt_arg())));
                            ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [config:%t] %M %N:%l Role %s\n"), m_commandArgumentValue[CommandArgument::Role]));
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
                            m_commandArgumentValue.insert(std::make_pair(CommandArgument::Protocol, std::string(opts.opt_arg())));
                            ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [config:%t] %M %N:%l Protocol: %s\n"), m_commandArgumentValue[CommandArgument::Protocol]));
                        break;

                        case 'c': //Connection-retry-interval
                            m_commandArgumentValue.insert(std::make_pair(CommandArgument::ConnectionRetryInterval, std::string(opts.opt_arg())));
                            ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [config:%t] %M %N:%l Connection Retry Interval: %s\n"), m_commandArgumentValue[CommandArgument::ConnectionRetryInterval]));
                        break;

                        case 'h': //Help
                        default:
                            ACE_ERROR_RETURN ((LM_ERROR,
                                ACE_TEXT("%D [Config:%t] %M %N:%l usage: %s\n"
                                " [-i --server-ip]\n"
                                " [-p --server-port]\n"
                                " [-r --role {client|server}]\n"
                                " [-f --protocol {tcp|udp|unix}\n"
                                " [-c --connection-retry-interval {For client role}\n"
                                " [-h --help]\n"), argv [0]), -1);
                    }
                }
            }

            std::string role() const {
                try {
                    return(m_commandArgumentValue[CommandArgument::Role]);
                } catch(...) {
                    return(std::string());
                }
            }

            std::string ip() const {
                try {
                    return(m_commandArgumentValue[CommandArgument::ServerIp]);
                } catch(...) {
                    return(std::string());
                }
            }

            std::uint16_t port() const {
                try {
                    return(std::stoi(m_commandArgumentValue[CommandArgument::ServerPort]));
                } catch(...) {
                    return(std::string());
                }
            }

            std::string protocol() const {
                try {
                    return(m_commandArgumentValue[CommandArgument::ProtocolType]);
                }catch(...) {
                    return(std::string());
                }
            }

            std::uint32_t connectionRetryInterval() const {
                try {
                    return(std::stoi(m_commandArgumentValue[CommandArgument::ConnectionRetryInterval]));
                }catch(...) {
                    return(std::string());
                }
            }

        private:
            /* The last argument tells from where to start in argv - offset of argv array */
            ACE_Get_Opt m_opts;
            std::unordered_map<CommandArgument, std::string> m_commandArgumentValue;
    };

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
            auto rx(const std::string& in);
            auto to(auto in);
        private:    
            
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

    class TcpServer {
        public:
            auto rx(const std::string& in);
            auto to(auto in);
            ACE_INT32 handle_signal(int signum, siginfo_t *s, ucontext_t *u) override;
    };

    class UnixServer: public ACE_Task<ACE_MT_SYNCH> {
        public:
            int svc(void) override;
            int open(void *args=0) override;
            int close(u_long flags=0) override;

            ACE_INT32 handle_signal(int signum, siginfo_t *s, ucontext_t *u) override;

        private:
    };

    class ConnectionHandler: public ACE_Event_Handler {
        public:
            ConnectionHandler(auto role) {
                switch(role) {
                    case Role::UdpClient:
                        m_role = std::make_unique<UdpClient>();
                    break;
                    case Role::TcpClient:
                        m_role = std::make_unique<TcpClient>();
                    break;
                    case Role::UnixClient:
                        m_role = std::make_unique<UnixClient>();
                    break;
                    case Role::TcpServer:
                        m_role = std::make_unique<TcpServer>();
                    break;
                }
            }

            ACE_INT32 handle_timeout(const ACE_Time_Value &tv, const void *act=0) override;
            ACE_INT32 handle_input(ACE_HANDLE handle) override;
            ACE_INT32 handle_signal(int signum, siginfo_t *s = 0, ucontext_t *u = 0) override;
            ACE_INT32 handle_close (ACE_HANDLE = ACE_INVALID_HANDLE, ACE_Reactor_Mask = 0) override;
            ACE_HANDLE get_handle() const override;
            ACE_INT32 tx(const std::string& rsp, ACE_HANDLE handle);

        private:
            long m_timerId;
            ACE_HANDLE m_handle;
            ACE_INET_Addr m_connAddr;
            std::variant<std::unique_ptr<TcpClient>, std::unique_ptr<UdpClient>, std::unique_ptr<UnixClient>, std::unique_ptr<TcpServer>> m_role;


    };

    class Connections {
        public:
            Connections() {}
            ~Connections() {}

        private:
            std::unordered_map<std::int32_t, ConnectionHandler> m_connections;
            std::variant<UdpClient, TcpClient, UnixClient, UdpServer, TcpServer, UnixServer> m_role;
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
                    //dump();
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















#endif /* __shell_hpp__*/