#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

#define BUFFERSIZE 100
#define SPORT 5550
#define SERVER_MODE
using namespace std;

void print_form(const char* frm[], char* sb);
int sb_check(char sb[][3]);
int create_OX_server(int port);
SOCKET connect_Rplayer(const char* ip, int port);
void waitClient(SOCKET sck1);
void play_game(SOCKET rsck, bool is_server);

bool is_server = false;

int main() {
    SOCKET rsck;
    char IP[20];
    WSADATA WsaData;

    if (0 != WSAStartup(MAKEWORD(2, 2), &WsaData)) {
        cerr << "WSAStartup failed" << endl;
        WSACleanup();
        exit(0);
    }

#ifdef SERVER_MODE
    is_server = true;
    if (!create_OX_server(SPORT)) {
        cerr << "Failed to create server" << endl;
        exit(0);
    }

    // Server mode, waiting for client
    while (true) {
        Sleep(1000);
    }
#else
    cout << "IP: ";
    cin >> setw(sizeof(IP)) >> IP;   // setw: read at most 19 chars so it can't overflow IP[20]
    rsck = connect_Rplayer(IP, SPORT);
    play_game(rsck, false);

    closesocket(rsck);
    WSACleanup();
    system("pause");   // keep the window open after the game so the result stays visible
#endif
    return 0;
}

int sb_check(char sb[][3]) {
    for (int i = 0; i < 3; i++) {
        if (sb[i][0] == sb[i][1] && sb[i][1] == sb[i][2]) return 1;
    }
    for (int i = 0; i < 3; i++) {
        if (sb[0][i] == sb[1][i] && sb[1][i] == sb[2][i]) return 1;
    }
    if (sb[0][0] == sb[1][1] && sb[1][1] == sb[2][2]) return 1;
    if (sb[2][0] == sb[1][1] && sb[1][1] == sb[0][2]) return 1;
    return 0;
}

void print_form(const char* frm[], char* p) {
    int i, k = 0;
    for (i = 0; i < 5; i++) {
        if (!(i % 2)) {
            printf(frm[i], p[k], p[k + 1], p[k + 2]);
            k += 3;
        }
        else {
            printf(frm[i]);
        }
    }
}

int create_OX_server(int port) {
    static SOCKET sck1;
    SOCKADDR_IN sa;

    sck1 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == sck1) {
        cerr << "Socket creation failed" << endl;
        WSACleanup();
        exit(0);
    }

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = INADDR_ANY;
    if (SOCKET_ERROR == ::bind(sck1, (LPSOCKADDR)&sa, sizeof(sa))) {
        cerr << "Bind failed" << endl;
        closesocket(sck1);
        WSACleanup();
        exit(0);
    }

    if (SOCKET_ERROR == listen(sck1, 1)) {
        cerr << "Listen failed" << endl;
        closesocket(sck1);
        WSACleanup();
        exit(0);
    }

    cout << "Server succeeded!" << endl;
    cout << "Waiting for new clients..." << endl;

    std::thread(waitClient, sck1).detach();
    return 1;
}

void waitClient(SOCKET sck1) {
    sockaddr_in addrClient;
    int addrClientlen = sizeof(addrClient);
    SOCKET ClientSocket;

    ClientSocket = accept(sck1, (sockaddr FAR*) & addrClient, &addrClientlen);
    if (INVALID_SOCKET == ClientSocket) {
        cout << "Socket accept error" << endl;
        return;
    }

    play_game(ClientSocket, true);

    closesocket(ClientSocket);
    cout << "Client Exit!!" << endl;

    // wait for the next player only after this game ends (otherwise two players would fight over the server's keyboard input)
    cout << "Waiting for new clients..." << endl;
    std::thread(waitClient, sck1).detach();
}

void play_game(SOCKET rsck, bool is_server) {
    const char* frm[] = {
        "%c|%c|%c\n",
        "-+-+-\n",
        "%c|%c|%c\n",
        "-+-+-\n",
        "%c|%c|%c\n\n"
    };
    char sb[3][3] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    char turn = 'O';                   // the first move is always O (server goes first), so both boards match
    int pos = 0, n = 0;
    int client_turn = !is_server;      // client_turn = 1: opponent's turn, wait for their move
    char buf[BUFFERSIZE];
    int rVal;

    print_form(frm, (char*)sb);

    do {
        if (client_turn) {
            rVal = recv(rsck, buf, BUFFERSIZE, MSG_WAITALL);   // MSG_WAITALL: return only after all 100 bytes arrive
            if (rVal <= 0) {
                cout << "Connection closed or error occurred" << endl;
                break;
            }
            if (sscanf_s(buf, "%d", &pos) != 1 || pos < 1 || pos > 9) {
                cout << "Received invalid move" << endl;
                break;
            }
            //cout << "Received position: " << pos << endl;

            // Update local game board with opponent's move
            sb[(pos - 1) / 3][(pos - 1) % 3] = turn;
            print_form(frm, (char*)sb);
        }
        else {
            cout << "[" << turn << "]>>";
            while (true) {
                if (!(cin >> pos)) {          // input is not a number (e.g. a letter)
                    cin.clear();              // clear the error state, otherwise this loops forever
                    cin.ignore(1000, '\n');   // discard the rest of the line
                    pos = 0;
                }
                if (pos >= 1 && pos <= 9 && ((char*)sb)[pos - 1] != 'O' && ((char*)sb)[pos - 1] != 'X') {
                    break;
                }
                else {
                    cout << "Invalid move, try again.\n[" << turn << "]>>";
                }
            }

            // Update local game board with my move
            sb[(pos - 1) / 3][(pos - 1) % 3] = turn;
            print_form(frm, (char*)sb);

            sprintf_s(buf, BUFFERSIZE, "%d", pos);
            rVal = send(rsck, buf, BUFFERSIZE, 0);
            if (rVal <= 0) {
                cout << "Connection closed or error occurred" << endl;
                break;
            }
            //cout << "Sent position: " << pos << endl;
        }

        if (sb_check(sb)) {
            cout << "Winner is [" << turn << "]!!!" << endl;
            break;
        }
        turn = (turn == 'O') ? 'X' : 'O';
        client_turn = !client_turn;
        n++;
    } while (n < 9);

    if (n == 9) {
        cout << "The competition is draw!!!" << endl;
    }
}

SOCKET connect_Rplayer(const char* ip, int port) {
    SOCKET sc;
    SOCKADDR_IN sa;

    sc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == sc) {
        WSACleanup();
        exit(0);
    }

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &sa.sin_addr) != 1) {
        cerr << "Invalid IP address: " << ip << endl;
        closesocket(sc);
        WSACleanup();
        exit(0);
    }

    if (SOCKET_ERROR == connect(sc, (LPSOCKADDR)&sa, sizeof(sa))) {
        cerr << "Connect server error!" << endl;
        WSACleanup();
        exit(0);
    }

    return sc;
}