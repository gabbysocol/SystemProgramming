#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include <errno.h>
#include <libgen.h>
#include <sys/param.h>
#include <dirent.h>
#include <sys/stat.h>
/*
1->2   2->(3,4)   4->5    3->6  6->7  7->8


1->2 SIGUSR2   2->(3,4,5) SIGUSR1   4->6 SIGUSR1   
 3->7 SIGUSR1  5->8 SIGUSR1 8->1 SIGUSR2
*/
/*             0
               |
               1
               |
               2
               | \ 
               3  4
               |  |
               6  5
               |
               7
               |
               8
*/

#define max 100
#define pids "pids.txt"
#define pathPids "/tmp/Lab4"

pid_t pid1;
pid_t pid2;
pid_t pid3;
pid_t pid4;
pid_t pid5;
pid_t pid6;
pid_t pid7;
pid_t pid8;


char * resolved_path;

void call_realpath (char * argv0)
{ 
    resolved_path = basename(argv0);
}

int get_current_time()
{
    struct timeval te; 
    gettimeofday(&te, NULL); 
    return te.tv_usec/1000;
}


//Print pid to file
void write_pid(int num, pid_t pid)
{
    FILE *fptr;
    char * filepath = malloc(1024);
    sprintf(filepath,"%s/%d",pathPids, num);

    fptr = fopen(filepath, "w+");

    fprintf(fptr, "%d", pid);
    fclose(fptr);
}

//Checking "Does all files exists?" 
int all_proccesses_exist(){
	DIR* dir;
	dir = opendir(pathPids);
	if (dir == NULL)
	{
		fprintf (stderr, "%s: opendir() error \n", strerror(errno));
		exit(1);
	}	
        struct dirent* entry;

	int num = -2; //(.. && .)
    while ((entry = readdir(dir)) != NULL)
	{
		num++;				
    }
	closedir(dir);
    return num == 8;
}

