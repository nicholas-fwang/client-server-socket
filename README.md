# 프로젝트 설명
- Client와 Server가 socket으로 Connection 됩니다.
- Client의 PUT 명령어를 이용해 Server에 파일을 전송하고 복사본을 만듭니다.
- Client의 GET 명령어를 이용해 Server의 파일을 가져옵니다.

![ScreenShot](https://github.com/fisache/client-server-socket/blob/master/network.png)
# 개발 환경
- OS : Mac
- Compiler : gcc
- Build System :
- Development Tool : vi

# 역할
- Client와 Server 코드를 작성했습니다.

# 사용한 기술
- Socket : Client와 Server 간의 통신을 위해 C의 Socket 라이브러리를 사용했습니다.

# 설명

```c
struct data{
    char filename[NAMESIZE];
    long filesize;
    char buf[BUFSIZE];
    char mode[MODE];
    long ack;
};
```
<pre>
Client-Server 간의 파일 통신을 위해 구조체를 만듭니다.
파일의 이름과 크기, 내용이 저장되는 버퍼, 전송 모드 그리고 제대로 전송되는지 여부를 확인하는 ack을 선언합니다.
</pre>

client.c <br />
```c
//send file data size of BUFSIZE
retcode = sendto(sockid,
        (void *)filedata,
        (BUFSIZE)+NAMESIZE+LONGDATA+LONGDATA+MODE,
        0,
        (struct sockaddr *)&server_addr,sizeof(server_addr));
if(retcode <= -1){
    printf("client: sendto failed: %d\n",errno); exit(0);
}              
//receive ack
nread = recvfrom(sockid,
        filedata,(BUFSIZE)+NAMESIZE+MODE+LONGDATA+LONGDATA,
        0,
        (struct sockaddr*)&server_addr,(socklen_t*)&addrlen);
ack = filedata->ack;

```
<pre>
Client가 Server에 파일을 전송하는 로직입니다. sendto()를 이용해 buf 크기만큼 파일을 보낸 뒤,
recvfrom()으로 ack을 확인해 제대로 전송됐는지 확인합니다.
</pre>

```c
//create copied file for sended file
sprintf(copiedfile,"copied_%s",filedata->filename);
copied = fopen(copiedfile,"a");
fwrite(filedata->buf,1,sizeof(filedata->buf),copied);
```
<pre>
ack 확인으로 제대로 전송됐다면 buf를 복사본 파일에 씁니다.
</pre>

```c
if(ack != 0){
    fseek(copied,ack-BUFSIZE,SEEK_SET);
    if(fread(filedata->buf,1,sizeof(filedata->buf),copied) < 1){
        printf("Server: copid to char fail\n");
        }
}
fclose(copied);
                    
//send ack if inital send 0
retcode = sendto(sockid,
        (void *)filedata,
        BUFSIZE+NAMESIZE+LONGDATA+MODE+LONGDATA,
        0,
        (struct sockaddr *)&server_addr,sizeof(server_addr));

```
<pre>
Server로부터 buf 크기만큼 파일을 읽고 ack을 보냅니다.
</pre>

<pre>
Server의 경우도 Client와 비슷한 구조로 작성되어 PUT, GET 명령어에 따라 Client와 반대로 동작하게 됩니다.
</pre>

# 결론
<pre>
기존에 Web Container만 사용해 Socket 통신을 제대로 구현해 본 적이 없었는데 프로젝트를 통해 Socket 통신의
동작 원리에 대해 깊게 공부할 수 있었습니다.
이를 활용해 실시간 통신이 필요한 채팅 서버, 파일 송수신하는 FTP 서버의 통신을 구현할 때 도움이 될 것 같습니다.
</pre>

