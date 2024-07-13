
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    int server;  //файл-дескриптор сервера
    int client;  //файл-дескриптор клиента 
    int portNum = 1500;  //номера порта (0 до 65535)
    bool isExit = false; //признак завершения программы.
    int bufsize = 1024;  //размер буффера
    char buffer[bufsize]; //буффер обмена для приема/отправки данных к/от сервера
 
    struct sockaddr_in server_addr;
    socklen_t size;
    
    server = socket(AF_INET, SOCK_STREAM, 0);
    printf("SERVER\n");

    if (server < 0) 
    {
        printf("Error establishing socket...\n");
        exit(1);
    }
	
	printf("=> Socket server has been created...\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(portNum);

    if ((bind(server, (struct sockaddr*)&server_addr,sizeof(server_addr))) < 0) 
    {
        printf("=> Error binding connection, the socket has already been established...\n");
        return -1;
    }

    size = sizeof(server_addr);
    printf("=> Looking for clients...\n");
 
    listen(server, 1);

    int clientCount = 1;
    client = accept(server,(struct sockaddr *)&server_addr,&size);

    if (client < 0) 
        printf("=> Error on accepting...");

    while (client > 0) 
    {
        printf("=> Connected with the client %d, you are good to go...\n",clientCount);
        printf("\n=> Enter # to end the connection\n");       
        strcpy(buffer, "=> Server connected...\n");
        send(client, buffer, bufsize, 0);

        do {
			printf("Client: ");
            do {
                recv(client, buffer, bufsize, 0);
                printf("%s ",buffer);
                if (buffer[0] == '#') 
                {
                    isExit = true;
                }
            } while (buffer[0] != '.' &&  buffer[0] != '#');                        
                        
            printf("\nServer: ");
            do {
                scanf("%s",buffer);
                send(client, buffer, bufsize, 0);
                if (buffer[0] == '#') 
                {
                    isExit = true;
                }
            } while (buffer[0] != '.' &&  buffer[0] != '#');            


        } while (!isExit);
 
        printf("\n\n=> Connection terminated with IP %s\n",inet_ntoa(server_addr.sin_addr));   
        close(client);
        close(server); 
        printf("\nGoodbye...");
        isExit = false;
        return 0;
    }
    return 0;
}