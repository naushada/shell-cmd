#ifndef __SSH_CLIENT_CC__
#define __SSH_CLIENT_CC__


#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <array>
#include <iostream>
#include <cstdio>
#include <sstream>
#include <thread>

int main(std::int32_t argc, char *argv[]) {

    int rdFd[2];
    int wrFd[2];
    std::vector<std::array<std::int32_t, 2>> fds;

    int ret = pipe(rdFd);
    if(ret < 0) {
        std::cout<< "pipe for rdFd Failed " << std::endl;
        return(ret);
    }

    ret = pipe(wrFd);
    if(ret < 0) {
        std::cout<< "pipe for rdFd Failed " << std::endl;
        return(ret);
    }

    auto pid = fork();
    if(pid > 0) {

        //Parent Process
        close(rdFd[0]);
        close(wrFd[1]);
        rdFd[0] = -1;
        wrFd[1] = -1;
        
        //rdFd[1] of child process (writes) ---> to rdFd[0] of parent process (reads)
        //wrFd[0] of child process (reads) ---> to wrFd[1] of parent process (writes)
        
        dup2(rdFd[1], 1); //This will maps rdFd[1] to stdout and closes stdout fd.
        dup2(wrFd[0], 0); //This will maps wrFd[0] to stdin and closes stdin fd.

        const char *args[] = {"/usr/bin/sh", NULL};
        if(execlp(args[0], args[0], NULL) < 0) {
            std::cout << "spawning of/usr/bin/sh processis failed" << std::endl;
            exit(0);
        }
    } else if(!pid) {

        //Child Process
        close(rdFd[1]);
        close(wrFd[0]);
        rdFd[1] = -1;
        wrFd[0] = -1;
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
                    std::cout << "The Command output is ====>>>>> " << data.c_str() <<std::endl;
                }
            } else {
                //std::cout << "read is failed " << std::endl;
            }
        };
        //std::thread reception(entryFn, rdFd[0]);

        while(true) {
            std::cout <<std::endl;
            std::cout << "Enter Command now " << std::endl;
            std::cout << "I am inside Child Process" << std::endl;

            std::string cmd;
            std::getline(std::cin, cmd);
            std::stringstream ss;

            ss << cmd.data() << "\n";
            std::int32_t len = write(wrFd[1], reinterpret_cast<const char *>(ss.str().c_str()), ss.str().length());
            if(len <= 0) {
                std::cout << "Failed to send Command to executable " << std::endl;
                exit(0);
            } else {
                std::cout << "Command sent to Executable successfully command: " << ss.str().c_str() << " length: " << ss.str().length() << std::endl;
                receptionFn(rdFd[0]);
            }
        }

    } else {
        //error 
        std::cout << " fork failed " << std::endl;
    }
}












#endif
