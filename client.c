#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h> 


#define SHARED_OBJECT_PATH         "/messenger"      
#define MAX_MESSAGE_LEN 50
#define MAX_MESSAGE_LENGTH      50
#define MESSAGE_TYPES               4
#define NULL_MESSAGE 0
#define GROUP_MESSAGE 2
#define SERVER_MESSAGE 3
#define RESPONSE_MESSAGE 4


#define EXIT_COMMAND "/exit"
#define EXIT_COMMAND_LEN 5


#define MAX_ID_LEN 10


#define DISCONNECT -1
#define CONNECTED 0
#define CONNECT 1
#define SERVER_FULL_MESSAGE "Serwer pelny!\n"


int Connected = 0;
char user_message[MAX_MESSAGE_LEN];
int Keep_Alive;
pthread_mutex_t* user_input_mutex;
int TYP_WIADOMOSCI;


typedef struct wiadomosc{
	int typ_wiadomosci;
	char id_nadawcy[MAX_ID_LEN];
	char id_odbiorcy[MAX_ID_LEN];
	char tresc[MAX_MESSAGE_LEN];
	int polaczenie;
	pthread_mutex_t* mutex_lock;
}msg_packet_t;


void open_connection(char uid[MAX_ID_LEN], msg_packet_t* shared_msg);
void close_connection(char uid[MAX_ID_LEN], msg_packet_t* shared_msg);
int send_message(msg_packet_t* shared_msg,char user_message[MAX_MESSAGE_LEN],char id_nadawcy[MAX_ID_LEN], int TYP_WIADOMOSCI);
void* read_user_input(void* args);
void clean_exit(int dum);

int main(int argc, char *argv[]) {
	char Uid[MAX_ID_LEN];
	int id=getpid();
	sprintf(Uid,"%d",id);
	Keep_Alive = 1;
	signal(SIGINT,clean_exit);
	int fd;
	int shared_seg_size = (sizeof(msg_packet_t));   /* SHM dla jednej wiadomosci */
	msg_packet_t* shared_msg; 


	/* shm_open() tylko do odczytu*/
	fd = shm_open(SHARED_OBJECT_PATH, O_RDWR, S_IRWXU | S_IRWXG);
	if (fd < 0) {
		perror("Serwer nie jest uruchomiony! / In shm_open()");
		exit(1);
	}
	printf("Opened shared memory object %s\n", SHARED_OBJECT_PATH);
	/* pobieranie SHM uzywajac mmap() */    
	shared_msg = (struct msg_packet_t*)mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (shared_msg == NULL) {
		perror("In mmap()");
		exit(1);
	}


	// Polaczenie z serwerem
	open_connection(Uid,shared_msg);
	printf("Twoje id: %d\n",id);
	pthread_t* user_input_thread;
	strcpy(user_message,"");
	//
	pthread_mutex_init(&user_input_mutex,NULL);
	int rc = pthread_create(&user_input_thread,NULL,read_user_input,(void*) NULL);
	TYP_WIADOMOSCI = GROUP_MESSAGE;

	while(Keep_Alive)
	{
		// Nasluchiwanie nadchodzacych wiadomosci
		pthread_mutex_lock(&shared_msg->mutex_lock);

		if(shared_msg->typ_wiadomosci == SERVER_MESSAGE)
		{
			int match;
			match = strcmp(shared_msg->id_odbiorcy,Uid);
			if(match == 0)
			{
				printf("%s: %s",shared_msg->id_nadawcy,shared_msg->tresc);
				shared_msg->typ_wiadomosci = RESPONSE_MESSAGE;
				if(strcmp(shared_msg->tresc,SERVER_FULL_MESSAGE)==0)
					Keep_Alive == 0;
			}
		}
		pthread_mutex_unlock(&shared_msg->mutex_lock);

		// Pobierz dane od uzytkownika i wyslij 
		pthread_mutex_lock(&user_input_mutex);
		if(strcmp(user_message,"") != 0 && strcmp(user_message,"\n") != 0)
		{
			if(send_message(shared_msg,user_message,Uid, TYP_WIADOMOSCI) == 1)
				strcpy(user_message,"");
		} 
		pthread_mutex_unlock(&user_input_mutex);
	}

	close_connection(Uid,shared_msg);


	return 0;
}


