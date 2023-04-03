#ifndef __unicloud_cc__
#define __unicloud_cc__


#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <array>
#include <iostream>
#include <cstdio>
#include <sstream>
#include <thread>

#include "shell.hpp"

ACE_INT32 assakeena::UnicloudMgr::handle_input(ACE_HANDLE channel) {

    if(m_services.contains(channel)) {
      m_services.at(channel).rx(channel);
    }

}

ACE_INT32 assakeena::UnicloudMgr::handle_timeout(const ACE_Time_Value &tv, const void *act) {
  return(0);
}

ACE_INT32 assakeena::UnicloudMgr::handle_signal(int signum, siginfo_t *s , ucontext_t *u) {
  return(0);
}

ACE_INT32 assakeena::UnicloudMgr::handle_close (ACE_HANDLE channel, ACE_Reactor_Mask mask) {
  return(0);
}

ACE_INT32 assakeena::UnicloudMgr::start() {

    /* subscribe for signal */
    ACE_Sig_Set ss;
    ss.empty_set();
    ss.sig_add(SIGINT);
    ss.sig_add(SIGTERM);

    ACE_Reactor::instance()->register_handler(&ss, this); 

    for(auto const& elm: m_services) {
        auto channel = elm->first;
        ACE_Reactor::instance()->register_handler(channel, this, ACE_Event_Handler::ACCEPT_MASK | 
                                                                 ACE_Event_Handler::TIMER_MASK |
                                                                 ACE_Event_Handler::SIGNAL_MASK); 
    }

    ACE_Time_Value to(0,10);

    while(true) {
        ACE_INT32 ret = ACE_Reactor::instance()->handle_events(to);
        if(ret < 0) break;
    }

    ACE_Reactor::instance()->remove_handler(ss); 
    return(0);
}

ACE_INT32 assakeena::UnicloudMgr::stop() {
    return(0);
}

//Service Handler ==================================>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

ACE_INT32 assakeena::ServiceHandler::tx(const std::string &rsp, ACE_HANDLE handle) {
    std::int32_t  toBeSent = rsp.length();
    std::int32_t offset = 0;
    ACE_INT32 ret = -1;

    do {
        ret = send(handle, (rsp.c_str() + offset), (toBeSent - offset), 0);
        if(ret < 0) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("%D [ConnectionHandler:%t] %M %N:%l sent to peer is failed\n")));
            break;
        }
        offset += ret;
        ret = 0;

    } while((toBeSent != offset));
    
    return(ret);
}

ACE_INT32 assakeena::ServiceHandler::rx(ACE_HANDLE handle) {

    std::int32_t ret = -1;
    fd_set rd_fd;
    FD_ZERO(&rd_fd);
    std::vector<std::string> req;
    req.clear();

    while(true) {

        struct timeval to = {0, 10};
        FD_SET(handle, &rd_fd);

        ret = select((handle + 1), &rd_fd, NULL, NULL, &to);
        if(!ret) {

            //Timeout happens
            if(!req.empty()) {

                for(const auto& elm: req) {
                    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [ConnectionHandler:%t] %M %N:%l handle: %d request: %s\n"), handle, elm.c_str()));
                }

                auto resp = m_role->rx(req);
                auto ret = tx(resp, handle);
                ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [ConnectionHandler:%t] %M %N:%l handle: %d response: %s\n"), handle, resp.c_str()));
            }

            //going out of while loop.
            break;

        } else if(ret < 0) {
            //error has happened
        } else if(FD_ISSET(handle, &rd_fd)) {

            std::array<std::int8_t, 1024> in;
            in.fill(0);
            auto len = recv(handle, in.data(), in.size(), 0);

            if(len > 0) {
                req.push_back(std::string(in.data(), len));
            } else if(!len) {
                //connection is closed
                ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [ConnectionHandler:%t] %M %N:%l connection close handle: %s\n"), handle));
                return(-1);
            }
        }
    }
}

ACE_INT32 assakeena::ServiceHandler::stop() {

}

ACE_HANDLE assakeena::ServiceHandler::start() {

}

// TCP Client ==========================>>>>>
/**
 * @brief 
 * 
 * @param in 
 * @return auto 
 */
auto assakeena::TcpClient::rx(const std::vector<std::string>& in) {
    std::string rsp;
    if(in.empty()) {
        //empty string
        return(std::string());
    }
    //process the Request
    Http http(in);
    
    return(rsp);
}

auto assakeena::TcpClient::time_out(const auto& in) {

}


//TCP Server ==================>>>>>
int assakeena::TcpServer::svc(void) {

}

int assakeena::TcpServer::open(void *args) {
    ACE_UNUSED_ARG(arg);
    /*! Number of threads are 1, which is 2nd argument. by default  it's 1.*/
    activate();
    return(0);
}

int assakeena::TcpServer::close(u_long flags) {
    ACE_UNUSED_ARG(flag);
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [worker:%t] %M %N:%l Micro service is closing\n")));
    return(0);
}

