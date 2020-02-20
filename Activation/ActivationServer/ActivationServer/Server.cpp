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

void activatenum(string serialnum, string machineid);

string checkserial(string serial_number, string machineid);

int main(int argc, char* argv[])
{
	ifstream fileInput;
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
	char		resultmessage[BUFFERSIZE];

	// If user types in a port number on the command line use it,
	// otherwise, use the default port number
	if (argc > 1)
		port = atoi(argv[1]);
	else
		port = DEFAULTPORT;


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
	

	cout << "\nWaiting for connections...\n" << endl;

	// Accept an incoming connection; Program pauses here until a connection arrives
	clientSocket = accept(listenSocket, NULL, NULL);
	if (clientSocket == INVALID_SOCKET)
	{
		cerr << "Accept failed with error: " << WSAGetLastError() << endl;
		// Need to close listenSocket; clientSocket never opened
		cleanup(listenSocket);
		return 1;
	}


	// Receive the serial number from the client
	iResult = recv(clientSocket, buffer, BUFFERSIZE - 1, 0);
	for (int i = 0; i < BUFFERSIZE; i++)
	{
		if (buffer[i] == '\0')
		{
			// Send a message back to the client that this is a valid serial number
			string message = "good";
			send(clientSocket, message.c_str(), sizeof(message.c_str()), 0);
			break;
		}
		if (!isdigit(buffer[i]))
		{
			// Send a message back to the client that this is an invalid serial number
			string message = "invalid";
			send(clientSocket, message.c_str(), sizeof(message.c_str()), 0);
		}
	}


	//turns the character array into a string, if this yells let me know.
	string serial(buffer);

	
	// Receive the machine id
	iResult = recv(clientSocket, buffer, BUFFERSIZE - 1, 0);
	string machine_id(buffer);


	// Send back a valid or invalid message for activation
	string response = checkserial(serial, machine_id);
	send(clientSocket,response.c_str(), sizeof(response.c_str()),0);
	
	cleanup(clientSocket);
	cleanup(listenSocket);

	return 0;
}


void cleanup(SOCKET socket)
{
	if (socket != INVALID_SOCKET)
		closesocket(socket);

	WSACleanup();
}

void activatenum(string serialnum, string machineid)
{
	ofstream fileserv;
	fileserv.open(DATAFILENAME, std::ofstream::app);
	// if there is an error here, separate the + \n's into separate writes.
	fileserv.write(serialnum.c_str(), sizeof(serialnum.c_str()));
	fileserv << endl;
	fileserv.write(machineid.c_str(), sizeof(machineid.c_str()));
	fileserv << endl;
	fileserv.close();
}

string checkserial(const string serial_number, string machine_id)
{
	ifstream fileserv(DATAFILENAME);
	if(fileserv.is_open())
	{
		string flin;
		do
		{
			// finds serial number in the database
			if( flin == serial_number)
			{
				// it was found, now lets see if its valid.
				if(getline(fileserv,flin))
				{
					//is valid.
					if(machine_id == flin)
					{
						string hold = "good";
						return hold;
					}
					string hold2 = "invalid";
					return hold2;
				}
			}
		} while(getline(fileserv,flin));
		fileserv.close();
		// serial number wasn't found so call activate number
		activatenum(serial_number, machine_id);
		string hold3 = "good";
		return hold3;
	}
	return "invalid";
}
