#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <iostream>
#include <string>
using namespace std;

// Make sure build environment links to Winsock library file
#pragma comment(lib, "Ws2_32.lib")

// Define default global constants
#define BUFFERSIZE 256
#define DEFAULTPORT 5000

// Function to close the specified socket and 
// perform DLL cleanup (WSACleanup)
void cleanup(SOCKET socket);

int main()
{
	WSADATA		wsaData;			// structure to hold info about Windows sockets implementation
	SOCKET		listenSocket;		// socket for listening for incoming connections
	SOCKET		clientSocket;		// socket for communication with the client
	SOCKADDR_IN	serverAddr;			// structure to hold server address information
	string		ipAddress;			// server's IP address
	string		input;				// generic input from user
	char		buffer[BUFFERSIZE];	// buffer to hold message received from server
	int			port;				// server's port number
	int			iResult;			// resulting code from socket functions
	bool		done;				// variable to control communication loop
	bool		moreData;			// variable to control receive data loop


	cout << "\n*** CHAT SERVER ***\n\n";

	// Get server address information from user
	cout << "Enter IP address (press enter for localhost): ";
	getline(cin, ipAddress);
	if (ipAddress == "")
		ipAddress = "127.0.0.1";

	cout << endl;
	cout << "Enter port number is " << DEFAULTPORT << endl;
	cout << "Press enter to accept port, or enter new port: ";
	getline(cin, input);
	if (input == "")
		port = DEFAULTPORT;
	else
		port = stoi(input);


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
	inet_pton(AF_INET, ipAddress.c_str(), &serverAddr.sin_addr);


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

	// For this program ONLY, once an incoming connection has been accepted,
	// the listen socket is no longer needed so it will be closed
	closesocket(listenSocket);



	cout << "Connected...\n\n";

	// Always make sure there's a '\0' at the end of the buffer
	buffer[BUFFERSIZE - 1] = '\0';

	// Loop to communicate with client until either side types "end"
	done = false;
	do
	{
		// RECV and then SEND

		// Wait to receive a message from the client
		cout << "\n\t--WAIT--\n\n";

		// String to hold received message
		input = "";

		// Receive until client stops sending data
		moreData = false;
		do
		{
			iResult = recv(clientSocket, buffer, BUFFERSIZE - 1, 0);

			if (iResult > 0)
			{
				// Received data; need to determine if there's more coming				
				if (buffer[iResult - 1] != '\0')
					moreData = true;
				else
					moreData = false;

				// Concatenate received data onto end of string we're building
				input = input + (string)buffer;
			}
			else if (iResult == 0)
			{
				cout << "Connection closed\n";
				// Need to close clientSocket; listenSocket was already closed
				cleanup(clientSocket);
				return 0;
			}
			else
			{
				cerr << "Recv failed with error: " << WSAGetLastError() << endl;
				cleanup(clientSocket);
				return 1;
			}

		} while (moreData);

		cout << "Recv > " << input << endl;


		// Communication ends if client sent an "end" message
		if (input == "end")
			done = true;


		if (!done)
		{
			// Get a message from the user and attempt to send it to the client
			cout << "Send > ";
			getline(cin, input);

			iResult = send(clientSocket, input.c_str(), (int)input.size() + 1, 0);
			if (iResult == SOCKET_ERROR)
			{
				cerr << "Send failed with error: " << WSAGetLastError() << endl;
				cleanup(clientSocket);
				return 1;
			}

			// Communication ends if server sent an "end" message
			if (input == "end")
				done = true;
		}

	} while (!done);

	// Remember, need to close clientSocket before exiting, listenSocket was previously closed
	cleanup(clientSocket);
	return 0;
}


void cleanup(SOCKET socket)
{
	if (socket != INVALID_SOCKET)
		closesocket(socket);

	WSACleanup();
}