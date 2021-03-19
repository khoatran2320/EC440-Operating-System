TO RUN:
    run 'make' then './myshell' in terminal

HOW IT WORKS:
    The program is a REPL program that simulates the shell. It supports basic shell commands, piping commands, redirection, and background processing. The program first prompts the user for a command. This command is restricted to one line of a max size of 512 characters at a time. The program will get that command line and build a pipeline of commands from it. In particular, it will lex the command and parse it. 
    
    The lexing part works by first tokenizing the string with a space delimiter. The result is an array of strings without any space. Then, the lexer will loop through every string and again tokenize each string by a whitespace as a delimiter. Now, the tokens are all rid of whitespaces. The tokens are then passed to the parser where it will build a pipeline. 

    The parser loops through all the tokens and builds a pipeline necessary for processing in the main program. 

    Once the pipeline has been built, the shell will make all the pipes required for the commands. Then, it will loop through each command and fork a child to execute the command. If there are multiple commands, then the program will re-route the inputs and outputs so that the output of the prior commmand is fed into the input of the current command. This is done by altering the STDIN and STDOUT file desciptors in the pipe and current file descriptions. The program handles redirection in the same manner. For the background feature, the program will not wait for the child processes, but rather set up a handler so that the parent gets notified when the child dies and clean up the child there. 

EXTERNAL RESOURCES:
    multiple pipes: 
        explains how multiple pipes work with demo: https://www.youtube.com/watch?v=NkfIUo_Qq4c&t=700s&ab_channel=CodeVault 
        example of multiple pipes: http://www.cs.loyola.edu/~jglenn/702/S2005/Examples/dup2.html?fbclid=IwAR3iEkZ3VwiORE-S0Q1X6yn60JfVbUHk6XdoFj1l-I8LI9wZqO-3qz7XIno
        example of multiple pipes: https://stackoverflow.com/questions/8389033/implementation-of-multiple-pipes-in-c?fbclid=IwAR1AJwCI1zLmD6uAAk0Lj5RzRhJPp48wLIYk8ZzEF3XgjeYUyFPL2ktG7ag
        example of pipes with redirection: https://stackoverflow.com/questions/916900/having-trouble-with-fork-pipe-dup2-and-exec-in-c/

    sigaction:
        explaines how sigaction works with demo: https://www.youtube.com/watch?v=jF-1eFhyz1U&t=19s&ab_channel=CodeVault
        example of SIGNCHLD sigaction: (slide from lecture)

    Sources for general consultation of concepts (including but not limited to wait, fork, dup, dup2 system calls):
        explanation of conceptions with examples: https://www.geeksforgeeks.org/
        manual describing behavior for various functions: https://man7.org/linux/man-pages/
