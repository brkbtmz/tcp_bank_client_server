#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <winsock2.h>
#include <mutex>
#include <thread>

#pragma comment (lib, "Ws2_32.lib")

#define port 20000
#define deductfee 5
#define maxclient 10

struct User {
    std::string username;
    std::string password;
    std::string name;
    std::string surname;
    std::string bank;
    double balance = 0;
};

std::unordered_map<std::string, User> users;
std::unordered_set<std::string> user_log;

std::mutex usersMutex, logMutex;


// These(opening the server socket which will listen incoming connections) are done here just to be able to close socket in another thread to cancel blocking accept.
// Another method was to use non-blocking accept and skip the error code. I preferred the first method to close listening socket.

// Initialize winsock 
WSADATA wsaData;
int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

SOCKET serversoc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

void exit_task()
{
    std::string exit;
    while (1)
    {
        std::getline(std::cin, exit);
        if (exit == "EXIT")
        {
            closesocket(serversoc);
            break;
        }
    }   
}

bool login(std::string username, std::string password)
{   
    if (users.count(username) == 0) 
    {
        // Not a user
        return false;
    }
    if (users[username].password == password)
    {
        logMutex.lock();
        user_log.insert(username);
        logMutex.unlock();
        
        return true;
    }
    return false;
}

bool islogged(std::string username)
{
    if (user_log.count(username)==0)
    {
        return false;
    }
    return true;
}

bool logout(std::string username)
{
    if (islogged(username))
    {
        logMutex.lock();
        user_log.erase(username);
        logMutex.unlock();
        return true;
    }
    return false;
}

void getusers()
{
    //Load user contents into map
   //Open the file
    std::ifstream file("users.txt");
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << std::endl;
        exit(1);
    }

    std::string line;
    //Read each line ended by \n
    while (std::getline(file, line)) {
        //Parse the line into the user struct and save it to users map (username is the key)
        std::istringstream iss(line);
        User user;
        iss >> user.username >> user.password >> user.name >> user.surname >> user.bank >> user.balance;
        users[user.username] = user;
    }
    file.close();
}

void updatecontent()
{
    std::ofstream file("users.txt");
    for (auto& k : users)
    {
        auto& user = k.second;
        file << user.username << " " << user.password << " " << user.name << " " << user.surname << " " << user.bank << " " << user.balance << std::endl;
    }
    file.close();
}

