#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef UNICODE
#define UNICODE
#endif

// Vsersion
#define WSVERS MAKEWORD(2,2) 
#define DEFAULT_PORT "1234"
#define _WIN32_WINNT 0x501 

// Basic Libs
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <cstdio>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <dirent.h> // easier directory access lib

// Socket Libs
#include <winsock2.h> // basic socket functions
#include <ws2tcpip.h> // required by getaddrinfo()
using namespace std;
WSADATA wsadata; // WSADATA object

// Socket Connection Funtion
SOCKET SocketConnect(char *ServerIP, char *PortNum, int SocketType){
    SOCKET DataSocket;
    addrinfo *Hints;
    if (getaddrinfo(ServerIP, PortNum, NULL, &Hints) != 0){
        return INVALID_SOCKET;
    }
    DataSocket = socket(Hints->ai_family, SocketType, 0);
    if (DataSocket == INVALID_SOCKET){
        freeaddrinfo(Hints);
        return INVALID_SOCKET;
    }
    if (connect(DataSocket, Hints->ai_addr, (int)Hints->ai_addrlen) == SOCKET_ERROR){
        closesocket(DataSocket);
        freeaddrinfo(Hints);
        return INVALID_SOCKET;
    }
    return DataSocket;
}

// Check if file in directory
bool file_in_dir(char *filename){
    DIR *directory;
    struct dirent *en;
    directory = opendir("./");
    if(directory){
        while((en = readdir(directory)) != NULL){
            if(strcmp(en->d_name, filename) == 0){
                return true;
            }
        }
        closedir(directory);
    }
    return false;
}

