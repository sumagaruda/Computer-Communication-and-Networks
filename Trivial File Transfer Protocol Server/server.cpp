//mode is netascii(for text) or octet(for binary) depending on the file
#include<iostream>
#include<fstream>
#include<string>
#include<stdio.h> // for file related functions
#include<sys/socket.h> //sendto and recvfrom functions
#include<netinet/in.h> // for htons
#include<sys/select.h> //for select()
#include<sys/wait.h> 
#include<stdlib.h>
#include<netdb.h> //hret
#include<string.h> //memset
#include<sys/socket.h>// for socklen_t
#include<arpa/inet.h> //ntop
#include<sys/types.h>
#include<unistd.h>
#include<fcntl.h>
#include<signal.h>
#include<cstdlib> 

using namespace std;

//opcode rrq 1 data 3 ack 4 error 5
#define rrq 1
#define data 3
#define ack 4
#define error 5

//max block size 32MB, 2^16*512, 512 byte bocks each
#define block_size 512

//defining tftp block structure
struct tftp
{
    char * file_name;
    uint16_t opcode; 
    uint16_t block_num;
    char * mode;
};

char temp[512];
 
/*void cr(FILE *src, FILE *dest)
{

char c;

while (is.get(c)!=EOF)
{    
    if (c=='\n')
    {
        fputc('\r', dest);
        fputc('\n', dest);
    }
    
    else if (c=='\r')
    {
        fputc('\r', dest);
        fputc('\0', dest);
    }
    
    else
    fputc (c,dest);
} 
} 
*/
//converting from host byte order to network byte order for 16 bits
void hton_16 (char *buffer, unsigned short int k) 
{
    k=htons(k);
    memcpy(buffer, &k, 2);
}

//converting from network byte order to host byte order for 16 bits
unsigned short int ntoh_16 (char * buffer)
{
    unsigned short int k;
    memcpy(&k, buffer, 2);
    k=ntohs(k);
    return k;
}

//to reap dead processes and avoid zombie processes
void sigchld_handler (int s)
{
    int errno;
    int saved_errno=errno;
    while (waitpid(-1, NULL, WNOHANG)>0);
    errno=saved_errno;
}


void *get_inaddr (struct sockaddr *sa)  //get sockaddr
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr); //IPv4
    }
     return &(((struct sockaddr_in6*)sa)->sin6_addr); //IPv6
}

struct tftp *decoding (char * block)
{
    struct tftp *result;
    result=(struct tftp *)malloc(sizeof(struct tftp));
    result->opcode=ntoh_16(block);
    
    if (result->opcode==rrq)
    {
    char temp[512];
    int m=2, n=0;
    
    while (block[m]!='\0')
    temp[n++]=block[m++];
    temp[n]='\0';
    
    result->file_name=(char *)malloc(strlen(temp)*sizeof(char));
    strcpy(result->file_name,temp);
    
    while(block[m]=='\0')
    m++;
    
    n=0;
    while(block[m]!='\0')
    temp[n++]=block[m++];
    
    temp[n]='\0';
    result->mode =(char *)malloc(strlen(temp)*sizeof(char));
    strcpy(result->mode, temp);
    }

else if(result->opcode==ack)
{
    result->block_num=ntoh_16(block+2);
}
return result;
}

char *encoding(uint16_t opcode, uint16_t block_num, char *msg, int length)
{
    char *block;
    if (opcode==data)
    {
        block= (char *)malloc((length+4)*sizeof(char));
        hton_16(block,opcode);
        hton_16(block+2, block_num);
        memcpy(block+4, msg, length);
        return block;
    }
    
    else if (opcode ==error)
    {
        block= (char *)malloc((length+5)*sizeof(char));
        hton_16(block,opcode);
        hton_16(block+2, block_num);
        memcpy(block+4, msg, length);
        memset(block+4+length,'\0', 1);        
        return block;
    }
    return NULL;
}