ACE_INT32 assakeena::TcpServer::handle_timeout(const ACE_Time_Value &tv, const void *act) {

}

ACE_INT32 assakeena::TcpServer::handle_input(ACE_HANDLE handle) {
    ACE_UNUSED_ARG(handle);
    int ret_status = 0;
    ACE_SOCK_Stream peer_stream;
    ACE_INET_Addr peer_addr;

    if(handle == handle()) {
        //new client connection 
        auto ret_status = m_server.accept(peer_stream, &peer_addr);

        if(!ret_status) {
            // New Connection isaccepted 
            TcpClient clnt = std::make_unique<TcpClient>();
            clnt->handle(peer_stream.get_handle());
            auto result = m_connections.insert_or_assign(std::make_pair(peer_stream.get_handle(), std::move(clnt)));

            if(!result->second) {
              //Error
              ACE_ERROR((LM_ERROR, ACE_TEXT("%D [TcpServer:%t] %M %N:%l insert_or_assing to unordered_map is failed\n")));
              close(peer_stream.get_handle());
              // If insertion to STL is failed then clnt shouldn't be moved and it will go out of scope and memory will be freed.
            } else {
                ACE_Reactor::instance()->register_handler(peer_stream.get_handle(), this, 
                                                      ACE_Event_Handler::READ_MASK |
                                                      ACE_Event_Handler::TIMER_MASK | 
                                                      ACE_Event_Handler::SIGNAL_MASK);
            }
        }
    } else {
        //existing client connection 

    }
}

ACE_INT32 assakeena::TcpServer::handle_signal(int signum, siginfo_t *s, ucontext_t *u) {

}

ACE_INT32 assakeena::TcpServer::handle_close (ACE_HANDLE channel, ACE_Reactor_Mask mask) {

}

ACE_HANDLE get_handle() const {

}

ACE_INT32 assakeena::TcpServer::handle_signal(int signum, siginfo_t *s, ucontext_t *u) {

}

auto assakeena::TcpServer::rx(const std::string& in) {

}

auto assakeena::TcpServer::to(auto in) {

}

auto assakeena::TcpServer::tx(std::string out) {

}

auto assakeena::TcpServer::start() {
    /* subscribe for signal */
    ACE_Sig_Set ss;
    ss.empty_set();
    ss.sig_add(SIGINT);
    ss.sig_add(SIGTERM);

    ACE_Reactor::instance()->register_handler(&ss, this); 

    for(auto const& elm: m_services) {
        auto channel = elm->first;
        ACE_Reactor::instance()->register_handler(handle(), this, ACE_Event_Handler::ACCEPT_MASK | 
                                                                  ACE_Event_Handler::TIMER_MASK  |
                                                                  ACE_Event_Handler::SIGNAL_MASK); 
    }
}

auto stop() {

}

// HTTP =====================

/**
 * @brief 
 * 
 * @param param 
 */
void assakeena::Http::format_value(const std::string& param) {
  auto offset = param.find_first_of("=", 0);
  auto name = param.substr(0, offset);
  auto value = param.substr((offset + 1));
  std::stringstream ss(value);
  std::int32_t c;
  value.clear();

  while((c = input.get()) != EOF) {
    switch(c) {
      case '+':
        value.push_back(' ');
      break;

      case '%':
      {
        std::int8_t octalCode[3];
        octalCode[0] = (std::int8_t)input.get();
        octalCode[1] = (std::int8_t)input.get();
        octalCode[2] = 0;
        std::string octStr((const char *)octalCode, 3);
        std::int32_t ch = std::stoi(octStr, nullptr, 16);
        value.push_back(ch);
      }
      break;

      default:
        value.push_back(c);
    }
  }

  if(!value.empty() && !name.empty()) {
    add_element(name, value);
  }
}

/**
 * @brief 
 * 
 * @param in 
 */
