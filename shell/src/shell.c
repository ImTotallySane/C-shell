#include "prompt.h"
#include "token.h"
#include "parse.h"
#include "hop.h"
#include "reveal.h"
#include "exec.h"
#include "log.h"
#include "redir.h"
#include "pipe.h"
#include "seq.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<pwd.h>
#include<stdbool.h>

int main()
    {
        getcwd(homedir, 1000);
        char prev_dir[1000];
        char temp[1000];
        prev_dir[0]='\0';
        bool hop=false;
        // FILE* ptr=fopen(".history", "r");
        // if(ptr==NULL)
        //     {
        //         perror("Error opening .history file");
        //     }
        // char** commands=(char**)malloc(sizeof(char*)*15);
        // for(int i=0;i<15;i++)
        //     {
        //         commands[i]=(char*)malloc(sizeof(char)*1000);
        //     }
        // int command_count=(load(ptr, commands))%15;
        while(1)
            {
                printprompt();
                char user_input[1000];
                fgets(user_input, sizeof(user_input), stdin);
                int count=0;
                token* list=tokenize(user_input, &count);
                if(list==NULL)
                    {
                        printf("Error in memory allocation");
                        continue;
                    }
                if(count<=1)
                    {
                        free_tokens(list, count);
                        continue;
                    }
                if(!parse(list, count))
                    {
                        continue;
                    }
                token** tk=split_tokens_by_semicolon(list, count);
                int token_index=0;

                // int semicolon_count=0;
                // for(int i=0;i<count-1;i++)
                //     {
                //         if(list[i].type=semicolon)
                //             {
                //                 semicolon_count++;
                //             }
                //     }
                while(tk[token_index]!=NULL)
                    {
                        bool has_pipe = false;
                                for (int i = 0; i < find_token_count(tk[token_index]) - 1; i++) {
                                if (tk[token_index][i].type == token_pipe) {
                                    has_pipe = true;
                                    break;
                                }
                        Command cmd = parse_tokens_for_execution(tk[token_index], find_token_count(tk[token_index]));
                        getcwd(temp, 1000);
                        if(has_pipe)
                                    {
                                        Pipeline pipeline = parse_tokens_for_pipeline(tk[token_index], find_token_count(tk[token_index]));
                                        execute_pipeline(pipeline, homedir, prev_dir, hop);
                                        continue;
                                    }
                        else if(cmd.input_file!=NULL || cmd.output_file!=NULL)
                                    {
                                         execute_redirected_command(cmd);
                                    }
                        else if(strcmp(tk[token_index][0].name, "hop")==0)
                            {
                                if(execute_hop(tk[token_index], find_token_count(tk[token_index]), homedir))
                                    {
                                        hop=true;
                                        strcpy(prev_dir, temp);
                                    }
                                // store(input, commands, command_count);
                                // command_count=(command_count+1)%15;
                            }
                        else if(strcmp(tk[token_index][0].name, "reveal")==0)
                            {
                                execute_reveal(tk[token_index], find_token_count(tk[token_index]), homedir, prev_dir, hop);
                            //     store(input, commands, command_count);
                            //     command_count=(command_count+1)%15;
                            }
                        // else if(strcmp(list[0].name, "log")==0)
                        //     {
                        //         if(count<=2)
                        //             {
                        //                 print_commands(commands, command_count);
                        //             }
                        //         else if(count==3 && strcmp(list[1].name, "purge")==0)
                        //             {
                        //                 command_count=0;
                        //             }
                        //         else if(count==4 && strcmp(list[1].name, "execute")==0 && atoi(list[2].name)<command_count)
                        //             {
                                        
                        //             }
                        //         else
                        //         printf("Invalid log operation\n");
                        //     }
                           
                                 
                                
                                else
                                execute_external_command(tk[token_index], find_token_count(tk[token_index]));
                                // store(input, commands, command_count);
                                // command_count=(command_count+1)%15;
                            }
                        token_index++;
                    }
                free_tokens(list, count);
                
            }
    }