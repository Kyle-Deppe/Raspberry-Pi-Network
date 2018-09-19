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
#include <time.h>

#define MSG_SIZE 40			// message size

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
	// 3 Arguments
   if (argc != 3)
   {
      printf("usage: ./Lab5 port name\n");
      exit(0);
   }
 
	// For rand() to work randomly
   srand(time(NULL));

   int sock, length, n;
   socklen_t fromlen;
   struct sockaddr_in server;
   struct sockaddr_in addr;
   char buffer[MSG_SIZE];	// to store received messages or messages to be sent.
   bool isLocalMaster = 0;	// Does this require <stdbool.h>?
   int voteNumber = 0;
   


   // Sockets are like files that let you communicate between computers
   sock = socket(AF_INET, SOCK_DGRAM, 0); // Creates socket. Connectionless.
   if (sock < 0)
   {
	   error("Opening socket");
   }

   // Sets up the server
   length = sizeof(server);			// length of structure
   bzero(&server,length);			// sets all values to zero. memset() could be used
   server.sin_family = AF_INET;		// symbol constant for Internet domain
   server.sin_addr.s_addr = INADDR_ANY;		// IP address of the machine on which
											// the server is running
	// Sets the port to a number such as 2000 (host to network short integer)
   server.sin_port = htons(atoi(argv[1]));	// port number

   // binds the socket to the address of the host and the port number	// Connects hardware (server) to a file (socket)
   if (bind(sock, (struct sockaddr *)&server, length) < 0)
   {
       error("binding");
   }

   // Interface Request
   struct ifreq interfaceRequest;
   // AF_INET is AddressFamily_Internet
   interfaceRequest.ifr_addr.sa_family = AF_INET;
   // Names ifreq wlan0
   snprintf(interfaceRequest.ifr_name, IFNAMSIZ, "wlan0");
   // Gets socket information and puts it into interfaceRequest
   ioctl(sock, SIOCGIFADDR, &interfaceRequest);
   
   char *IP;
   // Internet_NetworkToASCII, 
   IP = inet_ntoa(((struct sockaddr_in *)&interfaceRequest.ifr_addr)->sin_addr);
   printf("\nLocal IP: %s", IP);
   
   int isBroadcast = 1;
   addr.sin_addr.s_addr = inet_addr("128.206.19.255");
   addr.sin_port = htons(atoi(argv[1]));
   
   // change socket permissions to allow broadcast
   if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &isBroadcast, sizeof(isBroadcast)) < 0)
   	{
   		printf("error setting socket options\n");
   		exit(-1);
   	}

   fromlen = sizeof(struct sockaddr_in);	// size of structure

   while (1)
   {
	   // bzero: to "clean up" the buffer. The messages aren't always the same length...
	   bzero(buffer,MSG_SIZE);		// sets all values to zero. memset() could be used

	   // receive from a client
	   printf("\nWaiting to receive a datagram...\n");
	   // Receives a message from the client
	   n = recvfrom(sock, buffer, MSG_SIZE, 0, (struct sockaddr *)&addr, &fromlen);
       	   if (n < 0)
	   {
    	   	printf("recvfrom"); 
	   }

       printf("Received a datagram. It says: %s", buffer);

       // To send a broadcast message, we need to change IP address to broadcast address
       // If we don't change it (with the following line of code), the message
       // would be transmitted to the address from which the message was received.
	   // You may need to change the address below (check ifconfig)
       // VOTE
	   if (0 == strcmp(buffer, "VOTE\n"))
	   {
		   // Random vote number between 1 and 10
		   voteNumber = (rand() % 10) + 1;
		   // This could be of size MSG_SIZE, but whatever
		   char voteString[18];
		   sprintf(voteString, "# %s %d", IP, voteNumber);
			
			addr.sin_addr.s_addr = inet_addr("128.206.19.255");		// broadcast address
			// Sends to file socket the message voteString of size MSG_SIZE and device addr with a device address data structure of size fromlen
			n = sendto(sock, voteString, MSG_SIZE, 0,
    		      (struct sockaddr *)&addr, fromlen);
			if (n  < 0)
			{
				error("sendto() in VOTE");
			}
			
			isLocalMaster = 1;
	   }
	   // WHOIS
	   else if (0 == strcmp(buffer, "WHOIS\n"))
	   {
		   // Only send a response if you are master
		   if (isLocalMaster)
		   {
			   char message[MSG_SIZE];
			   sprintf(message, "User %s at %s is master.", argv[2], IP);
			   
			   addr.sin_addr.s_addr = inet_addr("128.206.19.255");
			   n = sendto(sock, message, MSG_SIZE, 0, (struct sockaddr *)&addr, fromlen);
			   if (n < 0)
			   {
				   error("sendto() in WHOIS");
			   }
		   }
	   }
	   // # 128.206.19.14 5
	   else if ('#' == buffer[0])
	   {
		   unsigned int theirNumber = 0;
		   unsigned int theirIP = 0;
		   
		   unsigned int myLastAddress = 0;
		   sscanf(IP, "%*u.%*u.%*u.%u %*u", &myLastAddress);
		   sscanf(buffer, "# %*u.%*u.%*u.%u %u", &theirIP, &theirNumber);
		   
		   // Set to master
		   if (theirNumber < voteNumber)
		   {
				isLocalMaster = 1;

		   }
		   // Check if IP is greater or lower
		   else if (theirNumber == voteNumber)
		   {
			   if (theirIP > myLastAddress)
			   {
				isLocalMaster = 0;
			   }
			   else
			   {
				isLocalMaster = 1;
			   }
		   }
		   // Set to slave
		   else if (theirNumber > voteNumber)
		   {
			   isLocalMaster = 0;
		   }
	   }
   }

   return 0;
 }
