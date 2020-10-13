#include "TCPServer.h" 
# define MAXTIMESTRYTORECEIVE 5

// char TCPServer::msg[MAXPACKETSIZE];

// int TCPServer::num_client;
// int TCPServer::last_client_num;
// int TCPServer::last_closed;
// bool TCPServer::isonline;
// vector<descript_socket*> TCPServer::Message;
// vector<descript_socket*> TCPServer::newsockfd;
std::mutex TCPServer::mt;

void sigpipe_handler(int signum){
	printf("There is a SIGPIPE error happened...exiting......(%d)\n", signum);
	// tcp.closed();
	exit(0);
}

char* TCPServer::receive_exact(int size)
{
	char* buffer = (char*) malloc(size);
	memset(&buffer[0], 0, sizeof(buffer));

  	string reply;
	if( recv(newsockfd[last_client_num]->socket , buffer , size, MSG_WAITALL) < 0)
  	{
	    	cout << "receive failed!" << endl;
		return nullptr;
  	}

  	return buffer;
}

string TCPServer::receive_name()
{
  	char buffer[SIZEOFPACKAGEFORNAME + 1];
	memset(&buffer[0], 0, sizeof(buffer));

	// printf("Trying to receive from last_client_num: %d\n", last_client_num);

  	string reply;
	if( recv(newsockfd[last_client_num]->socket , buffer , SIZEOFPACKAGEFORNAME, MSG_WAITALL) < 0)
  	{
	    	cout << "receive failed!" << endl;
		return nullptr;
  	}
	buffer[SIZEOFPACKAGEFORNAME]='\0';
  	reply = buffer;
  	return reply;
}

long TCPServer::receive_size_of_data()
{
  	char buffer[8];
	memset(&buffer[0], 0, sizeof(buffer));
	long size_of_data = 0;

	if( recv(newsockfd[last_client_num]->socket , buffer , 8, MSG_WAITALL) < 0)
  	{
	    	cout << "receive failed!" << endl;
		return -1;
  	}
	
	memcpy(&size_of_data, buffer, 8);

  	return size_of_data;
}

int TCPServer::setup(int port, vector<int> opts)
{
	int opt = 1;
	isonline = false;
	last_closed = -1;
	sockfd = socket(AF_INET,SOCK_STREAM,0);
 	memset(&serverAddress,0,sizeof(serverAddress));

	for(unsigned int i = 0; i < opts.size(); i++) {
		if( (setsockopt(sockfd, SOL_SOCKET, opts.size(), (char *)&opt, sizeof(opt))) < 0 ) {
			cerr << "Errore setsockopt" << endl; 
      			return -1;
	      	}
	}

	serverAddress.sin_family      = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port        = htons(port);

	if((::bind(sockfd,(struct sockaddr *)&serverAddress, sizeof(serverAddress))) < 0){
		cerr << "Errore bind" << endl;
		return -1;
	}
	
 	if(listen(sockfd,5) < 0){
		cerr << "Errore listen" << endl;
		return -1;
	}
	num_client = 0;
	isonline = true;
	return 0;
}

void TCPServer::accepted()
{
	socklen_t sosize    = sizeof(clientAddress);
	descript_socket *so = new descript_socket;
	so->socket          = accept(sockfd,(struct sockaddr*)&clientAddress,&sosize);
	so->id              = num_client;
	last_client_num = num_client;
	so->ip              = inet_ntoa(clientAddress.sin_addr);
	newsockfd.push_back( so );
	cerr << "accept client[ id:" << newsockfd[num_client]->id << 
	                      " ip:" << newsockfd[num_client]->ip << 
		              " handle:" << newsockfd[num_client]->socket << " ]" << endl;
	isonline=true;
	num_client++;
}

vector<descript_socket*> TCPServer::getMessage()
{
	std::lock_guard<std::mutex> guard(mt);
	return Message;
}

void TCPServer::Send(string msg, int id)
{
	send(newsockfd[id]->socket,msg.c_str(),msg.length(),0);
}

void TCPServer::Send(char* msg, int msg_len, int id)
{
	send(newsockfd[id]->socket,msg,msg_len,0);
}

// void TCPServer::send_viewer_msg(string msg)
// {
// 	send(newsockfd[last_client_num]->socket,msg,msg_len,0);
// }

void TCPServer::send_to_last_connected_client(void* data, int data_size)
{
	send(newsockfd[last_client_num]->socket,data,data_size,0);
}

int TCPServer::get_last_closed_sockets()
{
	return last_closed;
}

void TCPServer::clean(int id)
{
	// if(Message[id]->message){
	// 	free(Message[id]->message);
	// }
	Message[id]->message = NULL;
	memset(msg, 0, MAXPACKETSIZE);
	Message[id]->size_of_packet = 0;
}

string TCPServer::get_ip_addr(int id)
{
	return newsockfd[id]->ip;
}

bool TCPServer::is_online() 
{
	return isonline;
}

void TCPServer::detach(int id)
{
	close(newsockfd[id]->socket);
	newsockfd[id]->ip = "";
	newsockfd[id]->id = -1;
	// if(newsockfd[id]->message){
	// 	free(newsockfd[id]->message);
	// }
	newsockfd[id]->message = NULL;
	Message[id]->size_of_packet = 0;
} 

void TCPServer::closed() 
{
	printf("TCPServer is going to be closed...\n");
	close(sockfd);
}
