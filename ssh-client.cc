#ifndef __SSH_CLIENT_CC__
#define __SSH_CLIENT_CC__


#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <array>
#include <iostream>
#include <cstdio>

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
        std::vector<std::array<std::int32_t, 2>> fds;
        fds.push_back(reinterpret_cast<std::array<std::int32_t, 2>&>(rdFd));
        fds.push_back(reinterpret_cast<std::array<std::int32_t, 2>&>(wrFd));
        //rdFd[1] of child process (writes) ---> to rdFd[0] of parent process (reads)
        //wrFd[0] of child process (reads) ---> to wrFd[1] of parent process (writes)
        std::string sh("/usr/bin/sh");
        dup2(rdFd[1], 1);
        dup2(wrFd[0], 0);

        //char *const args[] = {"/usr/bin/sh",NULL};
        char *args[] = {"sh", NULL};
        if(execlp(sh.c_str(), "/usr/bin/sh","-svn", NULL) < 0) {
            std::cout << "spawning of/usr/bin/sh processis failed" << std::endl;
            exit(0);
        }

        #if 0
        while(true) {
            char data[1024];
            memset(data, 0, 1024);
            std::int32_t len = read(wrFd[0], data, 1024);
            std::cout << "Received " << data << std::endl;
        }
        #endif
        std::cout << "Parent Process Exit" << std::endl;

    } else if(!pid) {

        //Child Process
        close(rdFd[1]);
        close(wrFd[0]);
        rdFd[1] = -1;
        wrFd[0] = -1;
        //rdFd[1] of child process (writes) ---> to rdFd[0] of parent process (reads)
        //wrFd[0] of child process (reads) ---> to wrFd[1] of parent process (writes)

        std::vector<std::array<std::int32_t, 2>> fds;
        fds.push_back(reinterpret_cast<std::array<std::int32_t, 2>&>(rdFd));
        fds.push_back(reinterpret_cast<std::array<std::int32_t, 2>&>(wrFd));

        while(true) {
            //std::string line("");
            //std::iostream line;
            std::cout <<std::endl;
            std::cout << "Enter Command now " << std::endl;
            std::cout << "I am inside Child Process" << std::endl;

            //std::cin >> line;
            //char data[1024];
            //std::cin.get(line.data(), 1024);
            std::array<char, 2048> cmd;
            cmd.fill(0);
            std::cin.getline(cmd.data(), 2048);
            //std::cout << "cmd.data() " << cmd.data() << std::endl;
            std::int32_t len = write(wrFd[1], reinterpret_cast<const char *>(cmd.data()), std::cin.gcount());
            if(len <= 0) {
                std::cout << "Failed to send Command to executable " << std::endl;
            } else {
                std::cout << "Command sent to Executable successfully command: \'" << cmd.data() << "\' length: " << strlen(cmd.data()) << std::endl;
            }

            std::array<char, 1024> arr;
            arr.fill(0);
            len = read(rdFd[0], arr.data(), arr.size());
            if(len > 0) {
                std::string data(arr.data(), len);
                std::cout << "The Command output is " << data.c_str() <<std::endl;
            } else {
                //std::cout << "read is failed " << std::endl;
            }
        }
    } else {
        //error 
        std::cout << " fork failed " << std::endl;
    }
}












#endif
