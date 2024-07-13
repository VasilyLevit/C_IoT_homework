
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
    int portNum = 8000;  //номера порта (0 до 65535)
    bool isExit = false;  //признак завершения программы.
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
    
    
    while(!isExit)
    {
        client = accept(server,(struct sockaddr *)&server_addr,&size);

        if (client < 0) 
            printf("=> Error on accepting...");

        if(client > 0) 
        {
            printf("=> Connected with the client %d, you are good to go...\n",clientCount);

            int result = recv(client, buffer, bufsize, 0);
            if (result < 0)
            {
                printf("\n\n=> Connection terminated error %d  with IP %s\n",result,inet_ntoa(server_addr.sin_addr));   
                close(client);
                exit(1);
            }

            buffer[result] = '\0';               
            char response[1024] = "HTTP/1.1 200 OK\r\n"
                "Version: HTTP/1.1\r\n"
                "Content-Type: text/html; charset=utf-8\r\n"
                "\r\n\r\n"
                    "<!DOCTYPE HTML>"
                    "<html>"
                    "  <head>"
                    "    <meta name=\"viewport\" content=\"width=device-width,"
                    "    initial-scale=1\">"
                    "  </head>"
                    "  <h1>OrangePI - Web Server</h1>"
                    "  <p>Buttons"
                    "    <a href=\"ON\">"
                    "      <button>ON</button>"
                    "    </a>&nbsp;"
                    "    <a href=\"OFF\">"
                    "      <button>OFF</button>"
                    "    </a>"
                    "  </p>";
            strcat(response,"</html>");
            printf("%s\n",buffer);
            char* get_str_on = strstr(buffer, "/ON");
            char* get_str_off = strstr(buffer, "/OFF");
            if (get_str_on)
            {
                printf("%s\n", get_str_on);
            }
            if (get_str_off)
            {
                printf("%s\n", get_str_off);
            }
            
            send(client, response,strlen(response), 0);
            printf("\n\n=> Connection terminated with IP %s\n",inet_ntoa(server_addr.sin_addr));   
            close(client);
            printf("\n=> Press any key and <Enter>, # to end the connection\n");
            char c;
            while((c=getchar())!='\n')
                if(c=='#')
                {
                    isExit=true;                 
                }
        }
    }
    close(server); 
    printf("\nGoodbye...");
    isExit = false;
    return 0;
}