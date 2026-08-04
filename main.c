#define _POSIX_C_SOURCE 200809L

#include "core/dom/node.h"
#include "core/net/net.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(void) {
    char *host = malloc(256);
    strcpy(host, "archlinux.org");
    char *port = "80";
    char *path = malloc(256);
    strcpy(path, "/");

    struct addrinfo *res;
    int redirect_count = 0;
    int sock;
    Acumulator resultado;
    int logNumber;
    int terminado = 0;
    int error_code = 0;

    do {
        int usa_tls = (strcmp(port, "443") == 0) ? 1 : 0;

        sock = connect_to_host(host, port, &res);
        if (sock == -1) {
            freeaddrinfo(res);
            free(host);
            free(path);
            return -1;
        }

        Connection *conn = create_connect(sock, usa_tls, host);
        if (conn == NULL) {
            freeaddrinfo(res);
            close(sock);
            free(host);
            free(path);
            return -1;
        }
        freeaddrinfo(res);

        logNumber = send_get_request(conn, host, path);
        if (logNumber == -1) {
            free_memory_conn(conn);
            free(host);
            free(path);
            return -1;
        }

        resultado = receive_response(conn);
        free_memory_conn(conn);
        if (resultado.memoryBlockPointer == NULL) {
            free(host);
            free(path);
            return -1;
        }

        int status_code = get_status_code(resultado.memoryBlockPointer);
        switch (status_code) {
            case 2: {
                terminado = 1;
                printf("Exito\n");
                break;
            }
            case 3: {
                char *new_url = get_new_url(resultado.memoryBlockPointer, &error_code);
                if (new_url == NULL) {
                    fprintf(stderr, "ERROR tipo %d al obtener la URL de redireccion\n", error_code);
                    free(resultado.memoryBlockPointer);
                    free(host);
                    free(path);
                    return -1;
                }
                free(resultado.memoryBlockPointer);

                RedirectUrl parsed = parser_redirect_url(new_url);
                free(new_url);
                if (parsed.host == NULL) {
                    free(host);
                    free(path);
                    return -1;
                }

                free(host);
                free(path);
                host = parsed.host;
                path = parsed.path;
                port = parsed.port;
                redirect_count += 1;
                break;
            }
            case 4:
            case 5:
            default: {
                fprintf(stderr, "ERROR: status code categoria %d no soportada\n", status_code);
                free(resultado.memoryBlockPointer);
                free(host);
                free(path);
                return -1;
            }
        }

    } while (terminado == 0 && redirect_count < 20);

    printf("------resultado--------\n");
    printf("%s", resultado.memoryBlockPointer);

    free(resultado.memoryBlockPointer);
    free(path);
    free(host);

    return 0;
}