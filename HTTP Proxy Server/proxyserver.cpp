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

#include<vector>
#include<map>
#include<algorithm>
#include<list>
using namespace std;
//Structure to store requested url and socket information related to the client.
struct information_client
{
 string path;
 char hostname[1024];
 int client_fd;
 int host_fd;
};

vector<information_client*> list_client;
//Structure to define data blocks
struct block_data
{
 char data[10240];
 int data_size;
};
//Structure to store information related to a document.
struct document
{
 vector <block_data*> block;
 string path_name;
 char expires[255];
 char etag[255];
 time_t last_modified, expiry_time;
 bool check;
};
//Structure to store cache information where path of the document is mapped to the pointer to document.
struct cache
{
 map<string,document*> document_map;
 list<string> list_path;
};
//function to extract information from the headers.
bool info_extract(const char* key, char* buffer, char* extracted_info)
 {
  char *begin=strstr(buffer,key);
  if(!begin)
  {
   return false;
  }
  else
  {
  char* end=strstr(begin, "\r\n");
  begin= begin+ strlen(key);
  while(*begin==' ') ++begin;
  while(*(end-1)==' ') --end;
  strncpy(extracted_info, begin, end-begin);
  extracted_info[end-begin]='\0';
  //cout<<"Extracted Info is :"<< extracted_info<<endl;
  return true;
  }
 }
 //Function to extract the time and convert it into easily comparable format.
 time_t retrieve_time(const char* key, char* buffer)
 {
  char extracted_time[255]={0};
  bool x= info_extract(key, buffer, extracted_time);
    if(!x)
  {
   return 0;
  }
  struct tm t={0};
  char* t_ptr=strptime(extracted_time, "%a, %d-%b-%Y %H:%M:%S %Z", &t);
  if(!t_ptr) return 0;
  else return mktime(&t);
 }
