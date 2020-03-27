#include <inc/lib.h>
#include <lwip/sockets.h>
#include <lwip/inet.h>

#define BUFFSIZE 32
#define IPADDR "10.0.2.15"
#define PORT 10000

const char *msg = "Hello world!\n";

static void
die(char *m)
{
	cprintf("%s\n", m);
	exit();
}

void umain(int argc, char **argv)
{
	int sock;
	struct sockaddr_in echoserver;
	char buffer[BUFFSIZE];
	unsigned int echolen;
	int received = 0;

	cprintf("Connecting to:\n");
	cprintf("\tip address %s = %x\n", IPADDR, inet_addr(IPADDR));

	// Create the TCP socket
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		die("Failed to create socket");

	cprintf("opened socket\n");

	// Construct the server sockaddr_in structure
	memset(&echoserver, 0, sizeof(echoserver));       // Clear struct
	echoserver.sin_family = AF_INET;                  // Internet/IP
	echoserver.sin_addr.s_addr = inet_addr(IPADDR);   // IP address
	echoserver.sin_port = htons(PORT);		  // server port

	cprintf("trying to connect to server\n");

	// Establish connection
	if (connect(sock, (struct sockaddr *) &echoserver, sizeof(echoserver)) < 0)
		die("Failed to connect with server");

	cprintf("connected to server\n");

	// Send the word to the server
	echolen = strlen(msg);
	if (write(sock, msg, echolen) != echolen)
		die("Mismatch in number of sent bytes");

	// Receive the word back from the server
	cprintf("Received: \n");
	while (received < echolen) {
		int bytes = 0;
		if ((bytes = read(sock, buffer, BUFFSIZE-1)) < 1) {
			die("Failed to receive bytes from server");
		}
		received += bytes;
		buffer[bytes] = '\0';        // Assure null terminated string
		cprintf(buffer);
	}
	cprintf("\n");

	close(sock);
}
