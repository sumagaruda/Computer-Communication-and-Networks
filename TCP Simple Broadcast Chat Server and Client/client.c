#include<stdio.h>       //printf, fgets, I/O function
#include<stdlib.h>      //size_t, free, exit(), atoi
#include<string.h>      //memset, strcpy, strcmp
#include<sys/types.h>   
#include<sys/socket.h> //sockaddr structure (sa_family, sa_data)
#include<netdb.h>       //addrinfo structure (ai_family, ai_addrelen, *ai_next)
#include<netinet/in.h>   //defines sockaddr_in (sin_family, sin_port, sin_addr, sin_zero)
#include<stdarg.h>       //va_start(), va_arg()
#include<stdint.h>       //int8_t, int16_t, uint32_t
#include<arpa/inet.h>    //htons, ntohl
#include<ctype.h>        //isdigit, isalpha
#include<inttypes.h>     //int8_t, int16_t
#include<unistd.h>       //defines miscellaneous symbolic constants and types and declares miscellaneous functions
#include <sys/time.h>   //to declare the wait time and has timeval and timespec structures

struct attribute
{
    int16_t type;                           // username, message, reason, client count
    int16_t length;                         //length of sbcp attrinute
    char* atr_msg;                          //attribute payload
    
};

struct message
{
    int8_t version;                             //protocol version is 3
    int8_t type;                                //sbcp message type
    int16_t length;                             //length of the sbcp message
    struct attribute *payload;                  // 0 or more sbcp attributes
};

//storing an integer into a buffer
void packin16(char *buffer,unsigned int u)
{
    *buffer++=u>>8; 
    *buffer++=u;
}

//unpacking integer from the buffer
unsigned int unpackin16(char *buffer)
{
    return (buffer[0]<<8)|buffer[1];
}

//packing()-packing the data specified by form variable into buffer
int32_t packing (char *buffer,char *form, ...)
{
    va_list ap;
    int8_t a;    //8-bit value
    int16_t b;  //16-bit value
    char *c;    //string with length not declared
    int32_t total_size=0,str_len; //total size of the buffer after packing in bytes
    
    va_start(ap, form);
    
    for(; *form!='\0';form++)
    {
        switch(*form)
        {
            case 'a':total_size+=1;
                            a=(int8_t)va_arg(ap, int);
                            *buffer++=(a>>0)&0xff;
                            break;
            case 'b': total_size+= 2;
                        b=(int16_t)va_arg(ap,int);
                        packin16(buffer,b);
                        buffer=buffer+2;
                        break;
            
            case 'c': c=va_arg(ap, char*);
                            str_len=strlen(c);
                            total_size+=str_len+2;
                            packin16(buffer,str_len);
                            buffer+=2;
                            memcpy(buffer,c,str_len);
                            buffer+=str_len;
                            break;
        }
    }
va_end(ap);
return total_size;
}

//unpacking()- unpacking the data specified by form variable from the buffer
void unpacking(char *buffer, char *form, ...)
{
    va_list ap;
    int8_t *a;
    int16_t *b;
    char *c;
    int32_t buf_len, counting, strlength=0;
    va_start(ap, form);
    for(; *form!='\0'; form++)
    {
        switch(*form)
        {
            case 'a':
                    a=va_arg(ap, int8_t*);
                    *a=*buffer++;
                    break;
            case 'b':
                    b=va_arg(ap, int16_t*);
                    *b=unpackin16(buffer);
                    buffer+=2;
                    break;
            case 'c':
                    c=va_arg(ap, char*);
                    buf_len=unpackin16(buffer);
                    buffer=buffer+2;
                    if(strlength>0 && buf_len>strlength) 
                    counting=strlength-1;
                    else counting=buf_len;
                    memcpy(c,buffer,counting);
                    c[counting]='\0';
                    buffer+=buf_len;
                    break;
            default:
                    if(isdigit(*form))
                    {
                        strlength=strlength*10+(*form-'0');
                    }
        }
        if(!isdigit(*form)) strlength=0;
    }
        va_end(ap);
}

void *getaddress (struct sockaddr *sa)
{
    if (sa -> sa_family == AF_INET)
      {
       return & (((struct sockaddr_in*)sa) -> sin_addr);
      }
    return & (((struct sockaddr_in6*)sa) -> sin6_addr);
}

