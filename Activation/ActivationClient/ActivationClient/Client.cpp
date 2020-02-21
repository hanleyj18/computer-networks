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
#define ACTIVATIONFILENAME "actFile.txt"
#define IPADDRESS "127.0.0.1"
#define GOODMSG "good"
#define BADMSG "invalid"
#define DEFAULTPORT 6000


// Function to close the specified socket and perform DLL cleanup (WSACleanup)
void cleanup(SOCKET socket);
// Function to check if a file with the name filename exists. Returns true if it does
// and false if it doesn't.
bool fexists();
// Determines if the machine has already been activated by comparing the machine id to the
// contents of the activation file
bool determineactivated(const string machine_id);
// Writes the machine id to the activation file
void storemachineid(const string machine_id);


int main(int argc, char* argv[])
{
	WSADATA		wsaData;			// structure to hold info about Windows sockets implementation
	SOCKET		mySocket;			// socket for communication with the server
	SOCKADDR_IN	serverAddr;			// structure to hold server address information
	string		input;				// generic input from user
	string		machineId;			// machine id for the current machine
	string		serialNumber;		// Serial number for the current machine
	int			port;				// server's port number
	int			iResult;			// resulting code from socket functions
	char		buffer[BUFFERSIZE];	// buffer to hold message received from server

	// If user types in a port number on the command line use it,
	// otherwise, use the default port number
	if (argc > 1)
		port = atoi(argv[1]);
	else
		port = DEFAULTPORT; 

	cout << "Enter your machine id: " << endl;
	getline(cin, machineId);

	// If the file exists, check the contents to see if they match the machine id
	// If they do, this machine has been activated, otherwise, we need to activate
	if (fexists())
	{
		const bool is_activated = determineactivated(machineId);
		if (is_activated)
		{
			cout << "This machine has been activated" << endl;
			return 0;
		}
	}
	cout << "Need to activate" << endl;

	cout << "Enter your serial number: " << endl;
	getline(cin, serialNumber);

	
	// WSAStartup loads WS2_32.dll (Winsock version 2.2) used in network programming
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
	{
		cerr << "WSAStartup failed with error: " << iResult << endl;
		return 1;
	}


	// Create a new socket for communication with the server
	mySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (mySocket == INVALID_SOCKET)
	{
		cerr << "Socket failed with error: " << WSAGetLastError() << endl;
		cleanup(mySocket);
		return 1;
	}

	cout << "\nAttempting to connect...\n" << endl;


	// Setup a SOCKADDR_IN structure which will be used to hold address
	// and port information for the server. Notice that the port must be converted
	// from host byte order to network byte order.
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_pton(AF_INET, IPADDRESS, &serverAddr.sin_addr);


	// Try to connect to server
	iResult = connect(mySocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (iResult == SOCKET_ERROR)
	{
		cout << "Connect failed with error: " << std::to_string(WSAGetLastError()) << endl;
		cleanup(mySocket);
		return 1;
	}

	cout << "Connected...\n" << endl;

	// Send the serial number to the server
	iResult = send(mySocket, serialNumber.c_str(), sizeof(serialNumber.c_str()), 0);
	if (iResult == SOCKET_ERROR)
	{
		cerr << "Send failed with error: " << WSAGetLastError() << endl;
		cleanup(mySocket);
		return 1;
	}

	// Wait for a message saying if the serial number is valid or not
	iResult = recv(mySocket, buffer, BUFFERSIZE, 0);
	if (iResult == SOCKET_ERROR)
	{
		cerr << "Receive failed with error: " << WSAGetLastError() << endl;
		cleanup(mySocket);
		return 1;
	}
	// If the server returns "good" then we continue activating
	else if (strcmp(buffer, "good") == 0)
	{
		cout << "Serial Number: Good" << endl;
	}
	// If the server returns anything else then we are stopping activation
	else
	{
		cout << "Serial Number: Invalid" << endl;
		cleanup(mySocket);
		return 1;
	}


	// Send the machine id to the server next
	iResult = send(mySocket, machineId.c_str(), sizeof(machineId.c_str()), 0);
	if (iResult == SOCKET_ERROR)
	{
		cerr << "Send failed with error: " << WSAGetLastError() << endl;
		cleanup(mySocket);
		return 1;
	}


	// Wait for a message saying if the machine id is valid or not
	iResult = recv(mySocket, buffer, BUFFERSIZE, 0);
	if (iResult == SOCKET_ERROR)
	{
		cerr << "Receive failed with error: " << WSAGetLastError() << endl;
		cleanup(mySocket);
		return 1;
	}
	// If the server returns "good" then we continue activating
	else if (strcmp(buffer, "good") == 0)
	{
		cout << "Machine Id: Good" << endl;
		storemachineid(machineId);
		cout << "This machine is now activated" << endl;
	}
	// If the server returns anything else then we are stopping activation
	else
	{
		cout << "Machine Id: Invalid\nCouldn't activate this machine" << endl;
		cleanup(mySocket);
		return 1;
	}

	// Everything activated fine and we are finished.
	return 0;
}

void cleanup(SOCKET socket)
{
	if (socket != INVALID_SOCKET)
		closesocket(socket);

	WSACleanup();
}

bool fexists()
{
	ifstream fin;
	fin.open(ACTIVATIONFILENAME, ios::in);
	if (!fin.is_open()) {
		return false;
	}
	fin.close();
	return true;
}

bool determineactivated(const string machine_id)
{
	ifstream fin;
	fin.open(ACTIVATIONFILENAME, ios::in);
	string stored_machine_id;
	getline(fin, stored_machine_id);

	if (stored_machine_id == machine_id)
	{
		return true;
	}
	return false;
}

void storemachineid(const string machine_id)
{
	fstream act_file;
	act_file.open(ACTIVATIONFILENAME, ios::trunc | ios::out);
	if (act_file.is_open())
	{
		act_file << machine_id;
		act_file.close();
	}
	cout << "Couldn't open activation file" << endl;
}
