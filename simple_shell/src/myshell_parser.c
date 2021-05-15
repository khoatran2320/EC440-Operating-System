#include "myshell_parser.h"
#include "stddef.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"


int isWhiteSpace(const char c){
	return ((c == '\n') || (c  == '\t') || c  == ' ');
}
int isSpecialToken(const char c){
	return ((c == '|') || (c == '<') ||  (c == '>') || (c == '&'));
}

//DESC:		separates all words and tokens
//INPUTS:	command_line 	= given command line string
//			tokens_size		= number of tokens returned
//OUTPUTS:	char **			= array of tokens
char **find_tokens(const char *command_line, int *tokens_size){

	//find length of command line
	unsigned long cl_length = strlen(command_line);
	char cl_cpy[cl_length];
	char **tokens = malloc(MAX_ARGV_LENGTH * MAX_LINE_LENGTH * sizeof(char)); 
	int i, j;
	char token[cl_length];
	memset(&token[0], 0, cl_length);
	j = 0;
	//make a copy to work with
	strcpy(cl_cpy, command_line);
	for(i = 0; i < cl_length;i++){
		char curr = cl_cpy[i];
		int tk_len = strlen(token);
		if(isWhiteSpace(curr)){
			if(tk_len > 0){
				char *temp = (char *)malloc(tk_len *sizeof(char));
				memcpy(temp, &token[0], tk_len);
				tokens[j++] = temp;
				memset(&token[0], 0, cl_length);
			}
		} else if (isSpecialToken(curr)){
			if(tk_len > 0){
				char *temp = (char *)malloc(tk_len *sizeof(char));
				memcpy(temp, &token[0], tk_len);
				tokens[j++] = temp;
				memset(&token[0], 0, cl_length);

				char *temp1 = (char *)malloc(sizeof(char));
				memcpy(temp1, &curr, 1);
				tokens[j++] = temp1;
			}	
			else{
				char *temp1 = (char *)malloc(sizeof(char));
				memcpy(temp1, &curr, 1);
				tokens[j++] = temp1;
			}
		}else{
			token[tk_len] = curr; 
		}
	}
	
	*tokens_size = j;
	return tokens;
}

int find_num_cmds(char **tokens, int num_tokens){
	int commands = 1;
	for(int i = 0; i < num_tokens; i++){
		if(strcmp(tokens[i],"|") == 0){
			commands++;
		}
	}
	return commands;
}
struct pipeline *pipeline_build(const char *command_line, int * num_commands)
{
	int num_tokens = 0;
	char **tokens = find_tokens(command_line, &num_tokens);
	*num_commands = find_num_cmds(tokens, num_tokens);

	struct  pipeline_command *pipelines = (struct pipeline_command *)malloc(*num_commands * sizeof(struct pipeline_command));

	int j = 0, i = 0, k = 0;
	int curr_cmd_done;
	int catch = 0;
	for(i = 0;i < *num_commands; i++){
		curr_cmd_done = 0;
		struct pipeline_command pl;
		pl.redirect_in_path = NULL;
		pl.redirect_out_path = NULL;
		while(!curr_cmd_done && j < num_tokens)
		{
			if((strcmp(tokens[j], ">") == 0) && ((j + 1) < num_tokens))
			{
				pl.redirect_out_path = tokens[j+1];
				j++;
				if(j > num_tokens - 2){
					catch = 1;
				}
			}
			else if((strcmp(tokens[j], "<") == 0) && ((j + 1) < num_tokens))
			{
				pl.redirect_in_path = tokens[j+1];
				j++;
				if(j > num_tokens - 2){
					catch = 1;
				}
			}
			else if((strcmp(tokens[j], "&") != 0) && (strcmp(tokens[j], "|") != 0))
			{
				pl.command_args[k++] = tokens[j];
			}
			if(strcmp(tokens[j], "|") == 0 || j > num_tokens - 2 || catch){
				catch = 0;
				pl.command_args[k++] = NULL;
				pl.next = (i < (*num_commands - 1)) ? &pipelines[i + 1] : NULL;
				pipelines[i] = pl;
				k = 0;
				curr_cmd_done = 1;
			}
			j++;
		}
	}
	struct pipeline *final_pipe = (struct pipeline *)malloc(sizeof(struct pipeline));
	*final_pipe = (struct  pipeline){.commands = &pipelines[0], .is_background = (strcmp(tokens[num_tokens - 1], "&") == 0) ? true : false};
	free(tokens);
	return final_pipe;
}

void pipeline_free(struct pipeline *pipeline)
{
	if(pipeline){
		struct pipeline_command *curr_pipe = pipeline->commands;
		while (curr_pipe)
		{
			int i = 0;
			while(curr_pipe->command_args[i]){
				free(curr_pipe->command_args[i++]);
			}
			if(curr_pipe->redirect_in_path){
				free(curr_pipe->redirect_in_path);
			}
			if(curr_pipe->redirect_out_path){
				free(curr_pipe->redirect_out_path);
			}
			curr_pipe = curr_pipe->next;
		}
		free(pipeline->commands);
		free(pipeline);
	}
	return;
}
