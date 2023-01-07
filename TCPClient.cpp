#include <iostream>
#include <sstream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

int main()
{
    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed with error: " << result << std::endl;
        exit(1);
    }

    // Create socket
    SOCKET clisock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clisock == -1) {
        std::cerr << "Error creating socket" << std::endl;
        exit(1);
    }

    // Connect to the server
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(20000);
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);
    if (connect(clisock, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) < 0)
    {
        std::cerr << "connect failed with error " << std::endl;
        closesocket(clisock);
        exit(1);
    }

    std::cout << "Connected to server." << std::endl;

    std::string userid, password;
    
    while(1)
    { 
        std::cout << "Enter username: ";
        std::getline(std::cin, userid);
        std::cout << "Enter password: ";
        std::getline(std::cin, password);

        // Send login request to server
        std::stringstream request;
        request << "LOGIN " << userid << " " << password;
        std::string sendmsg = request.str();
        if (send(clisock, sendmsg.c_str(), sendmsg.length()+1, 0) == -1)
        {
            std::cerr << "Send Error" << std::endl;
            exit(1);
        }

        // Receive response from server
        char buffer[1500];
        int bytesReceived = recv(clisock, buffer, 1500, 0);
        if (bytesReceived == -1) {
            std::cerr << "Receive Error" << std::endl;
            exit(1);
        }
        //buffer[bytesReceived] = '\0';
        std::string response(buffer);

        // Parse response
        std::stringstream responseStream(response);
        std::string status;
        responseStream >> status;
        if (status == "SUCCESS") {
            std::string name, surname;
            double balance;
           
            responseStream >> name >> surname >> balance;
            std::cout << "Logged in successfully!" << std::endl;
            std::cout << "Welcome " << name << " " << surname << std::endl;
            std::cout << "Your Balance: " << balance << std::endl << std::endl;
            break;
        }
        else {
            std::cout << "Invalid username or password" << std::endl;
        }
    }
    std::string opstr, command;
    std::stringstream opss;
    
        std::cout << "Usage:" << std::endl << "DEPOSIT Amount" << std::endl;
        std::cout << "WITHDRAW Amount" << std::endl;
        std::cout << "TRANSFER RecepientID RecepientName RecepientSurname Amount" << std::endl;
        std::cout << "LOGOUT" << std::endl << std::endl;

    while (1)
    {
        std::cout << "Enter the operation :";
        std::getline(std::cin, opstr);
        opss.str("");
        opss.clear();
        opss << opstr;
        opss >> command;
        if (command == "WITHDRAW" || command == "DEPOSIT")
        {
            send(clisock, opstr.c_str(), opstr.length() + 1, 0);
        }
        else if (command == "TRANSFER")
        {
            send(clisock, opstr.c_str(), opstr.length() + 1, 0);
        }
        else if (command == "LOGOUT")
        {
            send(clisock, command.c_str(), command.length() + 1, 0);
        }
        else
        {
            std::cerr << "Invalid Operation" << std::endl;
            exit(1);
        }

        char buf[1500];
        int rcv = recv(clisock, buf, 1500, 0);
        if (rcv == -1) {
            std::cerr << "Receive Error" << std::endl;
            exit(1);
        }
        std::string resp(buf);
        double bal;
        std::stringstream respStream(resp);
        std::string stat;
        respStream >> stat;
        if (stat == "SUCCESS")
        {   
            if (command == "LOGOUT")
            {
                std::cout << "Logged Out" << std::endl;
                exit(0);
            }
            respStream >> bal;
            std::cout << "Operation Successful." << std::endl << "Your Balance: " << bal << std::endl << std::endl;
        }
        else
        {
            std::cout << "Operation Failed. Invalid receipent or Insufficient balance" << std::endl << std::endl;
        }
    }

    closesocket(clisock);
    return 0;
}
