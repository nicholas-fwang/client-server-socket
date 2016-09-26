#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <sys/timeb.h>
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
    int sockid, retcode,addrlen,nread;
    struct hostent *hostp;
    FILE *fp,*copied;
    struct sockaddr_in my_addr, server_addr;
    long fsize;
    struct data *filedata;
    char filename[128];
    char mode[4];
    long ack=0;
    char host[20];
    char connect[8];
    int port;
    clock_t start,finish;
    double diff;
    char copiedfile[128];

    while(1){
        scanf("%s %s %d",connect,host,&port);
        while(strcmp(connect,"connect") != 0){
            printf("Client: Not Connect, rewrite connect\n");
            scanf("%s %s %d",connect,host,&port);
        }
        if((sockid = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0){
            printf("Client: socket failed: %d\n",errno);exit(0);
        }
        memset((char*)&my_addr,0,sizeof(my_addr));
        my_addr.sin_family = AF_INET;
        my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        my_addr.sin_port = htons(0);

        if(bind(sockid,(struct sockaddr *)&my_addr, sizeof(my_addr)) < 0){
            printf("Client: bind fail: %d\n",errno); exit(0);
        }
        if((hostp = gethostbyname(host)) == 0){
            fprintf(stderr, "%s: unkown host\n",host);
            exit(1);
        }
        bzero((char *)&server_addr, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(host);
        memcpy((void *)&server_addr.sin_addr, hostp->h_addr, hostp->h_length);
        server_addr.sin_port=  htons((u_short)port);

        while(1){
            scanf("%s",mode);
            if(strcmp(mode,"put") == 0||strcmp(mode,"get") == 0){
                scanf("%s",filename);
            }else if(strcmp(mode,"close") == 0){
                close(sockid);
                printf("Disconnected.\n");
                break;
            }else if(strcmp(mode,"quit") == 0){
                close(sockid);
                exit(1);
            }else{

            }
            //put
            if(strcmp(mode,"put") == 0){
                start = clock();
                while(1){
                    fp = fopen(filename,"r");
                    if(fp == NULL){
                        printf("Client: File Not Exist\n");
                        printf("Client: rewirte put 'filename'\n");
                        break;
                    }
                    fseek(fp,0L,SEEK_END);
                    fsize = ftell(fp);
                    rewind(fp);
                    filedata = (struct data *)malloc(sizeof(struct data));
                    sprintf(filedata->filename,"%s",filename);
                    sprintf(filedata->mode,"%s",mode);
                    filedata->filesize = fsize;
                    fseek(fp,ack,SEEK_SET);
                    if(fread(filedata->buf,1,sizeof(filedata->buf),fp) < 1){
                        printf("Client: copy to char fail");
                    }
                    //send file data size of BUFSIZE
                    retcode = sendto(sockid,(void *)filedata,(BUFSIZE)+NAMESIZE+LONGDATA+LONGDATA+MODE,0,(struct sockaddr *)&server_addr,sizeof(server_addr));
                    if(retcode <= -1){
                        printf("client: sendto failed: %d\n",errno); exit(0);
                    }
                    
                    //receive ack
                    nread = recvfrom(sockid,filedata,(BUFSIZE)+NAMESIZE+MODE+LONGDATA+LONGDATA,0,(struct sockaddr*)&server_addr,(socklen_t*)&addrlen);
                    ack = filedata->ack;

                    //if server have same file name
                    if(ack != ftell(fp)){
                        printf("File overlaped\n");
                        free(filedata);
                        fclose(fp);
                        ack=0;
                        fsize=0;
                        break;
                    }
                    
                    //create copied file for sended file
                    sprintf(copiedfile,"copied_%s",filedata->filename);
                    copied = fopen(copiedfile,"a");
                    if(copied == NULL){
                        printf("Client copy file fail\n");
                    }
                    fwrite(filedata->buf,1,sizeof(filedata->buf),copied);
                    fclose(copied);
                    finish = clock();
                    diff = (double)(finish-start)/CLOCKS_PER_SEC;
                    if(diff >= 1){
                        start = clock();
                        printf("%s(size: %lu MB) is being sent.\n",filedata->filename,filedata->filesize/1000000);
                        int rate = ((double)filedata->ack / (double)filedata->filesize)*100;
                        int i=0;
                        for(i=0;i<(rate/10);i++){
                            printf("*");
                        }
                        printf("\n");
                    }
                    
                    //check file to send completely
                    if(filedata->ack >=  fsize){
                        free(filedata);         
                        fclose(fp);
                        ack=0;
                        fsize = 0;
                        printf("Successfully transferred.\n");
                        break;
                    }else{
                        free(filedata);         
                        fclose(fp);
                    }
                }
            }
            
            //get
            else if(strcmp(mode,"get") == 0){
                start = clock();
                while(1){
                    fp = fopen(filename,"a");
                    if(fp == NULL){

                    }

                    filedata = (struct data *)malloc(sizeof(struct data));
                    filedata->filesize = fsize;
                    filedata->ack = ack;
                    sprintf(filedata->filename,"%s",filename);
                    sprintf(filedata->mode,"%s",mode);
                    copied = fopen(filedata->filename,"r");
                    if(copied == NULL){
                        printf("Server copied read fail\n");
                    }
                    if(ack != 0){
                        fseek(copied,ack-BUFSIZE,SEEK_SET);
                        if(fread(filedata->buf,1,sizeof(filedata->buf),copied) < 1){
                            printf("Server: copid to char fail\n");
                        }
                    }
                    fclose(copied);
                    
                    //send ack if inital send 0
                    retcode = sendto(sockid,(void *)filedata,BUFSIZE+NAMESIZE+LONGDATA+MODE+LONGDATA,0,(struct sockaddr *)&server_addr,sizeof(server_addr));

                    if(retcode <= -1){
                        printf("Server: sendto failed: %d\n",errno);  
                        exit(0);
                    }


                    //end to receive file data
                    if((ftell(fp) >= filedata->filesize) && filedata->filesize != 0){
                        free(filedata);
                        fclose(fp);
                        ack = 0;
                        fsize=0;
                        printf("Successfully transferred.\n");
                        break;
                    }else{
                    }
                    
                    //receive file data size of BUFSIZE
                    nread = recvfrom(sockid,filedata,BUFSIZE+NAMESIZE+MODE+LONGDATA+LONGDATA,0,(struct sockaddr*)&server_addr,(socklen_t*)&addrlen);
                    fwrite(filedata->buf,1,sizeof(filedata->buf),fp);
                    ack = ftell(fp);
                    fsize = filedata->filesize;
                    
                    //if client have same file name
                    if(ack != filedata->ack){
                        printf("File existed\n");
                        free(filedata);
                        fclose(fp);
                        fsize=0;
                        ack=0;
                        break;
                    }
                    finish = clock();
                    diff = (double)(finish-start)/CLOCKS_PER_SEC;
                    if(diff >= 1){
                        start = clock();
                        printf("%s(size: %lu MB) is being received.\n",filedata->filename,filedata->filesize/1000000);
                        int rate = ((double)ack / (double)filedata->filesize)*100;         
                        int i=0;
                        for(i=0;i<(rate/10);i++){
                            printf("*");
                        }
                        printf("\n");
                    }          

                    fclose(fp);
                    free(filedata);
                }
            }else{
                printf("Client: wrong mode\n");
            }
        }
    }
}
