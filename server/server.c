#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#define BUFSIZE 1024
#define NAMESIZE 128
#define MODE 8
#define LONGDATA 8

struct data{
    char filename[NAMESIZE];
    long filesize;
    char buf[BUFSIZE];
    char mode[MODE];
    long ack;
};

int main(int argc, char *argv[]){
    int sockid, nread, addrlen,retcode;
    struct sockaddr_in my_addr, client_addr;
    FILE *fp,*copied;
    struct data *filedata;
    long fsize= 0.0L;
    long *ack;
    clock_t start,finish;
    double diff;
    char copiedfile[128];

    if(argc != 2){
        printf("%s port\n", argv[0]);
        return 0;
    }
    printf("Server: creating socket\n");
    if((sockid = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0){
        printf("Server: socket failed: %d\n",errno);exit(0);
    }
    printf("Server: binding my local socket\n");

    memset((char*)&my_addr,0,sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(atoi(argv[1]));
    if(bind(sockid,(struct sockaddr *)&my_addr, sizeof(my_addr)) < 0){
        printf("Server: bind fail: %d\n",errno); exit(0);
    }
    start = clock();
    while(1){
        filedata = (struct data *)malloc(sizeof(struct data));
        addrlen = sizeof(client_addr);
        
        //receive file data or ack
        nread = recvfrom(sockid,filedata,(BUFSIZE)+NAMESIZE+MODE+LONGDATA+LONGDATA,0,(struct sockaddr*)&client_addr,(socklen_t*)&addrlen);

        if (nread >0) {
            //get
            if(strcmp(filedata->mode,"get") == 0){
                fp = fopen(filedata->filename,"r");
                if(fp == NULL){
                    printf("Server: No input file\n");
                }

                fseek(fp,0L,SEEK_END);
                fsize = ftell(fp);
                rewind(fp);
                //create copied file
                sprintf(copiedfile,"copied_%s",filedata->filename);
                copied = fopen(copiedfile,"a");
                if(copied == NULL){
                    printf("Client copy file fail\n");
                }
                if(filedata->filesize > ftell(copied)){
                    fwrite(filedata->buf,1,sizeof(filedata->buf),copied);
                }
                fclose(copied);
                filedata->filesize = fsize;
                fseek(fp,(filedata->ack),SEEK_SET);
                if(filedata->filesize > ftell(fp)){
                    if(fread(filedata->buf,1,sizeof(filedata->buf),fp) < 1){
                        printf("Server: copy to char fail\n");
                    }
                }

                filedata->ack = ftell(fp);
                //sned file data
                retcode = sendto(sockid,(void *)filedata,(BUFSIZE)+NAMESIZE+LONGDATA+MODE+LONGDATA,0,(struct sockaddr *)&client_addr,sizeof(client_addr));
                if(retcode <= -1){
                    printf("client: sendto failed: %d\n",errno); exit(0);
                }
                if(diff >= 1){
                    start = clock();
                    int rate = ((double)filedata->ack / (double)filedata->filesize)*100;
                    printf("Transfer status:send [%s] [%d%%, %lu MB / %lu MB]\n",filedata->filename,rate,filedata->ack/1000000,filedata->filesize/1000000);
                }
                free(filedata);
                fclose(fp);
            //put
            }else if(strcmp(filedata->mode,"put") == 0){

                fp = fopen(filedata->filename,"a");
                if(fp == NULL){
                    printf("Server: failed write");
                }
                //write file
                fwrite(filedata->buf,1,sizeof(filedata->buf),fp);
                filedata->ack = ftell(fp);
                copied = fopen(filedata->filename,"r");
                if(copied == NULL){
                    printf("Server copied read fail\n");
                }
                fseek(copied,filedata->ack-BUFSIZE,SEEK_SET);
                if(fread(filedata->buf,1,sizeof(filedata->buf),copied) < 1){
                    printf("Server: copid to char fail\n");
                }
                fclose(copied);
                
                //send ack
                retcode = sendto(sockid,(void *)filedata,BUFSIZE+NAMESIZE+LONGDATA+MODE+LONGDATA,0,(struct sockaddr *)&client_addr,sizeof(client_addr));
                if(retcode <= -1){
                    printf("Server: sendto failed: %d\n",errno);  
                    exit(0);
                }
                if(diff >= 1){
                    start = clock();
                    int rate = ((double)filedata->ack / (double)filedata->filesize)*100;
                    printf("Transfer status:recv [%s] [%d%%, %lu MB / %lu MB]\n",filedata->filename,rate,filedata->ack/1000000,filedata->filesize/1000000);
                }
                free(filedata);
                fclose(fp);
            }
        }
        finish = clock();
        diff = (double)(finish-start)/CLOCKS_PER_SEC;
    }
    close(sockid);
}