void client_task(SOCKET cli_sock)
{   
    // Receive packets, perform operation, update content.
    std::stringstream resp;
    std::string msg, operation, id, pass, name, surname, tid, tname, tsurname;
    double amount = 0;

    while (1)
    {
        char pkg[1500];
        
        // Read the packet and parse it
        int bytecount = recv(cli_sock, (char*)&pkg, sizeof(pkg), 0);

        if (bytecount == -1) {
            std::cerr << "Receive Error" << std::endl;
            break;
        }

        std::istringstream iss(pkg);
        iss >> operation;   

        if (operation == "LOGIN")
        {
            iss >> id >> pass;
            if (islogged(id))
            {
                std::cout << "Login attempt failed. Already Logged." << std::endl;
                msg = "FAIL";
                send(cli_sock, msg.c_str(), msg.length() + 1, 0);
            }
            else
            {
                if (login(id, pass))
                {
                    std::cout << "User ID: " << id << " logged in" << std::endl;
                    resp.str("");
                    resp.clear();
                    resp << "SUCCESS " << users[id].name << " " << users[id].surname << " " << users[id].balance;
                    msg = resp.str();
                    send(cli_sock, msg.c_str(), msg.length() + 1, 0);
                }
                else
                {
                    std::cout << "Login attempt failed" << std::endl;
                    msg = "FAIL";
                    send(cli_sock, msg.c_str(), msg.length() + 1, 0);
                }
            }
            
        }
        else if (operation == "DEPOSIT")
        {
            iss >> amount;
            std::cout << "User ID: " << id << " Deposited "<< amount << std::endl;
            usersMutex.lock();
            users[id].balance += amount;
            usersMutex.unlock();
            resp.str("");
            resp.clear();
            resp << "SUCCESS " << users[id].balance;
            msg = resp.str();
            send(cli_sock, msg.c_str(), msg.length() + 1, 0);
        }
        else if (operation == "WITHDRAW")
        {
            iss >> amount;
            if (users[id].balance >= amount)
            {
                std::cout << "User ID: " << id << " Withdrew " << amount << std::endl;
                usersMutex.lock();
                users[id].balance -= amount;
                usersMutex.unlock();
                resp.str("");
                resp.clear();
                resp << "SUCCESS " << users[id].balance;
                msg = resp.str();
                send(cli_sock, msg.c_str(), msg.length() + 1, 0);
            }
            else
            {
                msg = "FAIL";
                send(cli_sock, msg.c_str(), msg.length() + 1, 0);
            }
        }
        else if (operation == "TRANSFER")
        {
            iss >> tid >> tname >> tsurname >> amount;
            if (users[id].balance >= amount)
            {
                auto target = users.find(tid);
                if (target != users.end() &&
                    target->second.name == tname &&
                    target->second.surname == tsurname) {
                    std::cout << "User ID: " << id << " Transferred " << amount << "to " << tid  << std::endl;
                    usersMutex.lock();
                    users[id].balance -= amount;
                    target->second.balance += amount;
                    usersMutex.unlock();
                    
                    if (users[id].bank != target->second.bank) {
                        usersMutex.lock();
                        target->second.balance -= deductfee;
                        usersMutex.unlock();
                    }
                    resp.str("");
                    resp.clear();
                    resp << "SUCCESS " << users[id].balance;
                    msg = resp.str();
                    send(cli_sock, msg.c_str(), msg.length() + 1, 0);
                }
                else {
                    msg = "FAIL";
                    send(cli_sock, msg.c_str(), msg.length() + 1, 0);
                }
            }
            else
            {
                msg = "FAIL";
                send(cli_sock, msg.c_str(), msg.length() + 1, 0);
            }

        }
        else if (operation == "LOGOUT")
        {   // Call logout for the user, print log info to screen and feed back the client
            if (logout(id))
            {   
                std::cout << "User ID: " << id << " logged out" << std::endl;
                msg = "SUCCESS";
                send(cli_sock, msg.c_str(), msg.length() + 1, 0);
                break;
            }
            else
            {
                std::cout << "Failed log out attempt. " << id << " is not logged in" << std::endl;
                msg = "FAIL";
                send(cli_sock, msg.c_str(), msg.length() + 1, 0);
            }
        }
    }

    closesocket(cli_sock);
}

int main()
{   // Load user Contents First
    getusers();

    // Initialize winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed with error: " << result << std::endl;
        exit(1);
    }

    // Open Server Socket 
    //SOCKET serversoc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serversoc < 0)
    {
        std::cerr << "Socket Error" << std::endl;
        exit(1);
    }

    //Bind socket   
    sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);
  
    if (bind(serversoc, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0)
    {
        std::cerr << "Bind Error" << std::endl;
        exit(1);
    }

    // Start Listening Client Connections
    if (listen(serversoc, SOMAXCONN) < 0)
    {
        std::cerr << "Failed to listen" << std::endl;
        exit(1);
    }

    // Server socket opened, binded and started to listen
    std::cout << "Server is listening on port 20000" << std::endl;

    // Store the created threads for the client
    std::vector<std::thread> client_threads;

    sockaddr_in client;
    int client_size = sizeof(client);
    SOCKET client_socket;

    std::thread t_exit(exit_task);

    while (client_socket = accept(serversoc, (sockaddr*)&client, &client_size))
    {   
        
        // If the client socket is invalid, break out of the loop
        if (client_socket == INVALID_SOCKET)
        {
            std::cerr << "Accept Error" << std::endl;
            break;
        }

        // Create a thread for the client and store it in the vector
        client_threads.push_back(std::thread(client_task, client_socket));

        // If the number of client threads exceeds maxclient, wait for one to finish
        if (client_threads.size() > maxclient)
        {
            std::cout << "Max number of clients are connected" << std::endl;
            // Wait the first element of vector (task1) to end
            client_threads.front().join();
            client_threads.erase(client_threads.begin());
        }
    }

    t_exit.join();

    // Wait for all client threads to finish
    for (auto& threads : client_threads)
    {
        threads.join();
    }

    // Write updated content to the .txt file
    updatecontent();
    std::cout << "Contents updated" << std::endl;
    // Close the server socket
  //  closesocket(serversoc);
    std::cout << "Server closed" << std::endl;
   
    WSACleanup();

    return 0;
}