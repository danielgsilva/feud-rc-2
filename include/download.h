#ifndef _DOWNLOAD_H_
#define _DOWNLOAD_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>

typedef struct
{
    char *user;
    char *password;
    char *host;
    char *url_path;
} ParsedUrl;

#define SERVER_PORT 21

#define SERVICE_READY 220         // Service ready for new user
#define SEND_PASSWORD 331         // Send password please
#define LOGGED_ON 230             // User logged on; requested minidisk, BFS, or SFS Directory not available; proceed
#define PASSIVE_MODE 227          // The FTP server has opened a passive connection for data transfer at the specified IP address and port.
#define OPEN_DATA_CONNECTION 150  // File status okay; about to open data connection
#define CLOSE_DATA_CONNECTION 226 // Closing data connection; requested file action successful
#define QUIT 221                  // QUIT command received

int parseUrl(char *url);
int getIp(const char *hostname);
void printParsedUrl();
int downloadApp();
int connectToServer(char *server_address, int port);
int getServerReplyCode(FILE *file);
int sendCommandToServer(FILE *file, int sockfd, const char *command, const char *command_arg, int expected_reply_code);
void sendStringToServer(int sockfd, char *buf, size_t buf_size);
int downloadFile(int sockfd);
int readFile(int fd, char *fileName);

#endif // _DOWNLOAD_H_
