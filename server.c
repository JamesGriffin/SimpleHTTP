/*

An extremely basic HTTP server in C

https://github.com/JamesGriffin

*/

#include <netinet/in.h>    
#include <stdio.h>    
#include <stdlib.h>
#include <stdbool.h>
#include <string.h> 
#include <sys/socket.h>    
#include <sys/stat.h>    
#include <sys/types.h>    
#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>

// HTTP Request Data Structure
typedef struct {
    char *method;
    char *path;
} HTTPRequest;

// Parses raw HTTP request, returns HTTPRequest stucture
HTTPRequest parse_request(char *request) {
    HTTPRequest req;
    req.method = strtok(request, " ");
    req.path = strtok(NULL, " ");

    return req;
}

// Returns file descripter to requested resource
int get_resource(HTTPRequest req) {
    char *filename = req.path + sizeof(char);
    int fd = open(filename, O_RDONLY);

    return fd;
}

// Serves files in the current working directory
int main (void) {

    // Set up socket
    int create_socket, client_socket;
    struct sockaddr_in listen_address, client_address;
    socklen_t addrlen = sizeof(client_address);
    int buffer_size = 1024;
    char *buffer = malloc(buffer_size);    

    create_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (create_socket <= 0) {
        printf("Failed to create socket\n");
        exit(1);
    }

    // Bind to address
    listen_address.sin_family = AF_INET;
    listen_address.sin_addr.s_addr = INADDR_ANY;
    listen_address.sin_port = htons(8000);

    if (bind(create_socket, (struct sockaddr *) &listen_address, sizeof(listen_address)) != 0){    
      printf("Failed to bind to 0.0.0.0:8000\n");
      exit(2);
    }

    // Start listening
    listen(create_socket, 10);

    printf("Listening on 0.0.0.0:8000\n\n");
    
    // Server loop
    while (true) {
        //  Get HTTP request
        client_socket = accept(create_socket, (struct sockaddr *) &client_address, &addrlen);
        if (client_socket == -1) {
            printf("Couldn't accept incoming connection\n");
            continue;
        }
        
        recv(client_socket, buffer, buffer_size, 0);

        // Parse request
        HTTPRequest req = parse_request(buffer);

        // Log request
        printf("Method: %s, Path: %s\n", req.method, req.path);

        // Attempt to load resource
        int input_file = get_resource(req);

        size_t response_length;
        bool file_found = false;

        // If file exists, send file
        if(input_file > 0) {
            file_found = true;
            struct stat stat_buf;

            // Read file length
            fstat(input_file, &stat_buf);

            // Send header
            write(client_socket, "HTTP/1.0 200 OK\n\n", 17);
            // Send file
            sendfile(client_socket, input_file, NULL, stat_buf.st_size);

            close(input_file);
        }
        // Else, return 404
        else {
            write(client_socket, "HTTP/1.0 404 Not Found\n", 23);
            write(client_socket, "Content-Type: text/html\n\n", 25);
            write(client_socket, "<h1>Error 404</h1><p>Page not found.</p>", 40);
        }

        //  Close connection to browser
        close(client_socket);
 
    }
    // Exit
    close(create_socket);
    return 0;
}