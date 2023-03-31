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

        shell.fds(assakeena::FDs::READ, assakeena::FDs::WRITE, -1);
        shell.fds(assakeena::FDs::WRITE, assakeena::FDs::READ, -1);

        //rdFd[1] of child process (writes) ---> to rdFd[0] of parent process (reads)
        //wrFd[0] of child process (reads) ---> to wrFd[1] of parent process (writes)

        auto receptionFn = [](auto channel) -> auto {
            std::array<char, 1024> arr;
            std::vector<std::array<char, 1024>> response;
            ssize_t len = -1;
            fd_set fdset;

            std::cout << "Reception thread " << std::endl;
            response.clear();
            FD_ZERO(&fdset);

            do {
                arr.fill(0);
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
            } while(true);

            if(response.size()) {
                for(auto const &elm: response) {
                    std::string data(reinterpret_cast<const char *>(elm.data()), elm.size());
                    std::cout << "The Command output is ====>>>>> "<< std::endl << data.c_str() <<std::endl;
                }
            } else {
                //std::cout << "read is failed " << std::endl;
            }
            return(response);
        };
        
        while(true) {
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
            std::int32_t len = write(shell.fds(assakeena::FDs::WRITE, assakeena::FDs::WRITE), reinterpret_cast<const char *>(ss.str().c_str()), ss.str().length());
            if(len <= 0) {
                std::cout << "Failed to send Command to executable " << std::endl;
                exit(0);
            } else {
                std::cout << "command: " << std::endl << ss.str().c_str() << "sent to Executable length: " << ss.str().length() << std::endl;
                auto result = receptionFn(shell.fds(assakeena::FDs::READ, assakeena::FDs::READ));
            }
        }

    } else {
        //error 
        std::cout << " fork failed " << std::endl;
        exit(0);
    }
}












#endif /* __SHELL_CC__*/
