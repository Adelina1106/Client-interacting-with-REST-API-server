#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>   
#include <string.h>   
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h>   
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

char *compute_get_request(char *host, char *url, char *query_params,
                          char **cookies, int cookies_count, char *token_jwt)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    if (query_params != NULL)
    {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    }
    else
    {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // adaug header-ul authorization daca valoarea lui nu este NULL
    if (token_jwt != NULL)
    {
        memset(line, 0, LINELEN);
        sprintf(line, "Authorization: Bearer %s", token_jwt);
        compute_message(message, line);
    }

    // adaug host-ul
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);
    // adaug cookies
    if (cookies != NULL)
    {
        memset(line, 0, LINELEN);
        sprintf(line, "Cookie: ");

        for (int i = 0; i < cookies_count; i++)
            if (i == (cookies_count - 1))
            {
                strcat(line, cookies[i]);
            }
            else
            {
                strcat(line, cookies[i]);
                strcat(line, ";");
            }
    }
    compute_message(message, line);
    compute_message(message, "");
    return message;
}

char *compute_post_request(char *host, char *url, char *content_type, char **body_data,
                           int body_data_fields_count, char **cookies, int cookies_count, char *token_jwt)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *body_data_buffer = calloc(LINELEN, sizeof(char));
    ;

    // nume, URL si tipul protocolului
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

    // adaug header-ul authorization
    if (token_jwt != NULL)
    {
        memset(line, 0, LINELEN);
        sprintf(line, "Authorization: Bearer %s", token_jwt); // adauga header-ul authorization
        compute_message(message, line);
    }

    // adaug host-ul
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // adag tipul si lungimea
    int body_data_size = 0;
    for (int i = 0; i < body_data_fields_count - 1; i++)
    {
        strcat(body_data_buffer, body_data[i]);
        strcat(body_data_buffer, "&");
        body_data_size += strlen(body_data[i]);
        body_data_size++;
    }

    strcat(body_data_buffer, body_data[body_data_fields_count - 1]);
    body_data_size += strlen(body_data[body_data_fields_count - 1]);

    sprintf(line, "Content-Type: %s\r\nContent-Length: %d", content_type, body_data_size);
    compute_message(message, line);

    // adaug cookies
    if (cookies != NULL)
    {
        memset(line, 0, LINELEN);
        strcat(line, "Cookie: ");

        for (int i = 0; i < cookies_count; i++)
            if (i == (cookies_count - 1))
            {
                strcat(line, cookies[i]);
            }
            else
            {
                strcat(line, cookies[i]);
                strcat(line, ";");
            }
        compute_message(message, line);
    }
 
    compute_message(message, "");
    // adaug continutul
    memset(line, 0, LINELEN);
    compute_message(message, body_data_buffer);

    free(line);

    return message;
}
