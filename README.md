# FTP Download Application

This project is a simple FTP client written in C, according to the specification provided in [this](./doc/Lab2-Guide.pdf) guide. It implements a subset of the FTP protocol (as defined in **RFC959**) to download a single file from a remote server. The application takes a standard FTP URL (as defined in **RFC1738**) as a command-line argument.

## Features 

* `RFC1738 URL Parsing`: Correctly parses URLs in the format `ftp://[<user>:<password>@]<host>/<url-path>`

* `DNS Resolution`: Uses `gethostbyname` to resolve the host's IP address

* `FTP Command Implementation`: Handles core commands - `USER`, `PASS`, `PASV`, `RETR`, and `QUIT`

* `Authentication`: Supports both specified user/password combinations and defaults to `anonymous`/`any-password` if omitted

* `Interactive Password`: If a password fails or is not provided, the application will interactively prompt the user for one

* `Passive Mode (PASV)`: Implements passive mode for data transfers, where the client initiates both the control and data connections

* `Dual Socket Management`: Manages a persistent TCP control connection (port 21) and a separate, temporary TCP data connection for the file transfer

* `FTP Reply Code Parsing`: Reads and interprets server reply codes (e.g., `220`, `227`, `150`, `226`) to manage the connection state

## How it Works

The application follows a precise sequence to download a file:

1. `Parse URL`: The `parseUrl` function uses `strtok` to deconstruct the input URL. It extracts the `user`, `password`, `host`, and `url-path` into a `ParsedUrl` struct. If user/password are absent, it defaults to an `anonymous` login

2. `Get IP Address`: The `getIp` function is called to resolve the hostname into a server IP address

3. `Control Connection`: A TCP socket is opened to the server's IP on port 21. The client then waits for the server's `220` (Service Ready) welcome message

4. `Authentication`: The client sends the `USER` and `PASS` commands. If the login fails (e.g., wrong password), the user is given a chance to re-enter the password

5. `Enter Passive Mode`: The client sends the `PASV` command. The server responds with a `227` (Entering Passive Mode) message, which includes an IP address and a new port number

6. `Data Connection`: The `downloadFile` function parses the `227` response to calculate the correct data port. It then opens a **second** TCP socket to the server on this new port

7. `File Transfer`:
    * The client sends the `RETR <url-path>` command over the original **control connection**
    * After receiving a `150` (File status okay) reply, the `readFile` function begins reading all data from the **data connection**
    * This data is written byte-by-byte into a new local file with the same name as the requested file

8. `Shutdown`:
    * Once the file transfer is complete, the server closes the data socket
    * The server sends a `226` (Closing data connection) message on the control connection
    * The client sends the `QUIT` command
    * Both sockets are closed

## Usage

```bash
./download "ftp://[<user>:<password>@]<host>/<url-path>" 
```

### Example (Anonymous Login):
```bash
./download "fttp://ftp.up.pt/pub/kodi/timestamp.txt"
```

![](./doc/successful_download_1.png)

### Example (User Login):
```bash
./download "ftp://rcom:rcom@netlab1.fe.up.pt/files/pic1.jpg"
```

![](./doc/successful_download_2.png)

