#include "download.h"

ParsedUrl connectionParameters;

// ftp://[<user>:<password>@]<host>/<url-path>
int parseUrl(char *url)
{
    if (strncmp(url, "ftp://", 6) != 0) // checks if the url has "ftp://"
        return -1;

    char *user = NULL;
    char *password = NULL;
    char *hostname = NULL;
    char *url_path = NULL;

    size_t url_size = strlen(url + 6); // +6 skip "ftp://"
    char *host_component = strtok(url + 6, "/");
    if (strlen(host_component) < url_size - 1) // checks if the url has <url-path>
    {
        url_path = url + 6 + strlen(host_component) + 1;
    }
    else
    {
        printf("No <url-path>\n");
        return -1;
    }
    if (host_component == NULL || url_path == NULL)
        return -1;

    if (strstr(host_component, "@") != NULL) // checks if the url has user and password
    {
        char *user_pass = strtok(host_component, "@");
        hostname = strtok(NULL, "@");
        if (hostname == NULL)
            return -1;
        user = strtok(user_pass, ":");
        password = strtok(NULL, ":");
    }
    else
        hostname = host_component;

    if (user != NULL)
    {
        connectionParameters.user = (char *)malloc(strlen(user) * sizeof(char));
        strcpy(connectionParameters.user, user);
        if (password == NULL)
            password = "";
        connectionParameters.password = (char *)malloc(strlen(password) * sizeof(char));
        strcpy(connectionParameters.password, password);
    }
    else
    {
        connectionParameters.user = (char *)malloc(strlen("anonymous") * sizeof(char));
        connectionParameters.user = "anonymous";
        connectionParameters.password = (char *)malloc(strlen("any-password") * sizeof(char));
        connectionParameters.password = "any-password";
    }

    if (getIp(hostname) != 0)
    {
        printf("Error getIp\n");
        return -1;
    }

    connectionParameters.url_path = (char *)malloc(strlen(url_path) * sizeof(char));
    strcpy(connectionParameters.url_path, url_path);

    return 0;
}

int getIp(const char *hostname)
{
    /**
    * The struct hostent (host entry) with its terms documented

    struct hostent {
        char *h_name;    // Official name of the host.
        char **h_aliases;    // A NULL-terminated array of alternate names for the host.
        int h_addrtype;    // The type of address being returned; usually AF_INET.
        int h_length;    // The length of the address in bytes.
        char **h_addr_list;    // A zero-terminated array of network addresses for the host.
        // Host addresses are in Network Byte Order.
    };

    #define h_addr h_addr_list[0]	The first address in h_addr_list.
    */

    printf("Get IP\n");
    struct hostent *h;
    if ((h = gethostbyname(hostname)) == NULL)
    {
        herror("gethostbyname()");
        exit(-1);
    }
    printf("Host name  : %s\n", h->h_name);
    char *ip_address = inet_ntoa(*((struct in_addr *)h->h_addr));
    printf("IP Address : %s\n", ip_address);
    connectionParameters.host = (char *)malloc(strlen(ip_address) * sizeof(char));
    strcpy(connectionParameters.host, ip_address);

    return 0;
}

void printParsedUrl()
{
    printf("\nStarting download application\n"
           "  - User: %s\n"
           "  - Password: %s\n"
           "  - Host: %s\n"
           "  - URL-Path: %s\n\n",
           connectionParameters.user,
           connectionParameters.password,
           connectionParameters.host,
           connectionParameters.url_path);
}

int downloadApp()
{
    int sockfd = connectToServer(connectionParameters.host, SERVER_PORT);

    FILE *file = fdopen(sockfd, "r");

    if (getServerReplyCode(file) != SERVICE_READY)
    {
        printf("Service not ready\n");
        return -1;
    }

    /*send USER command to the server*/
    if (sendCommandToServer(file, sockfd, "USER", connectionParameters.user, SEND_PASSWORD) == -1)
    {
        printf("Wrong user. Not logged on\n");
        return -1;
    }

    /*send PASS command to the server*/
    if (sendCommandToServer(file, sockfd, "PASS", connectionParameters.password, LOGGED_ON) == -1)
    {
        printf("Wrong password. Not logged on\n\n");
        sendCommandToServer(file, sockfd, "USER", connectionParameters.user, SEND_PASSWORD);
        printf("New password: ");
        char new_password[50];
        scanf("%s", new_password);
        if (sendCommandToServer(file, sockfd, "PASS", new_password, LOGGED_ON) == -1)
        {
            printf("Wrong password. Not logged on\n");
            return -1;
        }
    }

    /*send PASV command to the server*/
    size_t buf_size = 5;
    char *buf = (char *)malloc(buf_size * sizeof(char));
    sprintf(buf, "PASV\n");
    sendStringToServer(sockfd, buf, buf_size);

    if (downloadFile(sockfd) == -1){
        printf("Error downloadFile()\n");
        return -1;
    }

    /*send QUIT command to the server*/
    if (sendCommandToServer(file, sockfd, "QUIT", "", QUIT) == -1)
    {
        printf("QUIT command not received\n");
        return -1;
    }

    if (close(sockfd) < 0)
    {
        perror("close()");
        exit(-1);
    }

    return 0;
}

