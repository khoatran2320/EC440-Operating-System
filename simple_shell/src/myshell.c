#include "myshell_parser.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "sys/wait.h"
#include "unistd.h"
#include "errno.h"
#include "signal.h"
#include "fcntl.h"
#include "sys/types.h"
#include "sys/stat.h"

void handle_child(int signo){
    int status;
    pid_t pid;
    while((pid = waitpid(0, &status, WNOHANG))>0){
    }
    return;
}
void redirection(const char *rdr_in, const char * rdr_out, const int num_commands, const int curr_cmd_num){
    if(rdr_in && curr_cmd_num == 0){
        int f = open(rdr_in, O_RDONLY);
        if(f > 0){
            if(dup2(f, STDIN_FILENO)<0){
                perror("ERROR");
            }
            close(f);
        }else{
            perror("ERROR");
        }
    }
    if(rdr_out && (curr_cmd_num == num_commands - 1)){
        int f = open(rdr_out, O_WRONLY | O_CREAT, 0777);
        if(f > 0){
            if(dup2(f, STDOUT_FILENO)<0){
                perror("ERROR");
            }
            close(f);
        }
        else{
            perror("ERROR");
        }
    }
    return;
}

void multi_commands(int pipefd[], const int num_commands, const int curr_cmd_num){
    if(curr_cmd_num != 0){
        //set the input to be the output of the last command if not first command
        if(dup2(pipefd[(curr_cmd_num - 1)*2], STDIN_FILENO) < 0){
            perror("ERROR");
            return;
        }
    }
    if (curr_cmd_num < num_commands - 1){
        //set output to next command if not last command
        if(dup2(pipefd[curr_cmd_num*2+1], STDOUT_FILENO) < 0){
            perror("ERROR");
            return;
        }
    }
    //close all ports
    for(int j = 0; j < 2*(num_commands - 1); j++){
        if(close(pipefd[j]) < 0){
            perror("ERROR");
        }
    }
    return;
}
int main(int argc, char **argv){
    //check for -n
    int quiet = 0;
    if(argc == 2 && strcmp(argv[1], "-n") == 0){
        quiet = 1;
    }

    //init data structures
    int status = 0;
    char command_line[MAX_LINE_LENGTH];
    int num_commands = 0;
    struct pipeline_command *curr_cmd;
    struct pipeline* pipeline;

    //start loop
    while(1){

        //prompt
        if(!quiet){
            printf("my_shell$");
        }

        //get command line
        char *r = fgets(command_line, MAX_LINE_LENGTH, stdin);

        //build the pipeline
        pipeline = pipeline_build(command_line, &num_commands);
        //free pipeline on error before exit
        if(!r){
            if(pipeline){
                pipeline_free(pipeline);
            }
            return 0;
        }


        //set up
        int pipefd[2*(num_commands - 1)];
        int curr_cmd_num = 0;
        curr_cmd = pipeline->commands;

        // establish  pipe for all commands
        for(int i = 0; i < 2*(num_commands - 1); i++){
            if(pipe(pipefd + i*2) < 0) {
                perror("ERROR");
                return 0;
            }
        }
        //go through all commands
        while(curr_cmd){
            pid_t pid = fork();
            if(pid == 0){
                //child process
                if(num_commands > 1){
                    multi_commands(pipefd, num_commands, curr_cmd_num);
                }
                //check for redirection
                redirection(curr_cmd->redirect_in_path, curr_cmd->redirect_out_path, num_commands, curr_cmd_num);
                //execute command
                if(execvp(curr_cmd->command_args[0], curr_cmd->command_args) < 0){
                    perror("ERROR");
                    return 1;
                }
            }else if(pid < 0){
                perror("ERROR");
                return 1;
            }
            //move to next command
            curr_cmd = curr_cmd->next;
            curr_cmd_num++;
        }

        //close all pipes in parent process
        for(int j = 0; j < 2*(num_commands - 1); j++){
            if(close(pipefd[j]) < 0){
                perror("ERROR");
            }
        }
        //wait 
        if(pipeline->is_background){
            struct sigaction act;
            sigemptyset(&act.sa_mask);
            act.sa_flags = 0;
            act.sa_handler = handle_child;
            sigaction(SIGCHLD, &act, NULL);
        }
        else{
            for(int j = 0; j < num_commands; j++){
            wait(&status);
        }
        }
    }
    if(pipeline){
        pipeline_free(pipeline);
    }
    return 0;
}