void open_connection(char Uid[MAX_ID_LEN], msg_packet_t* shared_msg)
{
	int is_connected;
	is_connected  = CONNECT;
	while(is_connected != CONNECTED && Keep_Alive)
	{
		pthread_mutex_lock(&shared_msg->mutex_lock);
		if(shared_msg->typ_wiadomosci != NULL_MESSAGE)
		{
			pthread_mutex_unlock(&shared_msg->mutex_lock);
			continue;
		}

		shared_msg->typ_wiadomosci = SERVER_MESSAGE;
		strcpy(shared_msg->id_nadawcy, Uid);
		strcpy(shared_msg->id_odbiorcy,"");
		shared_msg->polaczenie = CONNECT;
		is_connected = CONNECTED;

		pthread_mutex_unlock(&shared_msg->mutex_lock);
	}

}

void close_connection(char Uid[MAX_ID_LEN], msg_packet_t* shared_msg)
{
	unsigned int time_out;
	time_out = 2;
	int is_connected;
	is_connected = CONNECTED;
	while(is_connected == CONNECTED || time_out != 1)
	{
		time_out++;
		pthread_mutex_lock(&shared_msg->mutex_lock);
		if(shared_msg->typ_wiadomosci != NULL_MESSAGE)
		{
			pthread_mutex_unlock(&shared_msg->mutex_lock);
			continue;
		}

		shared_msg->typ_wiadomosci = SERVER_MESSAGE;
		strcpy(shared_msg->id_nadawcy, Uid);
		strcpy(shared_msg->id_odbiorcy,"");
		shared_msg->polaczenie = DISCONNECT;
		is_connected = DISCONNECT;

		pthread_mutex_unlock(&shared_msg->mutex_lock);

		if(!Keep_Alive)
			break;
	}
	if(time_out == 0)
		printf("Timeout");

}

int send_message(msg_packet_t* shared_msg,char user_message[MAX_MESSAGE_LEN],char id_nadawcy[MAX_ID_LEN], int TYP_WIADOMOSCI)
{
	int msg_type;
	msg_type  = SERVER_MESSAGE;
	while(Keep_Alive)
	{
		pthread_mutex_lock(&shared_msg->mutex_lock);
		if(shared_msg->typ_wiadomosci == NULL_MESSAGE)
		{

			strcpy(shared_msg->id_nadawcy,id_nadawcy);
			strncpy(shared_msg->tresc,user_message,MAX_MESSAGE_LEN);
			shared_msg->typ_wiadomosci = TYP_WIADOMOSCI;
			pthread_mutex_unlock(&shared_msg->mutex_lock);
			return 1;
		}
		if(shared_msg->typ_wiadomosci == SERVER_MESSAGE)
		{
			pthread_mutex_unlock(&shared_msg->mutex_lock);
			return 0;
		}
		pthread_mutex_unlock(&shared_msg->mutex_lock);
	}	

}

void* read_user_input(void* args)
{
	int user_message_set;
	char* rec;
	rec = (char*) malloc(sizeof(char)* MAX_ID_LEN);
	while(Keep_Alive)
	{
		char temp_message[MAX_MESSAGE_LEN];
		fgets(temp_message,MAX_MESSAGE_LEN,stdin);
		if(isspace(temp_message[0]) ==0){
		if(strncmp(temp_message,EXIT_COMMAND,EXIT_COMMAND_LEN) == 0)
			break;

			TYP_WIADOMOSCI = GROUP_MESSAGE;
		user_message_set = 1;
		while(user_message_set && Keep_Alive){
			pthread_mutex_lock(&user_input_mutex);
			if(strcmp(user_message,"") == 0)
			{
				strcpy(user_message,temp_message);
				user_message_set = 0;
			}
			pthread_mutex_unlock(&user_input_mutex);
		}
}
	}
	Keep_Alive = 0;
}

void clean_exit(int dum)
{
	Keep_Alive = 0;
}