int main(int argc, char *argv[]){
    struct sockaddr_storage clientAddress; // IPV6 Sturct
    SOCKET s, ns;
    SOCKET ns_data, s_data_act;
    char clientHost[NI_MAXHOST], clientService[NI_MAXSERV], portNum[NI_MAXSERV];
    char send_buffer[80], receive_buffer[80];
    int n, bytes, addrlen, err, active = 0;
    bool IPV6_active = true;
    bool binOrAsc; // Binary = true Ascii = false

    //char const *userName = "napoleon";
    //char const *password = "342";

    // WSSTARTUP
    err = WSAStartup(WSVERS, &wsadata);
    if(err != 0){
        WSACleanup();
        cout << "WSAStartup failed with error: " << err << endl;
        exit(0);
    }

    // Confirm the WinSock DLL Version
    if(LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wHighVersion) != 2){
        cout << "Could not find a usable version of Winsock.dll" << endl;
        WSACleanup(); exit(0);
    }else{
        cout << "The Winsock 2.2 dll was initialised" << endl;
    }

    // Socket Address Structures
    struct addrinfo *result = NULL;
    struct addrinfo hints;
    struct sockaddr_in local_data_addr_act; // IPV4 Struct For Port Command

    int iResult;
    memset(&hints, 0, sizeof(addrinfo));

    // IP Vesrion Switch
    if(argc == 3){
        if(strcmp(argv[2], "IPV6") == 0){
            IPV6_active = true;
            cout << "IPV6 Active" << endl;
        }else{
            IPV6_active = false;
            cout << "IPV4 Active" << endl;
        }
    }
    // Set IPV6 or IPV4
    if(IPV6_active == true){
        hints.ai_family = AF_INET6;
    }else if(IPV6_active == false){
        hints.ai_family = AF_INET;
    }
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    if(argc == 3){
        iResult = getaddrinfo(NULL, argv[1], &hints, &result);
        sprintf(portNum, "%s", argv[1]);
    }else{
        iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
        sprintf(portNum, "%s", DEFAULT_PORT);
    }
    if(iResult != 0){
        cout << "getaddrinfo failed: " << iResult << endl;
        WSACleanup(); exit(0);
    }

    // Welcome Socket
    s = INVALID_SOCKET;
    s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if(s == INVALID_SOCKET){
        cout << "Error at socket(): " << WSAGetLastError() << endl;
        freeaddrinfo(result); WSACleanup(); exit(0);
    }

    // Binding Socket
    iResult = bind(s, result->ai_addr, (int)result->ai_addrlen);
    if(iResult == SOCKET_ERROR){
        cout << "bind failed with error: " << WSAGetLastError() << endl;
        closesocket(s); WSACleanup(); exit(0);
    }
    freeaddrinfo(result);

    // Listen
    if(listen(s, SOMAXCONN) == SOCKET_ERROR){
        cout << "Listen failed with error: " << WSAGetLastError() << endl;
        closesocket(s); WSACleanup(); exit(0);
    }else{
        cout << endl << "<<<SERVER>>> is listening at PORT: " << portNum << endl;
    }

    // Program Loop
    while(true){
        // Accepting Client
        addrlen = sizeof(clientAddress);
        ns = INVALID_SOCKET;
        ns = accept(s,(struct sockaddr *)(&clientAddress), &addrlen);
        if(ns == INVALID_SOCKET){
            cout << "accept failed: " << WSAGetLastError() << endl;
            closesocket(s); WSACleanup(); exit(0);
        }else{
            cout << "CLIENT has been accepted" << endl;
            memset(clientHost, 0, sizeof(clientHost));
            memset(clientService, 0, sizeof(clientService));
            getnameinfo((struct sockaddr *)&clientAddress, addrlen, clientHost, sizeof(clientHost), clientService, sizeof(clientService), NI_NUMERICHOST);
            cout << "Connected to CLIENT with IP address: " << clientHost << " at Port: " << clientService << endl;
            sprintf(send_buffer,"220 FTP Server ready. \r\n");
			bytes = send(ns, send_buffer, strlen(send_buffer), 0);
        }
        // Communication
        while(true){
            n = 0;
            while(true){
                bytes = recv(ns, &receive_buffer[n], 1, 0);
                if((bytes < 0) || (bytes == 0)) break;
                if(receive_buffer[n] == '\n'){
                    receive_buffer[n] = '\0';
                    break;
                }
                if(receive_buffer[n] != '\r') n++; 
            }
            if(bytes == 0) printf("\nclient has gracefully exited.\n");
            if((bytes < 0) || (bytes == 0)) break;
            printf("<< DEBUG INFO. >>: the message from the CLIENT reads: '%s\\r\\n' \n", receive_buffer);
            
            if(strncmp(receive_buffer,"USER",4)==0){
                printf("Logging in... \n");
                sprintf(send_buffer,"331 Password required\r\n");
                bytes = send(ns, send_buffer, strlen(send_buffer), 0);
                if(bytes < 0) break;
            }
            if(strncmp(receive_buffer,"PASS",4)==0){
                sprintf(send_buffer,"230 Public login sucessful \r\n");
                printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
                bytes = send(ns, send_buffer, strlen(send_buffer), 0);
                if(bytes < 0) break;
            }
            if(strncmp(receive_buffer,"SYST",4)==0){
                printf("Information about the system \n");
                sprintf(send_buffer,"215 Windows 64-bit\r\n");
                printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
                bytes = send(ns, send_buffer, strlen(send_buffer), 0);
                if(bytes < 0) break;
            }
            if(strncmp(receive_buffer,"OPTS",4)==0){
                printf("unrecognised command \n");
                sprintf(send_buffer,"502 command not implemented\r\n");
                printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
                bytes = send(ns, send_buffer, strlen(send_buffer), 0);
                if(bytes < 0) break;
            }
            if(strncmp(receive_buffer,"QUIT",4)==0){
                printf("Quit \n");
                sprintf(send_buffer,"221 Connection close by client\r\n");
                printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
                bytes = send(ns, send_buffer, strlen(send_buffer), 0);
                if (bytes < 0) break;
            }
            if(strncmp(receive_buffer,"PORT",4)==0){
                s_data_act = socket(AF_INET, SOCK_STREAM, 0);
                //local variables
                int act_port[2];
                int act_ip[4], port_dec;
                char ip_decimal[40];
                printf("===================================================\n");
                printf("\n\tActive FTP mode, the client is listening... \n");
                active=1;
                int scannedItems = sscanf(receive_buffer, "PORT %d,%d,%d,%d,%d,%d", &act_ip[0],&act_ip[1],&act_ip[2],&act_ip[3], &act_port[0],&act_port[1]);
                if(scannedItems < 6){
                    sprintf(send_buffer,"501 Syntax error in arguments \r\n");
                    printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
                    bytes = send(ns, send_buffer, strlen(send_buffer), 0);
                    break;
                }
                local_data_addr_act.sin_family=AF_INET;//local_data_addr_act  //ipv4 only 
                sprintf(ip_decimal, "%d.%d.%d.%d", act_ip[0], act_ip[1], act_ip[2],act_ip[3]);
                printf("\tCLIENT's IP is %s\n",ip_decimal);  //IPv4 format
                local_data_addr_act.sin_addr.s_addr=inet_addr(ip_decimal);  //ipv4 only
                port_dec=act_port[0];
                port_dec=port_dec << 8;
                port_dec=port_dec+act_port[1];
                printf("\tCLIENT's Port is %d\n",port_dec);
                printf("===================================================\n");
                local_data_addr_act.sin_port=htons(port_dec); //ipv4 only
                if(connect(s_data_act, (struct sockaddr *)&local_data_addr_act, (int) sizeof(struct sockaddr)) != 0){
                    printf("trying connection in %s %d\n",inet_ntoa(local_data_addr_act.sin_addr),ntohs(local_data_addr_act.sin_port));
                    sprintf(send_buffer, "425 Something is wrong, can't start active connection... \r\n");
                    bytes = send(ns, send_buffer, strlen(send_buffer), 0);
                    printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
                    closesocket(s_data_act);
                }else{
                    sprintf(send_buffer, "200 PORT Command successful\r\n");
                    bytes = send(ns, send_buffer, strlen(send_buffer), 0);
                    printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
                    printf("Connected to client\n");
                }
            }
            if((strncmp(receive_buffer,"LIST",4)==0) || (strncmp(receive_buffer,"NLST",4)==0)){
                system("dir > tmp.txt");
                FILE *fin=fopen("tmp.txt","r");
                sprintf(send_buffer,"150 Opening ASCII mode data connection... \r\n");
                printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
                bytes = send(ns, send_buffer, strlen(send_buffer), 0);
                char temp_buffer[80];
                while(!feof(fin)){
                    fgets(temp_buffer,78,fin);
                    sprintf(send_buffer,"%s",temp_buffer);
                    if (active==0) send(ns_data, send_buffer, strlen(send_buffer), 0);
                    else send(s_data_act, send_buffer, strlen(send_buffer), 0);
                }
                fclose(fin);
                sprintf(send_buffer,"226 File transfer complete. \r\n");
                printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
                bytes = send(ns, send_buffer, strlen(send_buffer), 0);
                if(active==0)closesocket(ns_data);
                else closesocket(s_data_act);
                //system("del tmp.txt");
            }
            if(strncmp(receive_buffer,"EPRT",4)==0){
                // Splitting the EPRT command
                int ipVersion;
                char ipv6_str[64], rest[47], port[12];
                int scanned_items = sscanf(receive_buffer, "EPRT |%d|%s", &ipVersion, rest);
                char *first = strchr(rest, '|');
                *first = 0;
                scanned_items = sscanf(rest, "%s", ipv6_str);
                char *portT = first + 1;
                char *second = strchr(portT, '|');
                *second = 0;
                scanned_items = sscanf(portT, "%s", port);

                // Debug Info
                cout << "===================================" << endl;
                cout << "Requested Ip Version: " << ipVersion << endl;
                cout << "Clients Ip: " << ipv6_str << endl;
                cout << "Clients Port: " << port << endl;
                cout << "===================================" << endl;

                // Open Data Socket
                s_data_act = SocketConnect(ipv6_str, port, SOCK_STREAM);
                active = 1; // active mode enabled

                // Check EPRT Version
                if(ipVersion != 2){
                    sprintf(send_buffer,"522 (Network protocol not supported, use (2)) \r\n");
                    printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
                    bytes = send(ns, send_buffer, strlen(send_buffer), 0);
                }
                sprintf(send_buffer, "200 EPRT Command successful\r\n");
                bytes = send(ns, send_buffer, strlen(send_buffer), 0);
                printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
                
            }
            if(strncmp(receive_buffer,"TYPE",4)==0){
                // Split the Type Command
                char dataType[10];
                int scanned_items = sscanf(receive_buffer, "TYPE %s", dataType);
                // Check data type
                if(strcmp(dataType, "A") == 0){
                    binOrAsc = false;
                    sprintf(send_buffer, "200 TYPE Command successful (ASCII MODE)\r\n");
                    bytes = send(ns, send_buffer, strlen(send_buffer), 0);
                    printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
                }else if(strcmp(dataType, "I") == 0){
                    binOrAsc = true;
                    sprintf(send_buffer, "200 TYPE Command successful (Binary MODE)\r\n");
                    bytes = send(ns, send_buffer, strlen(send_buffer), 0);
                    printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
                }else{
                    sprintf(send_buffer, "421 Unsupported DataType\r\n");
                    bytes = send(ns, send_buffer, strlen(send_buffer), 0);
                    printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
                }
            }
            if(strncmp(receive_buffer,"RETR",4)==0){
                // Split the RETR command
                char filename[FILENAME_MAX];
                char Binbuffer[1024];
                char ASCbuffer[1024];
                int scanned_items = sscanf(receive_buffer, "RETR %s", filename);

                // debug info
                cout << "===================================" << endl;
                cout << "Client requested file: " << filename << endl;
                cout << "===================================" << endl;

                // check if file is present
                if(file_in_dir(filename)){
                    if(binOrAsc == true){
                        // binary
                        sprintf(send_buffer,"150 Opening Binary mode data connection... \r\n");
                        printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
                        bytes = send(ns, send_buffer, strlen(send_buffer), 0);
                        FILE *BinFile = fopen(filename, "rb");
                        while(!feof(BinFile)){
                            fread(Binbuffer,sizeof(unsigned char),1024,BinFile);
                            if (active==0) send(ns_data, send_buffer, strlen(send_buffer), 0);
                            else send(s_data_act, Binbuffer, sizeof(Binbuffer), 0);
                        }
                        fclose(BinFile);
                        sprintf(send_buffer,"226 File transfer complete. \r\n");
                        printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
                        bytes = send(ns, send_buffer, strlen(send_buffer), 0);
                        if(active==0)closesocket(ns_data);
                        else closesocket(s_data_act);

                    }else{
                        // ascii
                        sprintf(send_buffer,"150 Opening ASCII mode data connection... \r\n");
                        printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
                        bytes = send(ns, send_buffer, strlen(send_buffer), 0);
                        FILE *AscFile = fopen(filename, "r");;
                        while(!feof(AscFile)){
                            fgets(ASCbuffer,1024,AscFile);
                            sprintf(send_buffer,"%s",ASCbuffer);
                            if (active==0) send(ns_data, send_buffer, strlen(send_buffer), 0);
                            else send(s_data_act, send_buffer, strlen(send_buffer), 0);
                        }
                        fclose(AscFile);
                        sprintf(send_buffer,"226 File transfer complete. \r\n");
                        printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
                        bytes = send(ns, send_buffer, strlen(send_buffer), 0);
                        if(active==0)closesocket(ns_data);
                        else closesocket(s_data_act);
                    }
                }else{
                    //file not found
                    sprintf(send_buffer,"550 (file-does-not-exist) \r\n");
                    printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
                    bytes = send(ns, send_buffer, strlen(send_buffer), 0);
                    if (bytes < 0) break;
                }
            }
        }
        // Close socket
        int iResult = shutdown(ns, SD_SEND);
        if(iResult == SOCKET_ERROR){
            cout << "shutdown failed with error: " << WSAGetLastError() << endl;
            closesocket(ns); WSACleanup(); exit(0);
        }
        closesocket(ns);
        cout << "===================================" << endl;
        cout << "disconnected from CLIENT with IP address: " << clientHost << " Port: " << clientService << endl;
        cout << "===================================" << endl;
    }
    closesocket(s);
    WSACleanup();
    return(0);
}
