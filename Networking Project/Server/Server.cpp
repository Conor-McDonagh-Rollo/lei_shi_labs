#define MY_PORT 56000
#define SPEED 10
//#define DEBUGGING

#include <WinSock2.h>
#include <SFML/Network.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>

struct Player {
    sf::Vector2f position {0,0};
    bool isIt = false;
};

struct GameState {
    Player players[3];
    bool gameRunning = true;
};

struct ClientInfo {
    int index;
    SOCKET socket;
    std::thread thread; // One thread per client

    // Move constructor
    ClientInfo(ClientInfo&& other)
        : index(other.index), socket(other.socket), thread(std::move(other.thread)) {
    }

// Other constructors for some reason (fuck you c++)
    ClientInfo(int idx, SOCKET sock)
        : index(idx), socket(sock) {
    }

    ~ClientInfo() {
        if (thread.joinable()) {
            thread.join();
        }
    }

    // Deleted copy operations
    ClientInfo(const ClientInfo&) = delete;
    ClientInfo& operator=(const ClientInfo&) = delete;

    // Default move assignment
    ClientInfo& operator=(ClientInfo&&) = default;
};

void handleClient(int clientIndex, SOCKET clientSocket, GameState& gameState, std::mutex& gameStateMutex) {
    while (gameState.gameRunning) {
        // Receive the new position
        char positionBuffer[256];
        int bytesReceived = recv(clientSocket, positionBuffer, 256, 0);
        if (bytesReceived > 0) {
            sf::Vector2f newPosition;
            memcpy(&newPosition, positionBuffer, sizeof(sf::Vector2f));

            // Lock the gameState before modifying
            std::lock_guard<std::mutex> lock(gameStateMutex);
            #ifdef DEBUGGING
            std::cout << "Broadcasting client index " << clientIndex << " which was " << 
            gameState.players[clientIndex].position.x << ", " << gameState.players[clientIndex].position.y
                        << " as the new position " << newPosition.x << ", " << newPosition.y << ".\n";
            #endif
            gameState.players[clientIndex].position = newPosition;
        }

        // Check if this player has tagged someone
        char tagBuffer[256];
        bytesReceived = recv(clientSocket, tagBuffer, 256, 0);
        if (bytesReceived > 0) {
            bool tagged;
            memcpy(&tagged, tagBuffer, sizeof(bool));

            std::lock_guard<std::mutex> lock(gameStateMutex);
            if (tagged) {
                gameState.players[clientIndex].isIt = true;
            } else {
                gameState.players[clientIndex].isIt = false;
            }
        }

        // Send updated game state to this player
        char gameStateBuffer[1024];
        {
            std::lock_guard<std::mutex> lock(gameStateMutex);
            memcpy(gameStateBuffer, &gameState, sizeof(GameState));
        }
        send(clientSocket, gameStateBuffer, sizeof(GameState), 0);
    }
}

int main()
{
    
    std::mutex gameStateMtx; // Mutex for thread safe access to game state
    /*
    -----------------------------------
    //  INITIALISE WINSOCK
    -----------------------------------
    */
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != NO_ERROR) {
        std::cerr << "Initialization failed." << std::endl;
        return 1;
    }
    std::cout << "Winsock initialized." << std::endl;

    /*
    -----------------------------------
    //  CREATE LISTENING SOCKET
    -----------------------------------
    */

    SOCKET listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listeningSocket == INVALID_SOCKET) {
        std::cerr << "Error at socket(): " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
    std::cout << "Listening socket created." << std::endl;

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(MY_PORT); // Use any port that is available
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the socket
    if (bind(listeningSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed with error: " << WSAGetLastError() << std::endl;
        closesocket(listeningSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Socket bound." << std::endl;

    // Listen on the socket
    if (listen(listeningSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed with error: " << WSAGetLastError() << std::endl;
        closesocket(listeningSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Listening for connections." << std::endl;

    /*
    -----------------------------------
    //  ACCEPT CONNECTIONS
    -----------------------------------
    */

    std::vector<ClientInfo> connectedClients; // Store client information
    GameState gameState;
    gameState.players[0].isIt = true;
    gameState.gameRunning = true;
    int nextClientIndex = 0; // Initialize the next available index

    for (int i = 0; i < 3; i++) {
        SOCKET clientSocket = accept(listeningSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
            closesocket(listeningSocket);
            WSACleanup();
            return 1;
        }

        connectedClients.emplace_back(nextClientIndex++, clientSocket);
        connectedClients.back().thread = std::thread(handleClient, connectedClients.back().index, connectedClients.back().socket, std::ref(gameState), std::ref(gameStateMtx));
        
        #ifdef DEBUGGING
        std::cout << connectedClients.at(i).socket << ", " << connectedClients.at(i).index << ", " << connectedClients.at(i).thread.get_id() << std::endl;
        #endif

        std::cout << "Accepted connection from client " << nextClientIndex << std::endl;
    
        // Send the assigned index to the client
        int clientIndex = connectedClients.back().index;
        send(clientSocket, reinterpret_cast<char*>(&clientIndex), sizeof(int), 0);
        
        #ifdef DEBUGGING
        std::cout << "sending " << clientIndex << "..." << std::endl;
        #endif
    }
    std::cout << "All clients connected!" << std::endl;

    /*
    -----------------------------------
    //  GAME LOGIC
    -----------------------------------
    */

    while (gameState.gameRunning) {
        std::cout << "type 'close' to end the server" << std::endl;
        std::string input = "";
        std::cin >> input;
        if (input == "close")
        {
            gameState.gameRunning = false;
        }
    }

    /*
    -----------------------------------
    //  CLOSE SOCKETS
    -----------------------------------
    */

    for (int i = 0; i < 3; ++i) {
        closesocket(connectedClients[i].socket);
    }
    for (int i = 0; i < 3; ++i) {
        if (connectedClients[i].thread.joinable()) {
            connectedClients[i].thread.join();
        }
    }
    std::cout << "Client sockets closed & threads joined." << std::endl;

    closesocket(listeningSocket);
    WSACleanup();
    std::cout << "Listening socket closed." << std::endl;
    system("pause");
    return 1;
}