void assakeena::Http::parse_uri(const std::string& in)
{
  std::string delim("\r\n");
  size_t offset = in.find_first_of(delim, 0);

  if(std::string::npos != offset) {
    /* Qstring */
    std::string first_line = in.substr(0, offset);
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [worker:%t] %M %N:%l The request string is: %s\n"), first_line.c_str()));

    offset = first_line.find_first_of(" ", 0);
    // HTTP Request line must start with method - GET/POST/PUT/DELETE/OPTIONS
    if(std::string::npos != offset) {

      //e.g. The request string is GET /webui/main.04e34705edfe295e.js HTTP/1.1
      auto req_method = first_line.substr(0, offset);
      method(req_method); //GET/POST/PUT/DELETE/OPTIONS
      offset = first_line.find_first_of("?");

      if(std::string::npos == offset) {

        //'?' is not present in the first_line, which means QS - Query String is not present
        //e.g. The request string is GET /webui/main.04e34705edfe295e.js HTTP/1.1
        offset = first_line.find_first_of(" ", method().length() + 1);

        if(std::string::npos != offset) {
          auto resource_uri = first_line.substr(0, offset);
          uri(resource_uri);
          return;
        }

      } else {

        auto resource_uri = first_line.substr(method().length() + 1, offset - (method().length() - 1));
        uri(resource_uri);
      }
    }

    std::string QS(first_line.substr(offset + 1);
    offset = QS.find_last_of(" ");
    QS = QS.substr(0, offset);

    while(true) {

      offset = QS.find_first_of("&");
      if(std::string::npos == offset) {
        format_value(QS);
        break;
      }
      auto key_value = QS.substr(0, offset);
      format_value(key_value);
      QS = QS.substr(offset+1);

    }
  }
}

/**
 * @brief 
 * 
 * @param in 
 */
void Http::parse_header(const std::string& in)
{
  std::stringstream input(in);
  std::string line_str;
  line_str.clear();

  /* getridof first request line 
   * GET/POST/PUT/DELETE <uri>?uriName[&param=value]* HTTP/1.1\r\n
   */
  std::getline(input, line_str, "\r\n");

  auto offset = input.find_last_of("\r\n\r\n");
  if(std::string::npos != offset) {
    //HTTP Header part
    auto header = input.substr(0, offset);
    std::stringstream ss(header);

    while(!ss.eof()) {

      line_str.clear();
      std::getline(ss, line_str, "\r\n");
      offset = line_str.find_first_of(": ", 0);
      auto key = line_str.substr(0, offset);
      auto value = line_str.substr(offset+2);
      //getting rid oftrailing \r\n
      offset = value.find_first_of("\r\n");
      value = value.substr(0, offset);

      if(!key.empty() && !value.empty()) {
        add_element(key, value);
      }
    }
  }
}

std::string Http::get_header(const std::string& in)
{
  std::string header("");
  auto offset = in.find_last_of("\r\n\r\n");
  if(std::string::npos != offset) {
    header = in.substr(0, offset);
  }
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [http:%t] %M %N:%l HTTP Header: %s\n", header.c_str())));
  return(header);

}

std::string Http::get_body(const std::string& in)
{
  auto header = get_header(in);
  auto bdy = in.substr(header.length(), in.length() - header.length());
  return(bdy);
}

// MAIN =============

int main(std::int32_t argc, char *argv[]) {

    ACE_LOG_MSG->open(argv[0], ACE_LOG_MSG->STDERR|ACE_LOG_MSG->SYSLOG);
    assakeena::UnicloudMgr unicloud;

    int ret = pipe(shell.fds().at(assakeena::FDs::READ).data());

    if(ret < 0) {
        std::cout<< "pipe for assakeena::FDs::READ Failed " << std::endl;
        return(ret);
    }
    
    ret = pipe(shell.fds().at(assakeena::FDs::WRITE).data());
    if(ret < 0) {
        std::cout<< "pipe for assakeena::FDs::WRITE Failed " << std::endl;
        return(ret);
    }

    auto pid = fork();

    if(pid > 0) {
        //Parent Process
        close(shell.fds(assakeena::FDs::READ, assakeena::FDs::READ));
        close(shell.fds(assakeena::FDs::WRITE, assakeena::FDs::WRITE));
        shell.fds(assakeena::FDs::READ, assakeena::FDs::READ, -1);
        shell.fds(assakeena::FDs::WRITE, assakeena::FDs::WRITE, -1);
        
        //rdFd[1] of child process (writes) ---> to rdFd[0] of parent process (reads)
        //wrFd[0] of child process (reads) ---> to wrFd[1] of parent process (writes)
        
        //This will maps rdFd[1] to stdout and closes stdout fd.
        dup2(shell.fds(assakeena::FDs::READ, assakeena::FDs::WRITE), 1);
        //This will maps wrFd[0] to stdin and closes stdin fd.
        dup2(shell.fds(assakeena::FDs::WRITE, assakeena::FDs::READ), 0);

        const char *args[] = {"/usr/bin/sh", NULL};
        if(execlp(args[0], args[0], args[1]) < 0) {
            std::cout << "spawning of/usr/bin/sh processis failed" << std::endl;
            exit(0);
        }
    } else if(!pid) {

        //Child Process
        close(shell.fds(assakeena::FDs::READ, assakeena::FDs::WRITE));
        close(shell.fds(assakeena::FDs::WRITE, assakeena::FDs::READ));

        fds(assakeena::FDs::READ, assakeena::FDs::WRITE, -1);
        fds(assakeena::FDs::WRITE, assakeena::FDs::READ, -1);

        //rdFd[1] of child process (writes) ---> to rdFd[0] of parent process (reads)
        //wrFd[0] of child process (reads) ---> to wrFd[1] of parent process (writes)

        if(argc > 0) {

        }
    }   
}












#endif /* __unicloud_cc__*/
