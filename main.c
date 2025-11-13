// Main file of the download application.

#include "download.h"

// Arguments:
//   $1: ftp://[<user>:<password>@]<host>/<url-path>
int main(int argc, char *argv[])
{
    if ((argc != 2) || (parseUrl(argv[1]) != 0))
    {
        printf("Usage: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv[0]);
        exit(-1);
    }
    
    printParsedUrl();
    
    if (downloadApp() == -1){
        printf("Error downloadApp()\n");
        exit(-1);
    }

    return 0;
}