//Reading pids from files
void read_pid(int num, int *pid)
{
    DIR* dir;
	dir = opendir(pathPids);
	if (dir == NULL)
	{
		fprintf (stderr, "%s: opendir() error \n", strerror(errno));
		exit(1);
	}	
        struct dirent* entry;

	int curr_pid = 0;
        while ((entry = readdir(dir)) != NULL)
	{
		if ((entry->d_name[0] == 48 + num)&& strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
        {
			char * filepath = malloc(1024);
    		sprintf(filepath,"%s/%s",pathPids, entry->d_name);

			FILE *fptr;
			fptr = fopen(filepath, "r");
			fscanf(fptr, "%d",&curr_pid);
			fclose(fptr);
			break;
		}			
    }
	closedir(dir);
	*pid = curr_pid;

}

//Fill pid variables
int get_pid(int num)
{
    int* pid = 0;
    switch(num){
        case 1:
            pid = &pid1;
            break;
        case 2:
            pid = &pid2;
            break;
        case 3:
            pid = &pid3;
            break;
        case 4:
            pid = &pid4;
            break;
        case 5:
            pid = &pid5;
            break;
        case 6:
            pid = &pid6;
            break;
        case 7:
            pid = &pid7;
            break;
        case 8:
            pid = &pid8;
            break;
    }
    while((*pid) == 0){
        read_pid(num, pid);
    }
    return (*pid);
}

void print_info(int num, int get, int usr)
{
    char got_sent_info[50], curr_usr[50];
    if (get == 0)
        strcpy(got_sent_info,"got ");

    if (usr == 0)
        strcpy(curr_usr, "USR1");        
    else
        strcpy(curr_usr, "USR2");

    strcat(got_sent_info, curr_usr);
    printf("%d %d %d %s %d\n", num, getpid(),getppid(), got_sent_info, get_current_time());
}

void print_term_info(int usr1, int usr2)
{
    printf("%d %d ended work after %d SIGUSR1 and %d SIGUSR2\n", getpid(), getppid(),usr1, usr2);
}

int curr_signals = 0;
int sent_signals1 = 0;
void handle_pr1(int signo)
{
    if(signo == SIGUSR2)
    {
        curr_signals++;
        print_info(1,0,1);
        if (curr_signals == max)
        {
            kill(get_pid(2), SIGTERM);
            int status;
            pid_t wpid;
            while ((wpid = wait(&status)) > 0);
            print_term_info(0, curr_signals);
            exit(0);
        }
        else
        {  
            kill(get_pid(2), SIGUSR2);
            sent_signals1++;
        }
    }
}

int sent_signals2 = 0;
void handle_pr2(int signo)
{
    if(signo == SIGUSR2)
    {
        print_info(2,0,1);
        kill(-get_pid(3),SIGUSR1);  
        sent_signals2++;
    }
    if (signo == SIGTERM)
    {
        kill(get_pid(3),SIGTERM);
        int status;
        pid_t wpid;
        while ((wpid = wait(&status)) > 0);
        print_term_info(sent_signals2, 0);
        exit(0);
    }
}

int sent_signals3 = 0;

void handle_pr3(int signo)
{
    if(signo == SIGUSR1){
        print_info(3,0,0);
        kill(get_pid(7), SIGUSR1);
        sent_signals3++;
    }
    if (signo == SIGTERM){
        kill(get_pid(4), SIGTERM);
        kill(get_pid(6), SIGTERM);
        int status;
        pid_t wpid;
        while ((wpid = wait(&status)) > 0);
        print_term_info(sent_signals3, 0);
        exit(0);
    }
}

int sent_signals4 = 0;
void handle_pr4(int signo)
{ 
    if(signo == SIGUSR1){
        print_info(4,0,0);
        kill(get_pid(6), SIGUSR1);
        sent_signals4++;
    }
    if (signo == SIGTERM){
        kill(get_pid(5), SIGTERM);
        int status;
        pid_t wpid;
        while ((wpid = wait(&status)) > 0);
        print_term_info(sent_signals4, 0);
        exit(0);
    }
   
}

int sent_signals5 = 0;

void handle_pr5(int signo)
{
    if(signo == SIGUSR1){
        print_info(5,0,0);
        kill(get_pid(8), SIGUSR1);
        sent_signals5++;
    }
    if (signo == SIGTERM){
        print_term_info(sent_signals5, 0);
        exit(0);
    }
}

int sent_signals6 = 0;
void handle_pr6(int signo)
{
    if(signo == SIGUSR1)
    {
        print_info(6,0,0);
        sent_signals6++;
    }
    if (signo == SIGTERM)
    {
        kill(get_pid(7), SIGTERM);
        int status;
        pid_t wpid;
        while ((wpid = wait(&status)) > 0);
        print_term_info(sent_signals6, 0);
        exit(0);
    }
   
}

int sent_signals7 = 0;
void handle_pr7(int signo)
{
    if(signo == SIGUSR1)
    {
        print_info(7,0,0);
        sent_signals7++;
    }
    if (signo == SIGTERM)
    {
        kill(get_pid(8), SIGTERM);
        int status;
        pid_t wpid;
        while ((wpid = wait(&status)) > 0);
        print_term_info(sent_signals7, 0);
        exit(0);
    }
   
}

int sent_signals8 = 0;
void handle_pr8(int signo)
{
    if(signo == SIGUSR1)
    {
        print_info(8,0,0);
        kill(get_pid(1), SIGUSR2);
        sent_signals8++;
    }
    if (signo == SIGTERM)
    {
        print_term_info(0, sent_signals8);
        exit(0);
    }
}

void create_process_tree(){
    struct sigaction act1, act2, act3, act4, act5, act6, act7, act8;

    memset(&act1, 0, sizeof(act1));
    act1.sa_handler = handle_pr1;

    memset(&act2, 0, sizeof(act2));
    act2.sa_handler = handle_pr2;

    memset(&act3, 0, sizeof(act3));
    act3.sa_handler = handle_pr3;

    memset(&act4, 0, sizeof(act4));
    act4.sa_handler = handle_pr4;

    memset(&act5, 0, sizeof(act5));
    act5.sa_handler = handle_pr5;

    memset(&act6, 0, sizeof(act6));
    act6.sa_handler = handle_pr6;

    memset(&act7, 0, sizeof(act7));
    act7.sa_handler = handle_pr7;

    memset(&act8, 0, sizeof(act8));
    act8.sa_handler = handle_pr8;

    //1
    if (fork() == 0){

        sigaction(SIGUSR2,  &act1, 0);
        sigaction(SIGTERM, &act1, 0);

        pid1 = getpid();
        write_pid(1,pid1);
        printf("1 %d %d %d\n", getpid(), getppid(), getpgrp());
        
        //2
        if(fork() == 0){
            sigaction(SIGUSR2,  &act2, 0);
            sigaction(SIGTERM, &act2, 0);

            pid2 = getpid();
            setpgid(0, 0);
            write_pid(2, pid2);
            printf("2 %d %d %d\n", getpid(), getppid(), getpgrp());        

            //3
            if (fork() == 0){
                sigaction(SIGUSR1,  &act3, 0);
                sigaction(SIGTERM, &act3, 0);

                pid3 = getpid();
                
                setpgid(0, 0);
                write_pid(3, pid3);

                printf("3 %d %d %d\n", getpid(), getppid(), getpgrp());

                //6
                if (fork() == 0){
                    sigaction(SIGUSR1,  &act6, 0);
                    sigaction(SIGTERM, &act6, 0);
                    pid6 = getpid();
                    
                    setpgid(0, 0);
                    write_pid(6, pid6);
                    printf("6 %d %d %d\n", getpid(), getppid(), getpgrp());

                    //7
                    if (fork() == 0){
                        sigaction(SIGUSR1,  &act7, 0);
                        sigaction(SIGTERM, &act7, 0);

                        pid7 = getpid();


                        setpgid(0,0);
                        write_pid(7, pid7);
                        printf("7 %d %d %d\n", getpid(), getppid(), getpgrp());

                        //8
                        if (fork() == 0){
                            sigaction(SIGUSR1,  &act8, 0);
                            sigaction(SIGTERM, &act8, 0);
                                                
                            pid8 = getpid();
                            setpgid(0,0);
                            write_pid(8, pid8);

                            printf("8 %d %d %d\n", getpid(), getppid(), getpgrp());
                            fflush(0);

                            while(1);
                        } 
                        while(1);
                    }
                    while(1);
                }

                while(1);
            }

            //4
            if (fork() == 0){

                sigaction(SIGUSR1,  &act4, 0);
                sigaction(SIGTERM, &act4, 0);
                        
                pid4 = getpid();
                printf("%d", get_pid(3));
                setpgid(0,get_pid(3));
                write_pid(4, pid4);

                printf("4 %d %d %d\n", getpid(), getppid(), getpgrp());

                //5
                if (fork() == 0){
                    sigaction(SIGUSR1,  &act5, 0);
                    sigaction(SIGTERM, &act5, 0);

                    pid5 = getpid();
                    setpgid(0,get_pid(3));
                    
                    write_pid(5, pid5);
                    printf("5 %d %d %d\n", getpid(), getppid(), getpgrp());
                    while(1);
                }                
                while(1);
            }        


            while(1);
        }  

        while(!all_proccesses_exist());
    }        
    
}

int main(int args, char **argv)
{   
    call_realpath(argv[0]);

    int flag = 1;
	while(flag)
    {
		if (mkdir(pathPids, 0777))
			system("rm -r /tmp/Lab4");				
		else	
			flag = 0;	
	}
    
    create_process_tree();

    if (getpid() == get_pid(1)) 
    {
        kill(get_pid(2), SIGUSR2);
        
        while(1);
    }
    else
    {
        int status;
        pid_t wpid;
        while ((wpid = wait(&status)) > 0);
    }
}
