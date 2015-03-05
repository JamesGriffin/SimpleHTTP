/*

An extremely basic HTTP server in C

https://github.com/JamesGriffin

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <czmq.h>

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

// Returns file pointer to requested resource
FILE *get_resource(HTTPRequest req) {
    char *filename = req.path + sizeof(char);
    FILE *fp = fopen(filename, "rb");

    return fp;
}

// Serves files in the current working directory
int main (void)
{
    // Set up ZMQ
    zctx_t *ctx = zctx_new ();
    void *router = zsocket_new (ctx, ZMQ_ROUTER);
    zsocket_set_router_raw (router, 1);
    int rc = zsocket_bind (router, "tcp://127.0.0.1:8000");
    assert (rc != -1);

    printf("Listening on 127.0.0.1:8000\n\n");
    
    // Server loop
    while (true) {
        //  Get HTTP request
        zframe_t *handle = zframe_recv (router);
        if (!handle)
            exit(0);          //  Ctrl-C interrupt

        char *request = zstr_recv (router);
        HTTPRequest req = parse_request(request);

        // Log request
        printf("Method: %s, Path: %s\n", req.method, req.path);

        // Attempt to load resource
        FILE *input_file = get_resource(req);

        char *response;
        int response_length;
        bool file_found = false;

        // If file exists, read file
        if(input_file != NULL) {
            file_found = true;
            char *file_contents;
            long input_file_size;

            // Read file
            fseek(input_file, 0, SEEK_END);
            input_file_size = ftell(input_file);
            rewind(input_file);
            file_contents = malloc((input_file_size) * (sizeof(char)));
            fread(file_contents, sizeof(char), input_file_size, input_file);
            fclose(input_file);

            // Construct response
            char header[120400];
            char *filename = req.path + 1;

            sprintf(header, "HTTP/1.0 200 OK\r\n\r\n");
            response = (char *)malloc((input_file_size + strlen(header)) * (sizeof(char)));
            strcpy(response, header);
            memcpy(response+strlen(header), file_contents, input_file_size);
            response_length = input_file_size + strlen(header);

            free(file_contents);
        }
        // Else, return 404
        else {            
            response =
                "HTTP/1.0 404 Not Found\r\n"
                "Content-Type: text/html\r\n\r\n<h1>Error 404</h1><p>Page not found.</p>";
            response_length = strlen(response);
        }
  
        //  Send response
        zframe_send (&handle, router, ZFRAME_MORE + ZFRAME_REUSE);
        zmq_send (router, response, response_length, 0);
        //  Close connection to browser
        zframe_send (&handle, router, ZFRAME_MORE);
        zmq_send (router, NULL, 0, 0);

        // Clean up
        if(file_found)
            free(response);
        free(request);        
    }
    // Exit
    zctx_destroy (&ctx);
    return 0;
}