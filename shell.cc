#ifndef __SHELL_CC__
#define __SHELL_CC__


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

ACE_INT32 assakeena::ConnectionHandler::handle_timeout(const ACE_Time_Value &tv, const void *act) {

}
ACE_INT32 assakeena::ConnectionHandler::tx(const std::string &rsp, ACE_HANDLE handle) {
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

ACE_INT32 assakeena::ConnectionHandler::handle_input(ACE_HANDLE handle) {
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

ACE_INT32 assakeena::ConnectionHandler::handle_signal(int signum, siginfo_t *s, ucontext_t *u) {

}

ACE_INT32 assakeena::ConnectionHandler::handle_close (ACE_HANDLE handle, ACE_Reactor_Mask) {

}

ACE_HANDLE assakeena::ConnectionHandler::get_handle() const {

}

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

    return(rsp);
}

auto assakeena::TcpClient::time_out(const auto& in) {

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

        auto resource_uri = first_line.substr(0, offset);
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

void Http::parse_mime_header(const std::string& in)
{
  std::stringstream input(in);
  std::string param;
  std::string value;
  std::string parsed_string;
  std::string line_str;
  line_str.clear();

  /* getridof first request line 
   * GET/POST/PUT/DELETE <uri>?uriName[&param=value]* HTTP/1.1\r\n
   */
  std::getline(input, line_str);

  param.clear();
  value.clear();
  parsed_string.clear();

  /* iterating through the MIME Header of the form
   * Param: Value\r\n
   */
  while(!input.eof()) {
    line_str.clear();
    std::getline(input, line_str);
    std::stringstream _line(line_str);

    std::int32_t c;
    while((c = _line.get()) != EOF ) {
      switch(c) {
        case ':':
        {
          param = parsed_string;
          parsed_string.clear();
          /* getridof of first white space */
          c = _line.get();
          while((c = _line.get()) != EOF) {
            switch(c) {
              case '\r':
              case ' ':
                /* get rid of \r character */
                break;

              default:
                parsed_string.push_back(c);
                break;
            }
          }
          /* we hit the end of line */
          value = parsed_string;
          add_element(param, value);
          parsed_string.clear();
          param.clear();
          value.clear();
        }
          break;

        default:
          parsed_string.push_back(c);
          break;
      }
    }
  }
}

void Http::dump(void) const 
{
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [worker:%t] %M %N:%l The uriName is %s\n"), m_uriName.c_str()));
    for(auto& in: m_tokenMap) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [worker:%t] %M %N:%l param %s value %s\n"), in.first.c_str(), in.second.c_str()));
    }

}

std::string Http::get_header(const std::string& in)
{

  if(std::string::npos != in.find("Content-Type: application/json")) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [worker:%t] %M %N:%l The content Type is application/json\n")));
    std::string body_delimeter("\r\n\r\n");
    size_t body_offset = in.find(body_delimeter.c_str(), 0, body_delimeter.length());
    if(std::string::npos != body_offset) {
      body_offset += body_delimeter.length();
      std::string document = in.substr(0, body_offset);

      ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [worker:%t] %M %N:%l The header is %s\n"), document.c_str()));
      return(document);
    }
  }
  return(std::string(in));
}

std::string Http::get_body(const std::string& in)
{
  std::string ct = get_element("Content-Type");
  std::string contentLen = get_element("Content-Length");
  std::string body_delimeter("\r\n\r\n");
  std::string ty("application/json");

  if(ct.length() && !ct.compare("application/json") && contentLen.length()) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [worker:%t] %M %N:%l The content Type is application/json CL %d hdrlen %d\n"), std::stoi(contentLen), header().length()));

    size_t body_offset = in.find(body_delimeter.c_str(), 0, body_delimeter.length());

    if(std::string::npos != body_offset) {
      std::string bdy(in.substr((body_delimeter.length() + body_offset), std::stoi(contentLen)));
      //ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [worker:%t] %M %N:%l Bodylen is %d The BODY is \n%s\n"), bdy.length(), bdy.c_str()));

      if(contentLen.length() && (in.length() == header().length() + std::stoi(contentLen))) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [worker:%t] %M %N:%l Bodylen is %d The BODY is \n%s\n"), bdy.length(), bdy.c_str()));
        return(bdy);
      }
    }

    return(std::string());

    #if 0
    std::string body_delimeter("\r\n\r\n");
    size_t body_offset = in.find(body_delimeter, 0);
    if(std::string::npos != body_offset) {
      body_offset += body_delimeter.length();
      std::string document = in.substr(body_offset);

      ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [worker:%t] %M %N:%l The body is %s\n"), document.c_str()));
      return(document);
    }
    #endif
  }
  return(std::string());
}

// MAIN =============

int main(std::int32_t argc, char *argv[]) {

    assakeena::Shell shell;
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












#endif /* __SHELL_CC__*/
