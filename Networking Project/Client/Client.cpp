#define  _WINSOCK_DEPRECATED_NO_WARNINGS
#define MY_PORT 56000
#define MY_IP "192.168.1.15"

// dont set too low so server isn't overwheled
#define SPEED 10
//#define DEBUGGING

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <WinSock2.h>
#include <thread>
#include <mutex>
#include <iostream>


struct Player {
    sf::Vector2f position {0,0};
    bool isIt = false;
};

struct GameState {
    Player players[3];
    bool gameRunning = true;
};

void move(Player& player, const sf::Vector2f& direction) {
    player.position += direction;
}

void sendPlayerPosition(SOCKET clientSocket, Player* player) {
    while (true) {
        char playerBuffer[sizeof(Player)];
        Player newPlayer = *player;
        memcpy(playerBuffer, &newPlayer, sizeof(Player));
        send(clientSocket, playerBuffer, sizeof(Player), 0);

        Sleep(SPEED);
    }
}

void receiveGameState(SOCKET clientSocket, GameState& gameState, int clientIndex, std::mutex& gameStateMutex) {
    while (gameState.gameRunning) {
        char gameStateBuffer[sizeof(GameState)];
        int bytesReceived = recv(clientSocket, gameStateBuffer, sizeof(GameState), 0);

        if (bytesReceived > 0) {
            std::lock_guard<std::mutex> lock(gameStateMutex);
            memcpy(&gameState, gameStateBuffer, sizeof(GameState));
            #ifdef DEBUGGING
            std::cout << "RECEIVING: " <<gameState.players[clientIndex].position.x << ", " << gameState.players[clientIndex].position.y << std::endl;
            #endif
        }
        else
        {
            gameState.gameRunning = false;
        }
    }
}

// draw all players
void drawPlayers(sf::RenderWindow& window, const GameState& gameState, int clientIndex) {
    for (int i = 0; i < 3; i++) {
        sf::RectangleShape playerShape(sf::Vector2f(50.0f, 50.0f)); // not very efficient but we ball
        playerShape.setPosition(gameState.players[i].position);

        if (gameState.players[i].isIt) {
            playerShape.setFillColor(sf::Color::Red); // Player that is "it"
        }
        else if (i == clientIndex) {
            playerShape.setFillColor(sf::Color::Green); // Current player
        } else {
            playerShape.setFillColor(sf::Color::White); // Other player
        }

        window.draw(playerShape);
    }
}

bool checkCollision(const sf::Vector2f& pos1, float size1, const sf::Vector2f& pos2, float size2) {
    sf::FloatRect box1(pos1.x, pos1.y, size1, size1);
    sf::FloatRect box2(pos2.x, pos2.y, size2, size2);

    return box1.intersects(box2);
}

int main()
{
    std::cout << "Top of main function" << std::endl;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize WinSock." << std::endl;
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(MY_PORT); // port
    serverAddr.sin_addr.s_addr = inet_addr(MY_IP); // ip
    

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to the server." << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    int clientIndex;
    recv(clientSocket, reinterpret_cast<char*>(&clientIndex), sizeof(int), 0);
    std::cout << "Connected as client " << clientIndex << std::endl;

    Player player;
    player.position = sf::Vector2f(100 * clientIndex, 200); // Initial position for debug
    player.isIt = false; // Initially the player is not "it" for debug
    if (clientIndex == 0)
        player.isIt = true;

    GameState gameState;
    gameState.gameRunning = true;
    std::mutex gameStateMutex;

    // thread for sending player position to the server
    std::thread sendThread(sendPlayerPosition, clientSocket, &player);
    // thread for receiving game state updates from the server
    std::thread receiveThread(receiveGameState, clientSocket, std::ref(gameState), clientIndex, std::ref(gameStateMutex));

    sf::RenderWindow window(sf::VideoMode(800, 600), "Tag Game");

    while (gameState.gameRunning) {
        if (!gameState.gameRunning)
            return 0;
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Handle player movement
        sf::Vector2f direction(0.0f, 0.0f);
        const float moveSpeed = 5.0f;
        if (window.hasFocus())
        {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) direction.y -= moveSpeed;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) direction.y += moveSpeed;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) direction.x -= moveSpeed;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) direction.x += moveSpeed;
        }
        {
            std::lock_guard<std::mutex> lock(gameStateMutex);
            move(player, direction);  // Update players position
        }
        for (int i = 0; i < 3; i++)
        {
            if (i == clientIndex)
                continue;
            if (checkCollision(gameState.players[i].position, 50.f, player.position, 50.f))
            {
                if (gameState.players[i].isIt)
                {
                    Sleep(200);
                    std::lock_guard<std::mutex> lock(gameStateMutex);
                    player.isIt = true;
                }
                else if (player.isIt)
                {
                    std::lock_guard<std::mutex> lock(gameStateMutex);
                    player.isIt = false;
                }
            }
        }

        window.clear(sf::Color::Black);
        drawPlayers(window, gameState, clientIndex);
        window.display();
        
        #ifdef DEBUGGING
        std::cout << gameState.players[clientIndex].position.x << ", " << gameState.players[clientIndex].position.y << std::endl;
        #endif

        Sleep(SPEED);
    }

    // Clean up
    if (sendThread.joinable()) {
        sendThread.join();
    }
    if (receiveThread.joinable()) {
        receiveThread.join();
    }

    closesocket(clientSocket);
    WSACleanup();
	return 1; // success
}