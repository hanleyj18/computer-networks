#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <iostream>
#include <string>
#include <fstream>
using namespace std;

// Make sure build environment links to Winsock library file
#pragma comment(lib, "Ws2_32.lib")


// Define default global constants
#define BUFFERSIZE 256
#define DATAFILENAME "dataFile.txt"
#define IPADDRESS "127.0.0.1"
#define GOODMSG "good"
#define BADMSG "invalid"
#define DEFAULTPORT 6000


// Function to close the specified socket and perform DLL cleanup (WSACleanup)
void cleanup(SOCKET socket);


int main(int argc, char* argv[])
{
	WSADATA		wsaData;			// structure to hold info about Windows sockets implementation
	SOCKET		listenSocket;		// socket for listening for incoming connections
	SOCKET		clientSocket;		// socket for communication with the client
	SOCKADDR_IN	serverAddr;			// structure to hold server address information
	string		input;				// generic input from user
	fstream		file;				// data file that holds list of activated clients
	errno_t		err;				// error reported from opening the data file
	char		buffer[BUFFERSIZE];	// buffer to hold message received from server
	int			port;				// server's port number
	int			iResult;			// resulting code from socket functions

	// If user types in a port number on the command line use it,
	// otherwise, use the default port number
	if (argc > 1)
		port = atoi(argv[1]);
	else
		port = DEFAULTPORT;

	// Attempts to open the data file with the activated clients or creates the list
	// if it doesn't exist.
	file.open(DATAFILENAME, ios::app | ios::in);


	// WSAStartup loads WS2_32.dll (Winsock version 2.2) used in network programming
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
	{
		cerr << "WSAStartup failed with error: " << iResult << endl;
		return 1;
	}


	// Create a new socket for communication with the client
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (listenSocket == INVALID_SOCKET)
	{
		cerr << "Socket failed with error: " << WSAGetLastError() << endl;
		cleanup(listenSocket);
		return 1;
	}


	// Setup a SOCKADDR_IN structure which will be used to hold address
	// and port information for the server. Notice that the port must be converted
	// from host byte order to network byte order.
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_pton(AF_INET, IPADDRESS, &serverAddr.sin_addr);


	// Attempt to bind a the local network address to the socket
	iResult = bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (iResult == SOCKET_ERROR)
	{
		cerr << "Bind failed with error: " << WSAGetLastError() << endl;
		cleanup(listenSocket);
		return 1;
	}


	// Start listening for incoming connections
	iResult = listen(listenSocket, 1);
	if (iResult == SOCKET_ERROR)
	{
		cerr << "Listen failed with error: " << WSAGetLastError() << endl;
		cleanup(listenSocket);
		return 1;
	}
	

	cout << "\nWaiting for connections...\n";

	// Accept an incoming connection; Program pauses here until a connection arrives
	clientSocket = accept(listenSocket, NULL, NULL);
	if (clientSocket == INVALID_SOCKET)
	{
		cerr << "Accept failed with error: " << WSAGetLastError() << endl;
		// Need to close listenSocket; clientSocket never opened
		cleanup(listenSocket);
		return 1;
	}

	getline(cin, input);
	
	return 0;
}


void cleanup(SOCKET socket)
{
	if (socket != INVALID_SOCKET)
		closesocket(socket);

	WSACleanup();
}


