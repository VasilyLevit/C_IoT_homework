
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>

int main()
{
    int client;
    int portNum = 1500; // Номер порта (один для сервера и клиента)
    bool isExit = false;
    int bufsize = 1024;
    char buffer[bufsize];
    char* ip = "127.0.0.1";

    struct sockaddr_in server_addr;

    client = socket(AF_INET, SOCK_STREAM, 0);

    printf("CLIENT\n");

    if (client < 0) 
    {
        printf("Error establishing socket...\n");
        exit(1);
    }

    printf("=> Socket client has been created...\n");
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portNum);
   
    inet_pton(AF_INET, ip, &server_addr.sin_addr);


    if (connect(client,(struct sockaddr *)&server_addr, sizeof(server_addr)) == 0)
        printf("=> Connection to the server %s with port number: %d\n",inet_ntoa(server_addr.sin_addr),portNum);

    if (connect(client,(struct sockaddr *)&server_addr, sizeof(server_addr)) == 0)
        printf("=> Connection to the server port number: %d \n",portNum);


    printf("=> Awaiting confirmation from the server...\n");
    recv(client, buffer, bufsize, 0);
    printf("=> Connection confirmed, you are good to go...\n");

    printf("\n\n=> Enter # to end the connection\n");

    
    do {
		printf("Client: ");
        do {
            scanf("%s",buffer);
            send(client, buffer, bufsize, 0);
            if (buffer[0] == '#') 
            {
                isExit = true;
            }
        } while (buffer[0] != '.' &&  buffer[0] != '#');

        printf("Server: ");
        do {
            recv(client, buffer, bufsize, 0);
            printf("%s ",buffer);
            if (buffer[0] == '#') 
            {
                isExit = true;
            }
        } while (buffer[0] != '.' &&  buffer[0] != '#');
        printf("\n");

    } while (!isExit);

    printf("\n=> Connection terminated.\n\nGoodbye...\n");

    close(client);
    return 0;
}