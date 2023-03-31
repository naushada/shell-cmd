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

    int rdFd[2];
    int wrFd[2];
    //std::vector<std::array<std::int32_t, 2>> fds;
    

    int ret = pipe(/*shell.fds().at(assakeena::FDs::READ).data()*/rdFd);
    if(ret < 0) {
        std::cout<< "pipe for assakeena::FDs::READ Failed " << std::endl;
        return(ret);
    }
    
    ret = pipe(/*shell.fds().at(assakeena::FDs::WRITE).data()*/wrFd);
    if(ret < 0) {
        std::cout<< "pipe for assakeena::FDs::WRITE Failed " << std::endl;
        return(ret);
    }

    auto pid = fork();
    if(pid > 0) {
        //Parent Process
        assakeena::Shell shell;
        std::array<std::int32_t, 2> fdRead{rdFd[0], rdFd[1]};
        std::array<std::int32_t, 2> fdWrite{wrFd[0], wrFd[1]};
        //fd.at(assakeena::FDs::READ) = rdFd[0];
        //fd.at(assakeena::FDs::READ) = rdFd[1];
        shell.fds(fdRead);
        shell.fds(fdWrite);

        //close(rdFd[0]);
        close(shell.fds(assakeena::FDs::READ, assakeena::FDs::READ));
        //close(wrFd[1]);
        close(shell.fds(assakeena::FDs::WRITE, assakeena::FDs::WRITE));
        //rdFd[0] = -1;
        shell.fds(assakeena::FDs::READ, assakeena::FDs::READ, -1);
        //wrFd[1] = -1;
        shell.fds(assakeena::FDs::WRITE, assakeena::FDs::WRITE, -1);
        
        //rdFd[1] of child process (writes) ---> to rdFd[0] of parent process (reads)
        //wrFd[0] of child process (reads) ---> to wrFd[1] of parent process (writes)
        
        //dup2(rdFd[1], 1); //This will maps rdFd[1] to stdout and closes stdout fd.
        dup2(shell.fds(assakeena::FDs::READ, assakeena::FDs::WRITE), 1);
        //dup2(wrFd[0], 0); //This will maps wrFd[0] to stdin and closes stdin fd.
        dup2(shell.fds(assakeena::FDs::WRITE, assakeena::FDs::READ), 0);

        //const char *args[] = {"/usr/bin/sh", NULL};
        std::array<std::string, 2> args = {"/usr/bin/sh", NULL};
        if(execlp(std::get<0>(args).c_str(), std::get<0>(args).c_str(), std::get<1>(args)) < 0) {
            std::cout << "spawning of/usr/bin/sh processis failed" << std::endl;
            exit(0);
        }
    } else if(!pid) {

        //Child Process
        assakeena::Shell shell;
        std::array<std::int32_t, 2> fdRead{rdFd[0], rdFd[1]};
        std::array<std::int32_t, 2> fdWrite{wrFd[0], wrFd[1]};
        //fd.at(assakeena::FDs::READ) = rdFd[0];
        //fd.at(assakeena::FDs::READ) = rdFd[1];
        shell.fds(fdRead);
        shell.fds(fdWrite);
        
        //close(rdFd[1]);
        close(shell.fds(assakeena::FDs::READ, assakeena::FDs::WRITE));
        //close(wrFd[0]);
        close(shell.fds(assakeena::FDs::WRITE, assakeena::FDs::READ));
        //rdFd[1] = -1;
        shell.fds(assakeena::FDs::READ, assakeena::FDs::WRITE, -1);
        //wrFd[0] = -1;
        shell.fds(assakeena::FDs::WRITE, assakeena::FDs::READ, -1);
        //rdFd[1] of child process (writes) ---> to rdFd[0] of parent process (reads)
        //wrFd[0] of child process (reads) ---> to wrFd[1] of parent process (writes)

        auto receptionFn = [](auto channel) {
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
            std::int32_t len = write(/*wrFd[1]*/shell.fds(assakeena::FDs::WRITE, assakeena::FDs::WRITE), reinterpret_cast<const char *>(ss.str().c_str()), ss.str().length());
            if(len <= 0) {
                std::cout << "Failed to send Command to executable " << std::endl;
                exit(0);
            } else {
                std::cout << "Command sent to Executable successfully command: "<< std::endl << ss.str().c_str() << " length: " << ss.str().length() << std::endl;
                receptionFn(shell.fds(assakeena::FDs::READ, assakeena::FDs::READ));
            }
        }

    } else {
        //error 
        std::cout << " fork failed " << std::endl;
    }
}












#endif /* __SHELL_CC__*/
