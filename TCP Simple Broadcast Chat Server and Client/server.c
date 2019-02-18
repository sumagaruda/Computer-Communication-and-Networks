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
#include<stdbool.h>
#include <math.h>


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
    *buffer++ =u>>8; 
    *buffer++ = u;
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
            case 'a':       total_size+=1;
                            a=(int8_t)va_arg(ap, int);
                            *buffer++=(a>>0)&0xff;
                            break;
            
            case 'b':   total_size+=2;
                        b=(int16_t)va_arg(ap,int);
                        packin16(buffer,b);
                        buffer+=2;
                        break;
            
            case 'c':       c=va_arg(ap, char*);
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
                    buffer+=2;
                    if(strlength>0 && buf_len>strlength) 
                    {
                        counting=strlength-1;
                    }
                    else 
                    {
                        counting=buf_len;
                    }
                    
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
        if(!isdigit(*form)) 
        strlength=0;
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
    
    char buffer[1000];
    char username[16];
    char client_names[100][16];
    char name[100];
    char buffer_sent_message [500];
    char buffer_rcvd_message[512];
    char exit_details[50];
    struct message message_rcvd;
    struct attribute attribute_rcvd;
    
    fd_set master;             //add fd to the master file descriptor list
    fd_set read_fds;           //temp file descriptor list for select()
    int fdmax;                 //maximum file descriptor number
    
    FD_ZERO (&master);         //like memset for fds, clearing operation
    FD_ZERO (&read_fds); 
    
    int listen_fd;
    int new_fd;
    
    struct sockaddr_storage remoteaddr;        //for storing client address
    socklen_t addrlen;
    struct addrinfo hints, *ai, *p;
    
    int rcvd_bytes;           // number of bytes read by the recv function
    int optval=1;
    int i,j=0;
    int count=0;
    char str_count[5];
    char count_display[100];
    int u,m;
    int rcv=0;
    FILE *fp;
    
    if (argc!=4)
      {
         fprintf (stderr, "correct implementation: ./server server_ip server_port max_clients\n");
         exit(1);
      }
    
    int max_clients= atoi(argv[3]);  //maximum clients allowed
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family= AF_UNSPEC;
    hints.ai_socktype= SOCK_STREAM;
    
    if ((rcv= getaddrinfo(argv[1], argv[2], &hints, &ai))!=0) // fills out the prederfined structs such as addrinfo
      {
         fprintf(stderr, "%s\n", gai_strerror(rcv));  //gai_strerror for errors specific to getaddrinfo
         exit(1);
      }
      
    
    for (p=ai; p!=NULL;p=p->ai_next)
     {
         listen_fd= socket (p->ai_family, p->ai_socktype, p->ai_protocol); //get file descriptors for all sockets in the list
         
         if (listen < 0) //if there is an error, go through socket call again
          {
             continue;
          }
          
          
    setsockopt (listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (int)); //used for multicast
    
    if (bind (listen_fd, p->ai_addr, p->ai_addrlen) <0) // bind the socket to the server port
     {
         close (listen_fd);
         continue;
     }
    
     break;
     }
          
     if(p==NULL)
    {
        fprintf(stderr, "failed to bind\n");
        exit(2);
    }
     
     freeaddrinfo (ai); //free address space
     
     if (listen (listen_fd,3) == -1) // listen for any new connections upto 3 clients
     {
         perror ("listen");
         exit(3);
     }
     
     printf ("%s", "listen works");
     
     FD_SET (listen_fd, & master);      //adding listen_fd to master list
     fdmax= listen_fd;
     
     for (;;)
     {
         read_fds= master;    // listen_fd is added to read_fds
         if (select (fdmax+1, &read_fds, NULL, NULL, NULL) == -1)  //helps the server understand which has messages to forward and who to forward these messages to.
          {
              perror ("select");
              exit(4);
          }
          
          for (i=0; i<=fdmax; i++)  //looking for data to read
          {
             if (FD_ISSET (i, &read_fds))
               { 
                 if (i == listen_fd)    // client waiting to connect
                   {
                       if (count < max_clients)
                         {
                             addrlen = sizeof remoteaddr;
                             new_fd= accept (listen_fd, (struct sockaddr*)&remoteaddr, &addrlen);
                             if (new_fd == -1)
                               {
                                   perror ("accept");
                               }
                              else
                               {
                                   FD_SET (new_fd, &master);
                                   if (new_fd > fdmax)
                                    {
                                        fdmax=new_fd;
                                    }
                                    count ++;
                               }
                         }
                   }
             else 
             {
                 if ((rcvd_bytes = recv (i, buffer, 1000, 0)) <= 0)
                  { 
                     
                      if (rcvd_bytes == 0)
                      {
                          sprintf (exit_details, " %s OFFLINE : client connection has closed", client_names[i]);
                          client_names[i][0]='\0';
                      
                      for (j=0; j <= fdmax; j++)
                      {
                          if (FD_ISSET (j, &master))
                          {
                              if (j!= listen_fd && j!=i)
                              {
                                if (send (j, exit_details, sizeof exit_details, 0)==-1)
                                 {
                                     perror ("send");
                                 }
                              }
                          }
                      }
                    }
              else
                      {
                          perror ("recv");
                      }
             count --; 
             close (i);
             FD_CLR (i, &master);
            
          }
          
              else
              {
                  unpacking(buffer,"aabbb", &message_rcvd.version, &message_rcvd.type, &message_rcvd.length, &attribute_rcvd.type, &attribute_rcvd.length);
                  
                  if (message_rcvd.type=='2' && attribute_rcvd.type==2)
                  {
                      unpacking (buffer + 8, "c", username);
                      int j,m;
                      int u=1;
                      for (j=1; j<=fdmax;j++)
                       {
                          if (strcmp(username,client_names[j])== 0)
                           {
                               u =0; //username already in use
                               if (send (i, "choose another username for client", 50, 0)== -1)
                                   {
                                       perror("send");
                                   }
                                count--;
                               close(i);
                               FD_CLR (i, &master);
                              break;
                        }
                  }
                  
                
                       if (u == 1)
                       {   
                           sprintf (client_names[i], "%s", username);
                           sprintf (count_display, "%i", count);
                           strcpy (name, "names of clients in the chat session:");
                       
                       for (m=4; m<=fdmax; m++)
                       {
                           strcat (name, client_names[m]);
                           strcat (name, " ");
                       }
                         strcat (name, count_display);
                       if (send (i, name, 100, 0) == -1)
                       {
                           perror("send");
                       }
                       
                  }
              }
              
          if (message_rcvd.type == '4' && attribute_rcvd. type == 4)
          {
              unpacking (buffer +8 , "c", buffer_rcvd_message);
              sprintf (buffer_sent_message, "%s : %s", client_names[i], buffer_rcvd_message);
          
          for (j=0; j <=fdmax; j++)
          {
              if (FD_ISSET (j, &master))
              {
                  if (j!=i && j!=listen_fd)
                  {
                      if (send (j, buffer_sent_message, rcvd_bytes, 0 )== -1)
                      {
                          perror("send");
                      }
                  }
              }
          }
          }
}
}
}
}
}
    return 0;
}
