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

        fds(assakeena::FDs::READ, assakeena::FDs::WRITE, -1);
        fds(assakeena::FDs::WRITE, assakeena::FDs::READ, -1);

        //rdFd[1] of child process (writes) ---> to rdFd[0] of parent process (reads)
        //wrFd[0] of child process (reads) ---> to wrFd[1] of parent process (writes)

        if(argc > 0) {

        }
    }   
}












#endif /* __SHELL_CC__*/
