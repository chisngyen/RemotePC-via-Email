#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "socket.h"
#include "CommandExecutor.h"
#include <fstream>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 4096
using namespace std;



int main() {
    SocketServer server(DEFAULT_PORT);

    Command cmd;
    // Initialize server
    if (!server.initialize()) {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return 1;
    }

    // Create listening socket
    if (!server.createListener()) {
        std::cerr << "Failed to create listening socket." << std::endl;
        return 1;
    }

    cout << "Server is listening for connections..." << endl;

    while (true) {
        // Accept connection
        if (!server.acceptConnection()) {
            cerr << "Failed to accept client connection." << endl;
            continue;
        }

        // Handle client communication
        while (true) {
            string command = server.receiveMessage();
            string response = "";

            if (command.empty()) {
                //cerr << "Connection closed by client." << endl;
                break;
            }

            cout << "Received command from client: " << command << endl;

            if (command == "list app") {
                string applist = cmd.Applist();
                if (!server.sendMessage(applist)) {
                    cout << "Failed to send app list to client" << endl;
                }
                else {
                    cout << "Data list app sent to client.\n";
                }
            }
            else if (command == "list service") {
                string servicelist = cmd.Listservice();
                if (!server.sendMessage(servicelist)) {
                    cout << "Failed to send service list to client" << endl;
                }
                else {
                    cout << "Data list service sent to client.\n";
                }
            }
            else if (command == "list process") {
                string processList = cmd.Listprocess();
                if (!server.sendMessage(processList)) {
                    cout << "Failed to send process list to client" << endl;
                }
                else {
                    cout << "Data list process sent to client.\n";
                }
            }
            else if (command == "help") {
                string helps = cmd.help();
                if (!server.sendMessage(helps)) {
                    cout << "Failed to send help list to client: " << WSAGetLastError() << endl;
                }
                else {
                    cout << "Data list help sent to client.\n";
                }
            }
            else if (command == "screenshot") {
                int width, height;
                vector<BYTE> image = cmd.captureScreenWithGDIPlus(width, height);
                cmd.sendImage(server.getClientSocket(), image);
            }
            else if (command.size() > 4 && command.substr(0, 4) == "view" ||
                command.size() > 6 && command.substr(0, 6) == "delete") {
                cmd.handleClientCommands(server.getClientSocket(), command);
            }
            else if (command == "open_cam") {
                cmd.openCamera();
                Sleep(2000);
                int width, height;
                vector<BYTE> image = cmd.captureScreenWithGDIPlus(width, height);
                cmd.sendImage(server.getClientSocket(), image);
                cout << "Camera has been opened.\n";
            }
            else if (command == "close_cam") {
                cmd.closeCamera();
                cout << "Camera has been closed.\n";
            }
            else if (command == "shutdown") {
                cmd.shutdownComputer();
            }
        }

        server.closeClientConnection();
    }

    return 0;
}