int main (int argc, char* argv[])
{
    int sock_fd, total_bytes;
    int16_t packet_size;
    int rcv,length,i;
    struct addrinfo hints, *serverinfo, *p;
    struct message message_send;
    struct attribute attribute_send;
    struct message;
    struct attribute;
    char buffer[1000];
    char username_buffer[16];
    char message_buffer[512];
    fd_set read_fds;  //temp file descriptor list for select()
    
    struct timeval tv;
    int retval;
    tv.tv_sec=60;  //wait upto 60 seconds
    tv.tv_usec=0;
    
    FD_ZERO(&read_fds);
    
    if(argc!=4)
    {
         fprintf (stderr, "correct implementation: ./client username server_ip server_port\n");
         exit(1);
    }
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family= AF_UNSPEC;
    hints.ai_socktype= SOCK_STREAM;
    
    if ((rcv= getaddrinfo(argv[2], argv[3], &hints, &serverinfo))!=0)
      {
         fprintf(stderr, "%s\n", gai_strerror(rcv));
         return 1;
      }
      
    for (p=serverinfo; p!=NULL;p=p->ai_next)
     {
         if((sock_fd=socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1) //get file descriptors for all sockets in the list
         {
             perror("client socket");
             continue;
         }
         
        if(connect(sock_fd, p->ai_addr, p->ai_addrlen)==-1)
        {
            close(sock_fd);
            perror("client connect");
            continue;
        }
    break;
     }
    if(p==NULL)
    {
        fprintf(stderr, "client failed to connect\n");
        return 2;
    }
    
    freeaddrinfo(serverinfo); //finished working with serverinfo structure
    
    strcpy(username_buffer, argv[1]);
    attribute_send.atr_msg=username_buffer;
    attribute_send.type=2;
    attribute_send.length=20;
    message_send.version='3';
    message_send.type='2';
    message_send.length=24;
    message_send.payload=&attribute_send;
    
    //packing the message into sbcp message format
    packet_size=packing(buffer, "aabbbc", message_send.version, message_send.type, message_send.length, attribute_send.type, attribute_send.length, username_buffer);
    if(send(sock_fd,buffer,packet_size,0)==-1)
    {
        perror("send");
        exit(1);
    }
    
    attribute_send.atr_msg=message_buffer;
    attribute_send.type=4;
    attribute_send.length=516;
    message_send.version='3';
    message_send.type='4';
    message_send.length=520;
    message_send.payload=&attribute_send;
    
    FD_SET(0,&read_fds); //adding input from the keyboard to read_fds
    FD_SET(sock_fd, &read_fds); //adding sock_fd to read_fd set
    
    while(1)
    {
        retval=select(sock_fd+1, &read_fds, NULL, NULL,&tv);
        if (retval==-1)
        {
            perror("select");
            exit(4);
        }
        else if(retval)
        {
        //to check if there is any data to be read
        for(i=0; i<=sock_fd;i++)
        {
            if(FD_ISSET(i,&read_fds))
            {
                //data from keyboard
                if(i==0)
                {
                 fgets(message_buffer, sizeof(message_buffer), stdin);
                 length=strlen(message_buffer)-1;
                 if(message_buffer[length]=='\n')
                 message_buffer[length]='\0';
                 packet_size=packing(buffer, "aabbbc", message_send.version, message_send.type, message_send. length,attribute_send.type, attribute_send.length, message_buffer);
                 if(send(sock_fd, buffer,packet_size,0)==-1)
                 {
                     perror("send");
                     exit(1);
                 }
                }
                //data to be read from server
                if(i==sock_fd)
                {
                    if((total_bytes=recv(sock_fd, buffer, 999,0))<=0)
                    {
                        exit(1);
                    }
                    buffer[total_bytes]='\0';
                    printf("%s\n",buffer);
                }
            }
            FD_SET(0,&read_fds);
            FD_SET(sock_fd, &read_fds);
        }
    }
    else 
    {
        printf("Client is idle for 10 seconds hence closing the connection\n");
          return 0;
              
    }
    }
    close(sock_fd);
    return 0;
}
