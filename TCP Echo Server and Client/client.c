#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <strings.h>
#include <arpa/inet.h>
#include <errno.h>

int writen(int sockfd,char sendline[])
{
    ssize_t actualsize;
    int i;
    int readsize=strlen(sendline);
    printf("%s","length of message received ");
    printf("%i \n",readsize-1);
    actualsize=write(sockfd,sendline,strlen(sendline));
    printf("%s","length of message written ");
    printf("%zu \n",actualsize-1);
    
   if (actualsize<readsize)
    {   
        for (i=actualsize;i<=readsize;i++)
        {
            write(sockfd,(void*)&sendline[i],1);
        }
    }    
    return actualsize;
}

ssize_t read_line(int newfd,void *buffer, size_t n)
{
    ssize_t read_bytes=0;                   
    size_t total_read=0;         
    char *buf=NULL;
    char ch;
    buf = buffer;       

    while(1) 
    {   memset(&ch,' ',1);
    
        read_bytes = read(newfd, &ch, 1);

        if (read_bytes > 0)
        {                        
            if (total_read < n-1)
            {      
                *buf= ch;
                 buf++;
                 total_read++;
           // printf("buf1=%s", buf-1);    
            }
        }
        
        if (read_bytes == -1) 
        {
            if (errno == EINTR)         
                continue;
            else
                return (-1);              
        } 
        
        if (read_bytes == 0) 
        {   
                exit(1);  //end of file
        }
        
            if (ch == '\n')
                break;
        }

    *buf = '\0';
    //printf("buf=%s", buf-total_read);  
    return total_read+1;
}

int main(int argc,char *argv[])
{
    int sockfd;
    char writeline[100];
    char read[100];
    struct sockaddr_in sadr;
    int port;
    port=atoi(argv[2]);
    
    sockfd=socket(PF_INET,SOCK_STREAM,0);
    
    if (sockfd==-1)
    {
        perror("socket");
        exit(-1);
    }
    else
    {
        fputs("socket created \n", stdout);
    }
    
    memset(&sadr,0,sizeof sadr);
 
    sadr.sin_family = AF_INET;
    sadr.sin_port = htons(port);
    inet_pton ( AF_INET, argv[2], &(sadr.sin_addr));
    
   if ( connect ( sockfd ,(struct sockaddr *)&sadr, sizeof(sadr))==-1)
   {
       close(sockfd);
       perror("connection failed");
       return 1;
   }
   else
   fputs ("connected ", stdout);

    while(1)
    {
        memset(writeline,0,100);
        memset(read,0,100);
        fgets(writeline,100,stdin); 
 
        if (writen(sockfd,writeline)<0)
        {
            printf("%s", "write error");
        }
        
        read_line(sockfd,read,100);
        fputs(read,stdout);
        
    }
    close(sockfd);
}
