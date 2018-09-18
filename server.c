#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>

#define MAX_STRING 100			// Max input size

typedef struct log				// A linked list of data
{
	char IP[40];
	char event[40];
	double volts;
	double ts;					// The timestamp
	struct log *nextLog;
} Log;

void error(const char *message);// Prints an error message and then exits the program

void addToLog(int reports, Log *root);
void freeLog(Log *root);
void displayLog(Log *root);
void instructRTU(void *ptr);

void insert(Log **root, Log *newLog);
void sort(Log **root);

int sock;
struct sockaddr_in server, addr;
socklen_t fromlen;
char buffer[MAX_STRING];

int main(int argc, char **argv) {
	// Make sure to include the port via command line arguments
	if (argc != 2) {
		error(
				"argc in main() in pi.c is not equal to 2\nCorrect Usage: ./exe port#");
	}

	int functionReturn;	// Used for sendto() and recvfrom()

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		error("socket() in main() in pi.c");
	}

	int isBroadcast = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &isBroadcast, sizeof(isBroadcast))
			< 0) {
		puts("error setting scok et options");
		exit(-1);
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[1]));
	addr.sin_addr.s_addr = inet_addr("128.206.19.255");

	fromlen = sizeof(struct sockaddr_in);

	strcpy(buffer, "Setting things up...");

	functionReturn = sendto(sock, buffer, sizeof(buffer), 0,
			(struct sockaddr *) &addr, fromlen);
	if (functionReturn < 0) {
		puts("\nMESSGE NTO SENT");
	}

	FILE * fp;
	int records = 1;
	Log * root = NULL;
	Log * prevCmd = NULL;
	Log * cmd;

	root = malloc(sizeof(Log));
	//root->IP = 0;

	strcpy(root->event, "STRT");

	fp = fopen("events.txt", "w+");

	if (fp == NULL) {
		puts("File couldn't be opened");
		return 1;
	}
	fclose(fp);

//	displayLog( root );

	//Start PThread for Sending
	pthread_t commandThread;
	pthread_create(&commandThread, NULL, (void *) instructRTU, (void*) root);

	while (1) {

		bzero(buffer, MAX_STRING);

		functionReturn = recvfrom(sock, buffer, MAX_STRING, 0,
				(struct sockaddr *) &server, &fromlen);
		if (functionReturn < 0) {
			error("Receive from");
		}

		int action = 0;
		/*
		 Compare the command with different options and set the action to a value, if it has one. otherwise action will remain 0, which will cause no action to happen.
		 */
		printf("\nReceived :: %s\n", buffer);

		fflush (stdout);

		if (buffer[0] == '#') {
		}

		else if (buffer[0] == '$') {
			//puts( "Final Project" );

			double volts;
			double ts;
			char command[40];
			char IP[40];

			sscanf(buffer, "$ %s %s %lf %lf", IP, command, &ts, &volts);

			//printf( "\nCommand: %s\nVoltage: %lf\nTimestamp: %lf\functionReturn", command, volts, ts );

			//Log * root = NULL;
			//Log * prevCmd = NULL;
			//Log * cmd;

			//puts( "\nAdding new value to list" );

			//Add Element sent by server to list
			records++;
			Log * newCmd = malloc(sizeof(Log));

			newCmd->nextLog = NULL;
			//cmd->event = command;
			strcpy(newCmd->IP, IP);
			strcpy(newCmd->event, command);
			newCmd->volts = volts;
			newCmd->ts = ts;

			cmd = root;
			//puts( "Walking through list" );
			while (cmd != NULL) {
				prevCmd = cmd;
				cmd = cmd->nextLog;
			}

//			cmd = newCmd
			prevCmd->nextLog = newCmd;

			//puts("\functionReturn\nTHESE VALUES EXIST: " );

//			displayLog( root );                       
			sort(&root);
			addToLog(records, root);
//			freeLog( root );

		}

		//printf( "Log recieved from client: %s", buffer );

		//Do comparisons here to decide what operation will be executed when each command is received.	

	}

	freeLog(root);

	return EXIT_SUCCESS;

}

void error(const char *message) {
	printf("\n");
	printf(message);
	printf("\n");
	exit (EXIT_FAILURE);
}

void sort(Log ** head_ref) {
	Log * sorted = NULL;
	Log *current = *head_ref;

	while (current != NULL) {
		Log * nextLog = current->nextLog;
		insert(&sorted, current);
		current = nextLog;
	}

	*head_ref = sorted;
}

void insert(Log** head_ref, Log* new_node) {
	Log * current;
	if (*head_ref == NULL || (*head_ref)->ts >= new_node->ts) {
		new_node->nextLog = *head_ref;
		*head_ref = new_node;
	} else {
		current = *head_ref;
		while (current->nextLog != NULL && current->nextLog->ts < new_node->ts) {
			current = current->nextLog;
		}
		new_node->nextLog = current->nextLog;
		current->nextLog = new_node;
	}
}

void instructRTU(void * ptr) {
	int functionReturn;

	Log * root = (Log*) ptr;

	while (1) {

		char inBuffer[MAX_STRING];
		puts("\nInput your message: ");
		//getline( inBuffer );
		//scanf( "%s", inBuffer );
		fgets(inBuffer, MAX_STRING - 1, stdin);
		//printf( "Sending %s", inBuffer );

		if (!strcmp(inBuffer, "VIEW\n")) {
			displayLog(root);
		} else {
			functionReturn = sendto(sock, inBuffer, sizeof(buffer), 0,
					(struct sockaddr *) &addr, fromlen);
		}

	}

	return;
}

void displayLog(Log * root) {

	if (root == NULL) {
		return;
	}

	displayLog(root->nextLog);
	//usleep( 50000 );

	printf("\n> %4s %13s %17.6lf %.6lf", root->event, root->IP,
			root->ts, root->volts);
}

void freeLog(Log * root) {

	if (root == NULL) {
		return;
	}

	freeLog(root->nextLog);
	//usleep( 50000 );

	printf("\nFreeing: %s", root->event);
	free(root);

}

void addToLog(int records, Log * root) {

	//puts( "Writing list back to file" );

	FILE * fp;
	if (access("events.txt", F_OK) != -1) {
		fp = fopen("events.txt", "w");
	} else {
		fp = fopen("events.txt", "w");
	}
	Log * cmd = root;
	if (fp != NULL) {
		fprintf(fp, "%d\n", records);
		while (cmd != NULL) {
			fprintf(fp, "$ %s %s %lf %lf\n", cmd->IP, cmd->event,
					cmd->volts, cmd->ts);
			cmd = cmd->nextLog;
		}
	}
	fclose(fp);

}