int main (int argc, char *argv[])
{
    if (argc!=3)
    {
        fprintf (stderr, "usage: ./server IP port \n");
        return 1;
    }
    
    struct sigaction sa;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage address_client, temp_address;
    socklen_t address_length, temp_length;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_DGRAM;
    
    int rv;
    if ((rv=getaddrinfo(argv[1], argv[2], &hints, &servinfo))!=0)
    {
        fprintf(stderr, "getaddrinfo: %s \n",gai_strerror(rv));
        return 1;
    }
    
    int sock_fd;
    for (p=servinfo; p!=NULL; p=p->ai_next)
    {
        if ((sock_fd=socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1)
        {
            perror ("socket error");
            continue;
        }
        
        if (bind(sock_fd, p->ai_addr, p->ai_addrlen)==-1)
        {
            close(sock_fd);
            perror ("bind error");
            continue;
        }
        break;
    }
    
   char buffer[1024];
   int nbytes;
    
struct sockaddr new_sa;
new_sa =*(p->ai_addr);
struct sockaddr_in* new_address;
new_address = (struct sockaddr_in*) &new_sa;
new_address->sin_port= htons(0);

sa.sa_handler=sigchld_handler; 
sigemptyset(&sa.sa_mask);
sa.sa_flags=SA_RESTART;
if (sigaction(SIGCHLD, &sa, NULL)==-1)
{
    perror("sigaction");
    exit(1);
}

fd_set master;
fd_set read_fds;
int fdmax;

FD_ZERO(&master);
FD_ZERO(&read_fds);

while (1)
{
    address_length=sizeof(address_client);
    if ((nbytes=recvfrom(sock_fd, buffer, 1023, 0, (struct sockaddr *)&address_client, &address_length))==-1)
    {
        perror ("recvfrom");
        exit(2);
    }
    
    struct sockaddr_in* new_client = (struct sockaddr_in*)&address_client;
    struct tftp *rcv;
    rcv = decoding (buffer);
    cout<<"opcode ="<<rcv->opcode<<", Filename ="<<rcv->file_name<<", mode ="<<rcv->mode<<endl;
    
    
    if (rcv->opcode !=rrq)
    {
        cout<<"invalid opcode received"<<endl;
        continue;
    }
    
    if (!(fork()))
    {
        close(sock_fd);
        if((sock_fd=socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1)
        {
            perror ("socket error");
        }
        
        if ((rv=bind(sock_fd, &new_sa, sizeof(new_sa)))==-1)
        {
            perror ("bind error");
        }
        
        socklen_t length= sizeof(new_sa);
        if (getsockname(sock_fd, &new_sa, &length)==-1)
        {
            perror ("getsockname error");
        }
        
        ifstream file;
        file.open(rcv->file_name, ios::in|ios::binary);
        if(file.is_open()==false)
        {
            string msg="Could not open requested file";
            cout << msg << rcv->file_name << endl;
            
            char *errmsg=(char *)malloc(1024*sizeof(char));
            errmsg=strcpy(errmsg,msg.c_str());
            char *errblock=encoding(error,1,errmsg,(strlen(errmsg)));
            free(errmsg);
            if ((nbytes=sendto(sock_fd,errblock, (strlen(errmsg)+5),0,(struct sockaddr *)&address_client, address_length))==-1)
            {
                perror ("sendto");
                exit(2);
            }
            exit(0);
        }
        
        streampos begin_file,end_file;
        begin_file= file.tellg();
        file.seekg(0,ios::end);
        end_file=file.tellg();
        file.seekg(0,ios::beg);
        
        unsigned long file_size= end_file-begin_file;
        unsigned long nblocks= (file_size/512) +1;
        
        cout<<"file size ="<<file_size<<", blocks="<<nblocks<<endl;
       
       /*if (strcmp(rcv->mode, "netascii")==0)
        {
            char temp_name[]= "/temp/file_a";
            int fd= mkstemp(temp_name);
            close(fd);
            ofstream temp_file= (fopen(temp_name,"w+"));
            cr(file, temp_name);
        }
        */
        
        char file_buffer[513];
        int ack_no=0, retransmit =0, msg_length;
        int block_num=0;
        unsigned long net_ack_no=0;
        
        free(rcv->file_name);
        free (rcv->mode);
        free(rcv);
        
        FD_SET(sock_fd, &master);
        fdmax=sock_fd;
        struct timeval tv;
        tv.tv_sec= 0;
        tv.tv_usec=100000;
        
        while(net_ack_no<nblocks)
        {
            if (ack_no==block_num)
            {
                file.read(file_buffer, 512);
                
               /* ifstream.open(file_buffer)
                    char c;
                    int ch;
                    ch= file_buffer.get(c);
                 while (ch!=EOF)
                   {    
                       if (ch=='\n')
                         {
                           file_buffer.put('\r');
                           file_buffer.put('\n');
                         }
    
                        else if (ch=='\r')
                         {
                           file_buffer.put('\r');
                           file_buffer.put('\0');
                         }
    
                        else
                        file_buffer.put (ch);
                    }
                */
                
                msg_length=file.gcount();
                block_num=(block_num+1)%65536;
                retransmit=0;
            }
            
            if (retransmit>0)
            {
                if(retransmit==11)
                {
                    cout<<"request timedout"<<endl;
                    break;
                }
                
            cout<<"retransmitting attempt"<<retransmit<<"for block"<<block_num<<endl;
            }
            char *block=encoding (data, block_num, file_buffer, msg_length);
            if ((nbytes=sendto(sock_fd, block, msg_length+4, 0, (struct sockaddr *)&address_client, address_length))==-1)
            {
                perror("sendto error");
                exit(1);
            }
            free(block);
            
            tv.tv_usec=100000;
            read_fds=master;
            if(select(fdmax+1, &read_fds, NULL, NULL, &tv)==-1)
            {
                perror("select error");
                exit(1);
            }
            
            if (FD_ISSET(sock_fd, &read_fds))
            {
                temp_length=sizeof(temp_address);
                if ((nbytes=recvfrom(sock_fd, buffer, 4, 0, (struct sockaddr *)&temp_address, &temp_length))==-1)
                {
                    perror ("recvfrom error");
                    exit(1);
                }
                
                struct sockaddr_in* temp_client_1 = (struct sockaddr_in*) &address_client;
                if(temp_client_1->sin_addr.s_addr==new_client->sin_addr.s_addr)
                {
                    rcv=decoding(buffer);
                    ack_no=rcv->block_num;
                    net_ack_no++;
                    free(rcv);
                }
            }
            retransmit++;
        }
        
        file.close();
        close(sock_fd);
        if (net_ack_no == nblocks)
        {
            cout<<"acknowledgement number "<<ack_no<<endl;
            cout<<"block number "<<nblocks<<endl;
            cout<<"file transmitted successfully "<<endl;
        }
        return 0;
        exit(0);
    }
       free(rcv->file_name);
       free(rcv->mode);
       free(rcv);
}
close(sock_fd);
freeaddrinfo(servinfo);
return 0;
}
