#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>


int PID,P_Stat,status;

void checkUsage(int argc,char *argv[]){
    if(argc<2){
        perror("Insufficient Arguments. Usage: ./a.out fis.txt <num1> <num2> ...\n");
    }
}

void makePr1(int argc,char* argv[]){
    for(int i=2;i<=argc;i++){
        if((PID=fork())<0){
            perror("Not positive number.\n");
        } 
        if(PID==0){
            for(int j=1;j<=atoi(argv[i]);j++){
                printf("PID: %d %d\n",getpid(),j);
            }
        }
    }
    exit(0);
}

void makePr2(char* argv[]){
    if((PID=fork())<0){
        perror("Malfunction.");
        exit(0);
    } 
    if(PID==0){
        execlp("grep","grep","ip",argv[1],NULL);
        perror("Execlp not working.");
        exit(0);
    }
}

void prParent(int argc){
    for(int k=0;k<argc-1;k++){
        P_Stat=wait(&status);
        if(P_Stat<0){
            perror("Waiting error.");
            exit(0);
        }
        if(WIFEXITED(status)){
            printf("Child %d terminated with status %d\n",P_Stat,WIFEXITED(status));
        }
    }
}

int main(int argc,char *argv[]){
    checkUsage(argc,argv);
    makePr1(argc,argv);
    makePr2(argv);
    prParent(argc);
    return 0;
}