//Fucntion to get the current time of the system.
time_t Gettingtime()
 {
  time_t raw_t;
  time(&raw_t);
  struct tm *pointer_t;
  pointer_t=gmtime(&raw_t);
  return mktime(pointer_t);
 }
 //Function to retrieve etag and expires field information
 void retrieve_time_info (char* buffer, const int numbytes, document* document_pointer)
 {
  time_t expires_t, lastmodified_t;
  expires_t= retrieve_time("expires=", buffer) ;
  if(!expires_t)
  {
   expires_t= retrieve_time("expires:", buffer) ;
  }
  lastmodified_t=retrieve_time("Last-Modified:", buffer);
  if(expires_t)
  {
   document_pointer->expiry_time=expires_t;
  }
  if(lastmodified_t)
  {
   document_pointer->last_modified=lastmodified_t;
  }
  if(info_extract("Etag: ", buffer, document_pointer->etag)==false)
  {
   info_extract("ETag: ", buffer, document_pointer->etag);
  }
  info_extract("expires=", buffer, document_pointer->expires);
  cout<< "Expires: "<< document_pointer->expires<<endl;
  cout<<"Etag: "<<document_pointer->etag<<endl;
 }
 //To delete blocks of data when cache is full or the entry is not current.
 void deleting_block(document* document_pointer)
 {
  vector<block_data*>::iterator it;
  for (it=document_pointer->block.begin();it!=document_pointer->block.end();it++)
  {
   delete *it;
  }
  document_pointer->block.clear();
 }
 //To add a new document to cache when it is not present in the cache already.
  bool addingdocument(document* document_pointer, cache* proxy_cache)
 {
  proxy_cache->list_path.push_front(document_pointer->path_name);
  proxy_cache->document_map[document_pointer->path_name]=document_pointer;
  
  if(proxy_cache->list_path.size() <= 10)
  {
   cout << "number of documents in cache is less than 10" <<endl;
   return false;
  }
  else
  {
  string path_name= proxy_cache->list_path.back();
  cout<< "cache size exceeded, deleting path:" << path_name << endl;
  deleting_block(proxy_cache->document_map[path_name]);
  delete proxy_cache->document_map[path_name];
  proxy_cache->document_map.erase(path_name);
  proxy_cache->list_path.pop_back();
  return true;
  }
 }
 //Retrieving the document when the path is known and is present in the cache.
 document* getting_document(string path_name, cache* proxy_cache)
{
 if(proxy_cache->document_map.find(path_name)==proxy_cache->document_map.end())
 return NULL;
 proxy_cache->list_path.splice(proxy_cache->list_path.begin(), proxy_cache->list_path, find(proxy_cache->list_path.begin(),proxy_cache->list_path.end(), path_name));
 return proxy_cache->document_map[path_name];
}
//To delete documents from cache when it is full or the entry is not current.
bool deleting_document(string path_name, cache *proxy_cache)
 {
  deleting_block(proxy_cache->document_map[path_name]);
  delete proxy_cache->document_map[path_name];
  proxy_cache->document_map.erase(path_name);
  proxy_cache->list_path.erase(find(proxy_cache->list_path.begin(),proxy_cache->list_path.end(), path_name));
  return true;
 }
 //Finding the client information among multiple clients when receivin a request from it.
 vector<information_client*>::iterator findingclient(int sock_fd)
{
 vector<information_client*>::iterator it;
 for (it=list_client.begin(); it!=list_client.end(); it++)
 {
  if ((*it)->client_fd==sock_fd || (*it)->host_fd==sock_fd)
  return it;
 }
 return list_client.end();
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
 //To close a connection to client/host on not receiving information when requested(idle/unresponsive).
 int connection_close(information_client* client, fd_set& master_fd)
 {
  if (client->client_fd!=-1)
  {
   close(client->client_fd);
   FD_CLR(client->client_fd, &master_fd);
   printf("closing client connected on socket: %d \n", client->client_fd);
  }
  if(client->host_fd!=-1)
   {
   close(client->host_fd);
   FD_CLR(client->host_fd, &master_fd);
   printf("closing host associated with client connected on socket: %d \n", client->host_fd);
  }
  delete client;
  return 0;
 }
 //To find the hostname and determine the path of the document.
  int path_info(const char* buffer, string& path, char* hostname)
 {
  char buf_response[1024];
  strcpy(buf_response, buffer);
  char* host_start= strstr(buf_response, "Host: ");
  host_start = host_start + strlen("Host: ");
  char* host_end= strchr(host_start, ' ');
  
  if(!host_end)
   {
   host_end= strchr(host_start, '\r');
   }
   
   strncpy(hostname, host_start, host_end-host_start);
   hostname[host_end-host_start]='\0';
   
   path.append(hostname);
   char* ptr_path;
   ptr_path= strtok(buf_response, " \r\n");
   
   if (ptr_path!=NULL)
   {
    ptr_path=strtok(NULL," \r\n" );
    path.append(ptr_path);
   }
   return 0;
 }
 //Establishing a connection to the host.
 int connectingtohost(char* host)
 {
  char http_host[INET6_ADDRSTRLEN];
  int sockfd, rv;
  struct addrinfo hints, *servinfo, *p;
  
  memset(&hints, 0, sizeof hints);
  hints.ai_family=AF_UNSPEC;
  hints.ai_socktype=SOCK_STREAM;
  
  if ((rv=getaddrinfo(host,"80",&hints,&servinfo))!=0)
  {
   fprintf(stderr, "getaddrinfo: %s \n", gai_strerror(rv));
   return 1;
  }
  
  for(p=servinfo;p!=NULL;p=p->ai_next)
  {
   if ((sockfd=socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1)
   {
    perror("socket error");
    continue;
   }
   if(connect(sockfd,p->ai_addr, p->ai_addrlen)==-1)
   {
    close(sockfd);
    perror("connect error");
    continue;
   }
   break;
  }
  if (p==NULL)
  {
   fprintf (stderr, "connection to host failed\n");
   return -1;
  }
  inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), http_host, sizeof http_host);
  printf("connected to %s \n", http_host);
  freeaddrinfo(servinfo);
  return sockfd;
 }
 cache *proxy_cache= new cache;
 //Updating the cache with the requested document and related info based on request type(200 or 304)
 void cache_update(information_client* client, char* buffer, int numbytes)
 {
  document* document_pointer=getting_document(client->path, proxy_cache);
  if(!document_pointer && (strstr(buffer,"HTTP/1.0 200 OK") || strstr(buffer,"HTTP/1.1 200 OK")))
  {
   document_pointer=new document;
   memset(document_pointer->expires,0,255);
   memset(document_pointer->etag,0,255);
   document_pointer->last_modified=0;
   document_pointer->expiry_time=0;
   document_pointer->check=false;
   document_pointer->path_name=client->path;
   
   cout<<"document cached:" << client->path <<endl;
   retrieve_time_info(buffer,numbytes,document_pointer);
   addingdocument (document_pointer, proxy_cache);
  }// checking for Conditional GET request situation
  if(document_pointer->check)
  {
   //If the buffer contains header information extract the information.
   if(strstr(buffer, "HTTP/1.0 304 Not Modified") || strstr(buffer, "HTTP/1.1 304 Not Modified"))
   {
    cout<< "HTTP/1.0 304 Not Modified" <<endl;
    retrieve_time_info(buffer, numbytes, document_pointer);
    return;
   }
   
   else if (strstr(buffer, "HTTP/1.0 200 OK") || strstr(buffer, "HTTP/1.1 200 OK"))
   {
    cout<< "HTTP/1.0 200 OK" <<endl;
    retrieve_time_info(buffer, numbytes, document_pointer);
    deleting_block(document_pointer);
   }
   document_pointer->check=false;
  }
 
 block_data* blkptr = new block_data;
 memcpy(blkptr->data,buffer, numbytes);
 blkptr->data_size=numbytes;
 document_pointer->block.push_back(blkptr);
 }
