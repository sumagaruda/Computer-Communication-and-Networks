#include<iostream>
#include<string.h>
#include<string>
#include<stdio.h> // for file related functions
#include<sys/socket.h> //sendto and recvfrom functions
#include<sys/select.h> //for select()
#include<stdlib.h>
#include<string.h> //memset
#include<sys/socket.h>// for socklen_t
#include<arpa/inet.h> //ntop
#include<unistd.h>
#include<inttypes.h>
#include<errno.h>
#include<netdb.h>
#include<assert.h>
#include<stdarg.h>
using namespace std;
/*function to extract the hostname and filename from url and 
send the GET request to the proxy server*/
int request(char* url,char *buffer, char *file_name)
{
    int length;
    char* start= strstr(url, "http://");
    if (start)
    {
       url=url+strlen("http://");
    }
        char* end= strchr (url, '/');
    
    if (end)
    {
        sprintf (file_name, "%s", end+1);
        char hostname[1024];
        strncpy(hostname, url, end-url);
        hostname[end-url]='\0';
        printf("sending request: GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", end, hostname);
		length=sprintf(buffer, "GET %s HTTP/1.0\r\n Host: %s\r\n\r\n", end, hostname);
    }
    else
     {
         sprintf (file_name, "%s", url);
         strcat(file_name, ".html");
         printf("sending request: GET / HTTP/1.0\r\nHost: %s\r\n\r\n", url);
	     length=sprintf(buffer, "GET / HTTP/1.0\r\nHost: %s\r\n\r\n", url);
     }
     
     return 1+length; //returns the number of characters written to the buffer.
}

//for IPv4 and IPv6	
void *get_in_addr (struct sockaddr *sa)
{
  if (sa -> sa_family == AF_INET)
  {
   return &(((struct sockaddr_in*)sa)->sin_addr);
  }
   return &(((struct sockaddr_in6*)sa)->sin6_addr);
 }
 
 int main (int argc, char *argv[])
 {
 int sockfd, numbytes, rv;
 char buffer_request[1024];
 char buffer_response[10240];
 char file_name[50];
 char url[1024];
 char proxyserver[INET6_ADDRSTRLEN];
 struct addrinfo hints, *servinfo, *p;
 
 if (argc!=4)
 {
 fprintf (stderr, "usage: ./client <proxy address> <proxy port> <URL to retrieve>");
 exit(1);
 }
 
 strcpy(url, argv[3]);
 
 memset (&hints, 0, sizeof hints);
 hints.ai_family = AF_UNSPEC;
 hints.ai_socktype=SOCK_STREAM;
 
 if ((rv=getaddrinfo (argv[1], argv[2], &hints, &servinfo))!=0)
 {
 fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (rv));
 return 1;
 }
 
 for (p=servinfo; p!=NULL; p=p->ai_next)
 {
 if ((sockfd=socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1)
 {
 perror ("client:socket");
 continue;
 }
 //establishing the connection
 if (connect(sockfd, p->ai_addr, p->ai_addrlen)==-1)
 {
 close(sockfd);
 perror("client:connect");
 continue;
 }
 break;
 }
 
 if (p==NULL)
 {
 fprintf(stderr, "client: failed to connect \n");
 return 2;
 }
 
 inet_ntop (p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),proxyserver, sizeof proxyserver);
 printf ("client connecting to %s \n", proxyserver);
 freeaddrinfo(servinfo);
 
 memset(file_name, 0, 50);
 
 numbytes=request(url, buffer_request, file_name);
 printf ("filename: %s\n", file_name);
 FILE *ostream;
 
 if (*file_name)
 {
     //Opening a new output file to store the received information from the proxy server.
     if ((ostream=fopen(file_name, "wb"))==NULL)
     {
         perror("error in opening file");
         printf("error: %d (%s) \n", errno, strerror(errno));
         return -1;
     }
 }
 //Sending GET request
 if (send(sockfd, buffer_request, numbytes, 0)==-1)
  {
      perror("send error");
  }
  
 printf ("response from proxyserver: \n");
 int block_num=0;
 size_t bytes_num=0;
 int block_1=1;
 char* ptr_block_1;
 
//Receiving data block by block and writing it to ostream while receiving some data from the proxy server.
do
 {
     if ((numbytes= recv(sockfd, buffer_response, 10239,0))==-1)
     {
         perror("recv error");
         exit(1);
     }
     
     buffer_response[numbytes]='\0';
     bytes_num= bytes_num + numbytes;
     block_num++;
     
     if (*file_name)
     {
         if (block_1)
         {
             ptr_block_1 = strstr (buffer_response, "/r/n/r/n");
             if (ptr_block_1) ptr_block_1 = ptr_block_1 + strlen("/r/n/r/n");
             fwrite(ptr_block_1, 1,numbytes-(ptr_block_1-buffer_response), ostream);
             block_1=0;
         }
         
         else
         fwrite(buffer_response, 1, numbytes, ostream);
     }
 } while (numbytes);
 
 if (*file_name)
 {
     fclose(ostream);
 }
 
 close(sockfd);
 //Closing the socket.
 printf("received %d blocks & %lu number of bytes /n", block_num, bytes_num);
 return 0;
 }
