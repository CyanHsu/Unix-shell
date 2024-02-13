//
//  main.cpp
//  Shell
//
//  Created by Chunhao on 2023/2/9.
//

#include <iostream>
#include <sstream>
#include <map>
#include <readline/readline.h>
#include <csignal>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>

#include "shelpers.hpp"

std::map<int, std::string> backgroundJobs;
std::map<int, int> jobs;
int jobCount = 0;

void proc_exit(int sigchild);

int main(int argc, const char * argv[]) {
    
    // handle the signal sent from child when when child ends
    signal(SIGCHLD, proc_exit);
    
    while(true){
        
        // create my shell's display string
        std::string path = currentPath();
        std::string shellString;
        shellString.append("@MyUnixShell  ");
        shellString.append(path);
        shellString.append("> ");
        
        // use readline() for tab completion
        std::string input = readline(shellString.c_str());
        
        // if user type exit, then end the shell program
        if(input == "exit"){
            exit(0);
        }
        
        // parse input commands
        std::vector<std::string> tokens = tokenize(input);
        std::vector<Command> cmds = getCommands(tokens);
        
        // if the command is cd, handle it in the parent process, don't create child
        if(cmds[0].exec == "cd"){
            if(cmds[0].argv.size() == 2 || strcmp(cmds[0].argv[1], "~") == 0){
                if(chdir(getenv("HOME")) == -1){
                    perror("no such directory\n");
                    continue;
                }
            }
            else{
                if(chdir(cmds[0].argv[1]) == -1){
                    perror("no such directory\n");
                    continue;
                }
            }
        }
        else if(cmds[0].setEV){
            //do nothing when setting environment variable
        }
        else{
            for(int i = 0; i < cmds.size(); i ++){
                
                Command cmd = cmds[i];
                int rc = fork();
                
                if (rc < 0) {
                    // fork failed; exit
                    perror("fork failed\n");
                    exit(1);
                }// cat main.cpp | head | tail -n 5   6-10 line of main.cpp
                else if (rc == 0) {
                        // redirect the stdout to file descriptor
                        if(cmd.fdStdout != 1){
                            if(dup2(cmd.fdStdout, 1) == -1){
                                perror("dup failed\n");
                                exit(1);
                            }
                        }
                        // redirect the stdin to file descriptor
                        if(cmd.fdStdin != 0){
                            if(dup2(cmd.fdStdin, 0) == -1){
                                perror("dup failed\n");
                                exit(1);
                            }
                        }
                        execvp(cmd.exec.c_str(), const_cast<char* const*>(cmd.argv.data()));
                        perror("execvp fail");
                        exit(1);
                }
                else { // parent
                    
                    int status;
                    // close the fd
                    if(cmd.fdStdin!=0)
                        if(close(cmd.fdStdin) == -1){
                            perror("fail to close fd");
                            exit(1);
                        }
                    if(cmd.fdStdout!=1)
                        if(close(cmd.fdStdout) == -1){
                            perror("fail to close fd");
                            exit(1);
                        }
                    
                    // wait child if it's not a background
                    if(!cmd.background ){
                        waitpid(rc, &status, 0);
                    }
                    else{
                        jobCount += 1; //job number
                        jobs.insert(std::pair<int, int>(rc , jobCount)); // assign job number -> pid : job number
                        backgroundJobs.insert(std::pair<int, std::string>(rc, cmd.exec.c_str()));// pid : command
                        printf("[%d]  %d\n", jobs[rc], rc );
                    }
                }
            }
        }
    }
    return 0;
}



void proc_exit(int sigchild){
    
    int wstat;
    pid_t  pid;
    // if there is background process, check the status
    while (backgroundJobs.size() > 0) {
        pid = wait3 (&wstat, WNOHANG, (struct rusage *)NULL );

        if (pid == 0)
            return;
        else if (pid == -1)
            return;
        else{
            for(std::map<int, std::string>::iterator it = backgroundJobs.begin(); it != backgroundJobs.end(); it++){
                if((*it).first == pid){
                    printf("\n[%d] + done     %s\n", jobs[pid], backgroundJobs[pid].c_str());
                    backgroundJobs.erase(pid);
                    jobs.erase(pid);
                    jobCount -= 1;
                    return;
                }
            }
        }
    }
}
