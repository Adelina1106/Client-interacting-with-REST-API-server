#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

#define HOST "34.241.4.235"
#define PORT 8080

int main(int argc, char *argv[])
{
    char *message;
    char *response;
    char *JWT_token = NULL;
    int sockfd, code;
    char input[LINELEN];
    char **cookies = malloc(1 * sizeof(char *));
    char aux[5000];
    char *all_books;
    char *book;

    while (1)
    {
        // citeste comanda cu comanda de la tastatura
        fgets(input, LINELEN, stdin);

        if (!strcmp(input, "register\n"))
        {
            // afiseaza si citeste de la tastatura numele si parola
            printf("username=");
            char user[LINELEN], pass[LINELEN];
            fgets(user, LINELEN, stdin);
            user[strlen(user) - 1] = '\0';
            printf("password=");
            fgets(pass, LINELEN, stdin);
            pass[strlen(pass) - 1] = '\0';

            // verific ca numele de utilizator si parola sa nu contina spatii
            char *space1 = strstr(user, " ");
            char *space2 = strstr(pass, " ");

            if (space1 == NULL && space2 == NULL)
            {

                // realizez conexiunea
                sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

                // parsez JSON mesajul ce trebuie trimis
                JSON_Value *value = json_value_init_object();
                JSON_Object *object = json_value_get_object(value);
                char *body = NULL;
                json_object_set_string(object, "username", user);
                json_object_set_string(object, "password", pass);
                body = json_serialize_to_string_pretty(value);
                json_value_free(value);

                char **body_data = (char **)malloc(1 * sizeof(char *));
                body_data[0] = malloc(LINELEN * sizeof(char));
                strcpy(body_data[0], body);

                // trimit o cerere de tip POST si prelucrez raspunsul primit de la server
                // pentru a prelua status code-ul
                message = compute_post_request(HOST, "/api/v1/tema/auth/register",
                                               "application/json", body_data, 1, NULL, 0, JWT_token);
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);
                sscanf(response, "%s %d %s", input, &code, aux);

                // afisez mesajul corespunzator pentru utilizator in functie de status code
                if (code == 400)
                {
                    printf("User already registered\n");
                }
                else if (code == 201)
                    printf("%d - OK - Successfully registered\n", code);
                else
                    printf("Error: %d\n", code);

                // eliberez memoria
                json_free_serialized_string(body);
                free(body_data[0]);
                free(body_data);
                close_connection(sockfd);
            }

            else
                printf("Wrong format: username and password should not contain spaces\n");
        }
        else if (!strcmp(input, "login\n"))
        {
            // afiseaza si citeste de la tastatura numele si parola
            printf("username=");
            char user[LINELEN], pass[LINELEN];
            fgets(user, LINELEN, stdin);
            user[strlen(user) - 1] = '\0';
            printf("password=");
            fgets(pass, LINELEN, stdin);
            pass[strlen(pass) - 1] = '\0';
            // cookies[0] retine cookie-ul de sesiune, acesta fiind NULL
            // in cazul in care alta sesiune nu este deja pornita
            if (cookies[0] == NULL)
            {
                sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

                // parsez JSON mesajul ce trebuie trimis
                JSON_Value *value = json_value_init_object();
                JSON_Object *object = json_value_get_object(value);
                char *body = NULL;
                json_object_set_string(object, "username", user);
                json_object_set_string(object, "password", pass);
                body = json_serialize_to_string_pretty(value);

                // compun mesajul ce va fi trimis
                char **body_data = (char **)malloc(1 * sizeof(char *));
                body_data[0] = malloc(LINELEN * sizeof(char));
                strcpy(body_data[0], body);

                // trimit o cerere de tip POST si prelucrez raspunsul primit de la server
                // pentru a prelua status code-ul
                message = compute_post_request(HOST, "/api/v1/tema/auth/login",
                                               "application/json", body_data, 1, NULL, 0, JWT_token);
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);
                sscanf(response, "%s %d %s", aux, &code, input);

                // AFisare in functie de status code
                if (code == 400)
                {
                    printf("Error: Wrong credentials\n");
                }
                else if (code != 200)
                {
                    printf("Error %d", code);
                }
                else
                {
                    printf("%d - OK - Login successfully\n", code);
                    cookies[0] = malloc(LINELEN * sizeof(char));
                    char *pos_cookie;
                    char *cookie;
                    pos_cookie = strstr(response, "connect.sid=");
                    strcpy(cookies[0], pos_cookie);
                    cookie = strtok(pos_cookie, ";");
                    strcpy(cookies[0], cookie);
                }

                // eliberez memoria
                json_free_serialized_string(body);
                json_value_free(value);
                free(body_data[0]);
                free(body_data);
                close_connection(sockfd);
            }
            else
                printf("Error: already logged in\n");
        }
        else if (!strcmp(input, "enter_library\n"))
        {
            if (JWT_token == NULL)
            {
                // daca exista un utilizator care e conectat
                if (cookies[0] != NULL)
                {
                    sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

                    // cerere GET
                    message = compute_get_request(HOST, "/api/v1/tema/library/access", NULL, cookies, 1, JWT_token);
                    send_to_server(sockfd, message);

                    // raspunsul de la server
                    response = receive_from_server(sockfd);
                    sscanf(response, "%s %d %s", aux, &code, input);
                    if (code != 200)
                        printf("Error: %d", code);
                    else
                    {
                        // prelucrez raspunsul de la server pentru a retine in JWT_token
                        // token-ul JWT primit
                        char *auxjwt_token = strdup(response);
                        char *aux_token = strstr(auxjwt_token, ":\"");
                        char *aux_jwt = malloc(LINELEN * sizeof(char));
                        strcpy(aux_jwt, aux_token + 2);
                        char *token = strtok(aux_jwt, "\"");
                        JWT_token = strdup(token);
                        printf("%d - OK - You have access to the library\n", code);
                        close_connection(sockfd);
                        free(aux_jwt);
                    }
                }
                else
                {
                    printf("You are not logged in. You can not access library!\n");
                }
            }
            else
                printf("You already have access to library\n");
        }
        else if (!strcmp(input, "get_books\n"))
        {

            // Daca JWT_token este diferit de NULL(adica s-a primit un token JWT, deci
            // exista acces la librarie), trimit cerere de tip GET si afisez raspunsul
            // primit in cazul in care status code-ul este cel bun
            if (JWT_token != NULL)
            {
                sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
                message = compute_get_request(HOST, "/api/v1/tema/library/books", NULL, cookies, 1, JWT_token);
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);
                int code;
                sscanf(response, "%s %d %s", aux, &code, input);
                if (code != 200)
                {
                    printf("Error: %d\n", code);
                }
                else
                {
                    all_books = strstr(response, "[");
                    printf("%s\n", all_books);
                    close_connection(sockfd);
                }
            }
            // nu exista acces la biblioteca
            else
            {
                printf("You don't have access to the library\n");
            }
        }
        else if (!strcmp(input, "add_book\n"))
        {

            // citesc de la tastatura datele despre carte
            printf("title=");
            char title[51], author[51], genre[51], publisher[51], page_count[51];
            fgets(title, 50, stdin);
            title[strlen(title) - 1] = '\0';
            printf("author=");
            fgets(author, 50, stdin);
            author[strlen(author) - 1] = '\0';

            printf("genre=");
            fgets(genre, 50, stdin);
            genre[strlen(genre) - 1] = '\0';

            printf("publisher=");
            fgets(publisher, 50, stdin);
            publisher[strlen(publisher) - 1] = '\0';

            printf("page_count=");
            fgets(page_count, 50, stdin);
            page_count[strlen(page_count) - 1] = '\0';

            // verific accesul la biblioteca
            if (JWT_token != NULL)
            {
                // verific daca datele date sunt valide
                char *space = strstr(page_count, " ");
                int count = 0;

                // verific daca in numarul de pagini exista alte caractere in afara de cifre
                for (int i = 0; i < strlen(page_count); i++)
                    if (page_count[i] < '0' || page_count[i] > '9')
                    {
                        count++;
                        break;
                    }

                // daca datele sunt valide
                if (space == NULL && count == 0)
                {
                    sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

                    // formatez JSON
                    JSON_Value *value = json_value_init_object();
                    JSON_Object *object = json_value_get_object(value);
                    char *body = NULL;
                    json_object_set_string(object, "title", title);
                    json_object_set_string(object, "author", author);
                    json_object_set_string(object, "genre", genre);
                    json_object_set_string(object, "publisher", publisher);
                    json_object_set_number(object, "page_count", atoi(page_count));
                    body = json_serialize_to_string_pretty(value);

                    // compun mesajul ce va fi trimis
                    char **body_data = (char **)malloc(1 * sizeof(char *));
                    body_data[0] = malloc(LINELEN * sizeof(char));
                    strcpy(body_data[0], body);

                    // cerere de tip POST
                    message = compute_post_request(HOST, "/api/v1/tema/library/books",
                                                   "application/json", body_data, 1, NULL, 0, JWT_token);
                    send_to_server(sockfd, message);
                    response = receive_from_server(sockfd);

                    // obtin status code-ul din raspuns
                    sscanf(response, "%s %d %s", aux, &code, input);

                    // afisez mesajul pentru utilizator
                    if (JWT_token == NULL)
                        printf("You do not have access to library!\n");
                    else if (code != 200)
                    {
                        printf("Error: %d\n", code);
                    }
                    else
                    {
                        printf("Book with title \"%s\" added successfully\n", title);
                    }

                    // eliberez memoria
                    json_free_serialized_string(body);
                    json_value_free(value);
                    free(body_data[0]);
                    free(body_data);
                    close_connection(sockfd);
                }
                else
                    printf("Wrong format\n");
            }
            else
                printf("You don't have access to library!\n");
        }
        else if (!strcmp(input, "get_book\n"))
        {

            // citesc id-ul
            printf("id=");
            char id[6];
            fgets(id, 6, stdin);
            id[strlen(id) - 1] = '\0';
            // exista un utilizator conectat care are acces la biblioteca
            if (JWT_token != NULL && cookies[0] != NULL)
            {
                // cerere de tip GET, preiau status code-ul si afisez mesajul corespunzator
                // pentru utilizator
                sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
                char buffer[100];
                sprintf(buffer, "/api/v1/tema/library/books/%s", id);
                message = compute_get_request(HOST, buffer, NULL, cookies, 1, JWT_token);
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);
                sscanf(response, "%s %d %s", aux, &code, input);
                if (code == 404)
                {
                    printf("The book with id=%s doesn't exist\n", id);
                }
                else if (code != 200)
                {
                    printf("Error: %d\n", code);
                }
                else
                {
                    book = strstr(response, "[");
                    printf("The book with id=%s is: %s\n", id, book);
                    close_connection(sockfd);
                }
            }
            else
            {
                printf("You don't have access to library!\n");
            }
        }
        else if (!strcmp(input, "delete_book\n"))
        {
            printf("id=");
            char id[6];
            fgets(id, 6, stdin);
            id[strlen(id) - 1] = '\0';

            // utilizator conectat si cu acces la librarie
            if (JWT_token != NULL && cookies[0] != NULL)
            {
                sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
                char buffer[100];
                sprintf(buffer, "/api/v1/tema/library/books/%s", id);
                message = compute_get_request(HOST, buffer, NULL, cookies, 1, JWT_token);

                // pentru cererea de tip DELETE, prelucrez o cerere GET, eliminand
                // primele 4 carectere("GET ") si inlocuindu-le cu "DELETE"
                message = message + 4;
                sprintf(buffer, "DELETE %s", message);
                send_to_server(sockfd, buffer);

                // preiau status code-ul si afisez mesajul
                //  corespunzator pentru utilizator
                response = receive_from_server(sockfd);
                sscanf(response, "%s %d %s", aux, &code, input);
                if (code == 404)
                {
                    printf("Id is invalid\n");
                }
                else if (code != 200)
                    printf("Error: %d", code);
                else
                {
                    printf("The book with id=%s has been succesfully deleted\n", id);
                    close_connection(sockfd);
                }
            }
            else
            {
                printf("You don't have access to library!\n");
            }
        }
        else if (!strcmp(input, "logout\n"))
        {
            // exista un utilizator conectat
            if (cookies[0] != NULL)
            {
                // eliberez memoria si setez JWT_token(accesul la biblioteca)
                // si cookies[0](utilizator conectat) la NULL, adica nu mai exista
                // utilizator conectat sau acces la biblioteca
                free(cookies[0]);
                cookies[0] = NULL;
                free(JWT_token);
                JWT_token = NULL;
                printf("Successfully logged out\n");
            }
            else
            {
                printf("You are not logged in\n");
            }
        }
        else if (!strcmp(input, "exit\n"))
        {
            // eliberez memoria
            if (cookies[0] != NULL)
                free(cookies[0]);
            if (JWT_token != NULL)
                free(JWT_token);
            break;
        }
        else
            printf("Invalid command\n");
    }
    free(cookies);

    return 0;
}
