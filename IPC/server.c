#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>


#define MAX_ID_LEN 10
#define MAX_MESSAGE_LEN 50
#define MAX_CLIENTS 10


#define NULL_MESSAGE 0
#define GROUP_MESSAGE 2
#define SERVER_MESSAGE 3
#define RESPONSE_MESSAGE 4


#define DISCONNECT -1
#define CONNECTED 0
#define CONNECT 1

#define SHARED_OBJECT_PATH         "/messenger"
#define MAX_MESSAGE_LENGTH      50
#define MESSAGE_TYPES               4

int NumClients;
char Clients[MAX_CLIENTS][MAX_ID_LEN];
int Keep_Alive;

typedef struct wiadomosc{
	int typ_wiadomosci;
	char id_nadawcy[MAX_ID_LEN];
	char id_odbiorcy[MAX_ID_LEN];
	char tresc[MAX_MESSAGE_LEN];
	int polaczenie;
	pthread_mutex_t* mutex_lock;
}msg_packet_t;


void send_message(msg_packet_t* shared_msg);
void wait_for_response(msg_packet_t* shared_msg);
void connect_client(msg_packet_t* shared_msg);
void disconnect_client(msg_packet_t* shared_msg);
void clean_exit(int dum);


int main(int argc, char *argv[]) {
	int fd;
	int shared_seg_size = (sizeof(msg_packet_t));   /* SHM dla jednej wiadomosci */
	msg_packet_t* shared_msg;      
	NumClients = 0; 
	Keep_Alive = 1;
	signal(SIGINT,clean_exit);

	/* Tworzenie SHM uzywajac shm_open(). Domyslnie /dev/shm. */
	fd = shm_open(SHARED_OBJECT_PATH, O_CREAT | O_EXCL | O_RDWR, S_IRWXU | S_IRWXG);
	if (fd < 0) {
		perror("Zostala juz uruchomiona poprzednia instancja! / In shm_open()");
		exit(1);
	}
	fprintf(stderr, "Created shared memory object %s\n", SHARED_OBJECT_PATH);

	/* Dopasuj rozmiar*/
	ftruncate(fd, shared_seg_size);

	/* SHM uzywajac mmap(). */    
	shared_msg = (msg_packet_t*)mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (shared_msg == NULL) {
		perror("In mmap()");
		exit(1);
	}
	fprintf(stderr, "Shared memory segment allocated correctly (%d bytes).\n", shared_seg_size);
	pthread_mutexattr_t mutex_attr;
	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_setpshared(&mutex_attr,PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&shared_msg->mutex_lock,&mutex_attr);
	pthread_mutex_lock(&shared_msg->mutex_lock);
	shared_msg->typ_wiadomosci = 0;
	pthread_mutex_unlock(&shared_msg->mutex_lock);
	fprintf(stderr, "Serwer uruchomil sie poprawnie.\n");
	while(Keep_Alive)
	{

		pthread_mutex_lock(&shared_msg->mutex_lock);
		// Nowy klient
		if(shared_msg->polaczenie == CONNECT)
		{
			connect_client(shared_msg);	
			pthread_mutex_unlock(&shared_msg->mutex_lock);
			continue;
		}

		if(shared_msg->polaczenie == DISCONNECT)
		{	
			disconnect_client(shared_msg);
			if(NumClients == 0)
			{
				fprintf(stderr, "Ostatni uzytkownik opuscil serwer.\nWylaczanie serwera.\n");
				pthread_mutex_unlock(&shared_msg->mutex_lock);
				break;
			}
		}
		// Klient wyslal wiadomosc
		if(shared_msg->typ_wiadomosci != 0)
		{

			if(shared_msg->typ_wiadomosci == GROUP_MESSAGE)
			{
				printf("%s: %s",shared_msg->id_nadawcy,shared_msg->tresc);
				pthread_mutex_unlock(&shared_msg->mutex_lock);
				send_message(shared_msg);
				pthread_mutex_lock(&shared_msg->mutex_lock);

			}
			if(shared_msg->typ_wiadomosci != SERVER_MESSAGE)
				shared_msg->typ_wiadomosci = NULL_MESSAGE;

		}

		pthread_mutex_unlock(&shared_msg->mutex_lock);		
	}

	// Sprzatanie
	if (shm_unlink(SHARED_OBJECT_PATH) != 0) {
		perror("In shm_unlink()");
		exit(1);
	}

	return 0;
}

void connect_client(msg_packet_t* shared_msg)
{
		char group_status_msg[MAX_MESSAGE_LEN];
		
		snprintf(group_status_msg,MAX_MESSAGE_LEN,"Witaj na czacie!\n");
		strcpy(Clients[NumClients++],shared_msg->id_nadawcy);


		strcpy(shared_msg->id_odbiorcy,shared_msg->id_nadawcy);
		strcpy(shared_msg->id_nadawcy,"Serwer");
		strcpy(shared_msg->tresc,group_status_msg);
		
		printf("%s dolaczyl do czatu!\n",Clients[NumClients-1]);

	shared_msg->polaczenie = CONNECTED;
	shared_msg->typ_wiadomosci = SERVER_MESSAGE;
}


void disconnect_client(msg_packet_t* shared_msg)
{
	int i;
	int client_found = -1;
	for(i=0; i<NumClients; i++)
	{
		if(strcmp(Clients[i],shared_msg->id_nadawcy) == 0)
		{
			strcpy(Clients[i],"     ");
			client_found = i;
			continue;
		}
		if(client_found > -1)
		{
			strcpy(Clients[i-1],Clients[i]);
		}
	}
	shared_msg->polaczenie = CONNECTED;
	shared_msg->typ_wiadomosci = 0;
	NumClients--;
	
}

void send_message(msg_packet_t* shared_msg)
{
	pthread_mutex_lock(&shared_msg->mutex_lock);
	pthread_mutex_unlock(&shared_msg->mutex_lock);
	int i;
	for(i=0; i < NumClients; i++)
	{	
		pthread_mutex_lock(&shared_msg->mutex_lock);
		if(strcmp(shared_msg->id_nadawcy,Clients[i]) == 0)
		{	
			pthread_mutex_unlock(&shared_msg->mutex_lock);
			continue;
		}
		shared_msg->typ_wiadomosci = SERVER_MESSAGE;
		strcpy(shared_msg->id_odbiorcy,Clients[i]);
		pthread_mutex_unlock(&shared_msg->mutex_lock);
		wait_for_response(shared_msg);
	}

}

void wait_for_response(msg_packet_t* shared_msg)
{
	while(Keep_Alive)
	{
		pthread_mutex_lock(&shared_msg->mutex_lock);
		if(shared_msg->typ_wiadomosci != SERVER_MESSAGE)
		{
			pthread_mutex_unlock(&shared_msg->mutex_lock);
			break;
		}
		pthread_mutex_unlock(&shared_msg->mutex_lock);
	}
	pthread_mutex_unlock(&shared_msg->mutex_lock);
}



void clean_exit(int dum)
{
	fprintf(stderr, "\nSerwer wylaczono poprawnie.\n");
	Keep_Alive = 0;
}