//To check for the If-Modified-Since field to check if conditional GET is required or not.
bool cache_check(document* document_pointer, char* buffer, int& numbytes)
{
 char* ending= strstr(buffer, "\r\n\r\n");
 if(!ending || document_pointer->expiry_time|| !strlen(document_pointer->etag))
 {
  return false;
 }
 else
 {
 ending = ending+2;
 numbytes= numbytes+ sprintf(ending, "%s%s\r\n%s%s\r\n\r\n", "If-Modified-Since", document_pointer->expires, "If-None-Match", document_pointer->etag);
 info_extract("If-Modified-Since:", buffer, document_pointer->expires);
 cout<< "If-Modified-Since:" << document_pointer->expires <<endl;
 cout<< "If-None-Match:"<< document_pointer->etag<<endl;
 numbytes=numbytes-2;
 return true;
 }
}
 
//vector<information_client*> list_client;
//cache *proxy_cache= new cache;

 int main(int argc, char *argv[])
 {
  struct addrinfo hints, *servinfo, *p;
  int fdmax;
  int listener; //listening for new connections
  fd_set master_fd;  //master set to store the file descriptors of multiple clients.
  fd_set read_fds;
  int rv;
  int optval=1;
  
  if (argc!=3)
  {
   printf("usage: /proxy <ip to bind> <port to bind> \n");
   return -1;
  }
  
  FD_ZERO(&master_fd);
  FD_ZERO(&read_fds);
  
  memset(&hints, 0, sizeof hints);
  hints.ai_family= AF_UNSPEC;
  hints.ai_socktype= SOCK_STREAM;
  hints.ai_flags= AI_PASSIVE;
  
  if ((rv=getaddrinfo(argv[1],argv[2],&hints,&servinfo))!=0)
  {
   fprintf(stderr, "proxyserver %s \n", gai_strerror(rv));
   exit(1);
  }
  
  for (p=servinfo; p!=NULL; p=p->ai_next)
  {
   
   listener= socket (p->ai_family, p->ai_socktype, p->ai_protocol);
   if (listener<0) continue;
   setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (int));
   
   if (bind(listener, p->ai_addr, p->ai_addrlen)<0)
   {
    close(listener);
    continue;
   }
   break;
  }
  
  if (p==NULL)
  {
   fprintf (stderr, "proxyserver: failed to bind \n");
   exit(2);
  }
  
  freeaddrinfo(servinfo);
  
  if (listen(listener, 10)==-1)
  {
   perror("listen");
   exit(3);
  }
  //adding the current file descriptor to master set
  FD_SET(listener, &master_fd);
  fdmax= listener;
  
  for(;;)
  {
   read_fds=master_fd;
   if(select(fdmax+1, &read_fds, NULL, NULL, NULL)==-1)
   {
    perror("select");
    exit(4);
   }
   //loop through all connections to check if we are receiving data
   for(int i=1; i<=fdmax; i++) 
   {
    if (FD_ISSET(i,&read_fds))
    {
     if(i==listener) //listening from new connection
     {
      struct sockaddr_storage client_addr;
      socklen_t address_length;
      address_length=sizeof client_addr;
      int ephemeral_fd;
      char client_ip[INET6_ADDRSTRLEN];
      
      if((ephemeral_fd=accept(listener,(struct sockaddr *)&client_addr, &address_length))==-1) //accept connection on the ephemeral port of proxy server
      {
       perror("accept");
      }
      else
      {
       FD_SET(ephemeral_fd,&master_fd);
       if (ephemeral_fd>fdmax)
       {
        fdmax=ephemeral_fd; //updating the maximum value of the file descriptor to new value.
       }
       information_client* client=new information_client;
       client->host_fd=-1;
       client->client_fd=ephemeral_fd;
       list_client.push_back(client);
       printf("address of client connected: %s/n", inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr),client_ip, INET6_ADDRSTRLEN));
      }
     }
     else //listening from existing client or web server
     {
      char buffer[10240];
      int numbytes;
      if((numbytes=recv(i, buffer, sizeof buffer, 0))<=0) //sending data to buffer from i
      {
       if(numbytes<0) 
       {
        perror("recv error");
       }
       vector<information_client*>::iterator it=findingclient(i);
       connection_close(*it, master_fd);
       list_client.erase(it);
      }
      else
      {
       information_client* client=*(findingclient(i));
       //received request from client
       if (i==client->client_fd) 
       {
        bool check= false, retrieve_new=true ;
        path_info(buffer, client->path, client->hostname);
        cout<<"hostname: "<<client->hostname<<endl;
        cout<<"path: "<<client->path<<endl;
        document* document_pointer = getting_document(client->path, proxy_cache);
        if(document_pointer)//this document present in cache
        {
         time_t current_t= Gettingtime();
         cout<<"Expiry_time: " <<document_pointer->expiry_time<<endl;
         cout<<"Current_t: " << current_t<<endl;
         if(document_pointer->expiry_time>current_t) //retieving an unexpired document from the cache
         {
          retrieve_new=false;
          cout<<" Document already present in cache and is current: "<< client->path<<endl;
          vector <block_data*>:: iterator it;
          for (it=document_pointer->block.begin();it!=document_pointer->block.end();it++)
          {
           int send_data=send(client->client_fd, (*it)->data, (*it)->data_size, 0);
           if (send_data==-1) perror ("sending error");
          }
          vector<information_client*>::iterator it_1= findingclient(i);
          connection_close(*it_1, master_fd);
          list_client.erase(it_1);
         }
         else
         {
          cout<<"Document present in cache but may have expired: "<<client->path<<endl;
          check= cache_check(document_pointer, buffer, numbytes);
          if(check) 
          {
          document_pointer->check=true;
          }
          else
          {
           cout<<"deleting document form cache:"<<client->path<<endl;
           deleting_document(client->path, proxy_cache);
          }
         
         }
        }
        if (retrieve_new) //document expired, hence requesting from server or document was not cahed, therefore requesting from server.
        {
         if(check)
         {
          cout<<"document expired for:" << client->path<<" sending get request again" <<endl;
         }
         else
         cout<<"document was not cached:"<<client->path<<" requesting from the server"<<endl;
        if(client->host_fd==-1)
        {
         cout<<"Hostname:"<<client->hostname<<endl;
         client->host_fd = (connectingtohost(client->hostname)); //connecting to the web server to retrieve information
         FD_SET(client->host_fd, &master_fd);
         if(client->host_fd>fdmax) fdmax=client->host_fd;
        }
        if(send(client->host_fd, buffer, numbytes, 0)==-1) perror("sending error");
        }
       }
       //received response from server
       else if (i==client->host_fd)
       {
        cache_update(client, buffer, numbytes);
        document* document_pointer= getting_document(client->path, proxy_cache);
        if(document_pointer && (strstr(buffer, "HTTP/1.0 304 Not Modified") || strstr(buffer, "HTTP/1.0 304 Not Modified")))
        {
         vector <block_data*>::iterator it;
         for(it=document_pointer->block.begin();it!=document_pointer->block.end();it++)
         {
          if (send(client->client_fd, (*it)->data,(*it)->data_size, 0) == -1) perror("sending error");
         }
          cout<< "requested data sent from cache: "<<client->path<<endl;
          vector <information_client*> ::iterator it_1=findingclient(i);
          connection_close(*it_1, master_fd);
          list_client.erase (it_1);
         }
         else if (send(client->client_fd, buffer, numbytes, 0)==-1)
         {
          perror("sending error");
         }
        }
       }
      }
     }
    }
   }
  return 0;
 }
 