int connectToServer(char *server_address, int port)
{
    int sockfd;
    struct sockaddr_in server_addr;

    /*server address handling*/
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_address); /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(port);                      /*server TCP port must be network byte ordered */

    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket()");
        exit(-1);
    }

    /*connect to the server*/
    if (connect(sockfd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("connect()");
        exit(-1);
    }

    return sockfd;
}

int sendCommandToServer(FILE *file, int sockfd, const char *command, const char *command_arg, int expected_reply_code)
{
    size_t buf_size = strlen(command) + strlen(command_arg) + 2;
    char *buf = (char *)malloc(buf_size * sizeof(char));
    sprintf(buf, "%s %s\n", command, command_arg);
    sendStringToServer(sockfd, buf, buf_size);
    if (getServerReplyCode(file) != expected_reply_code)
        return -1;
    free(buf);

    return 0;
}

int getServerReplyCode(FILE *file)
{
    int reply_code = -1;
    char *line = NULL;
    size_t len = 0;
    while (getline(&line, &len, file) != -1)
    {
        printf("%s\n", line);
        if (line[3] == ' ')
        {
            reply_code = atoi(line);
            break;
        }
    }
    free(line);

    return reply_code;
}

void sendStringToServer(int sockfd, char *buf, size_t buf_size)
{
    printf("%s\n", buf);
    /*send a string to the server*/
    if (write(sockfd, buf, buf_size) <= 0)
    {
        perror("write()");
        exit(-1);
    }
}

int downloadFile(int sockfd)
{
    FILE *stream = fdopen(sockfd, "r");
    char *line = NULL;
    size_t len = 0;
    getline(&line, &len, stream);
    printf("%s\n", line);
    int reply_code = atoi(line);
    if (reply_code != PASSIVE_MODE)
    {
        free(line);
        return -1;
    }

    int ip_address[4];
    int port[2];
    sscanf(line, "227 Entering Passive Mode (%d, %d, %d, %d, %d, %d).\n",
           &ip_address[0], &ip_address[1], &ip_address[2], &ip_address[3], &port[0], &port[1]);
    free(line);
    char server_ip_address[16];
    sprintf(server_ip_address, "%d.%d.%d.%d", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    int port_file_transfer = 256 * port[0] + port[1];

    int sockfd2 = connectToServer(server_ip_address, port_file_transfer);

    /*send RETR command to the server*/
    if (sendCommandToServer(stream, sockfd, "RETR", connectionParameters.url_path, OPEN_DATA_CONNECTION) == -1)
    {
        printf("Error opening data connection\n");
        return -1;
    }

    char *fileName = basename(connectionParameters.url_path);
    if (readFile(sockfd2, fileName) == -1){
        printf("Error readFile()\n");
        return -1;
    }

    if (getServerReplyCode(stream) != CLOSE_DATA_CONNECTION)
    {
        printf("Error closing data connection\n");
        return -1;
    }

    if (close(sockfd2) < 0)
    {
        perror("close()");
        exit(-1);
    }

    return 0;
}

int readFile(int fd, char *fileName)
{
    int file_fd;
    if ((file_fd = open(fileName, O_WRONLY | O_CREAT, 0666)) == -1)
    {
        perror("Open file");
        return -1;
    }
    char buf[1];
    int bytes;
    while (1)
    {
        bytes = read(fd, &buf[0], 1);
        if (bytes == 0)
            break;
        if (bytes < 0)
        {
            close(file_fd);
            return -1;
        }
        bytes = write(file_fd, &buf[0], 1);
        if (bytes < 0)
        {
            close(file_fd);
            perror("Couldn't write to file");
        }
    }
    close(file_fd);

    return 0;
}
