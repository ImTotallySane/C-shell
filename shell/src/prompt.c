#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<pwd.h>

char homedir[1000];

void printprompt()
    {
        char* username;
        char sysname[1000];
        char cwd[1000];
        struct passwd* pw=getpwuid(getuid());
        username=pw->pw_name;
        gethostname(sysname, 1000);
        getcwd(cwd, 1000);
        // printf("%s", cwd);
        if(strcmp(homedir, cwd)==0)
            {
                printf("<%s@%s:~> ", username, sysname);
            }
        else if(strncmp(homedir, cwd, strlen(homedir))==0 && cwd[strlen(homedir)]=='/')
            {
                char* remaining_dir=cwd+strlen(homedir);
                printf("<%s@%s:~%s> ", username, sysname, remaining_dir);
            }
        else
            {
                printf("<%s@%s:%s> ", username, sysname, cwd);
            }
       // printf("\n%s", homedir);
    }
