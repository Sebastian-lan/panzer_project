#include "net.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

ssize_t plain_send(Connection *conn, const char *data, size_t len) {
    return send(conn->sock, data, len, 0);
}

ssize_t plain_recv(Connection *conn, char *buffer, size_t max_len) {
    return recv(conn->sock, buffer, max_len, 0);
}

ssize_t tls_send(Connection *conn, const char *data, size_t len) {
    int resultado = SSL_write(conn->ssl, data, len);
    if (resultado <= 0) {
        /* Simplificación: tratamos cualquier <= 0 como error, sin
         * distinguir los casos de "reintentar" que SSL_get_error()
         * podría señalar. Suficiente para pedidos simples. */
        return -1;
    }
    return resultado;
}

ssize_t tls_recv(Connection *conn, char *buffer, size_t max_len) {
    int resultado = SSL_read(conn->ssl, buffer, max_len);
    if (resultado < 0) {
        return -1; /* error real */
    }
    return resultado; /* 0 = fin normal de los datos, o bytes leídos */
}

/*
 * Establece el handshake TLS si usa_tls es 1, o deja la conexión en texto
 * plano si es 0. En ambos casos, asigna los punteros de función
 * correspondientes. Devuelve NULL si algo falla en cualquier paso.
 */
Connection *create_connect(int sock, int usa_tls, const char *host) {
    Connection *conn = malloc(sizeof(Connection));
    if (conn == NULL) {
        return NULL;
    }
    conn->sock = sock;

    if (usa_tls == 0) {
        conn->send_fn = plain_send;
        conn->recv_fn = plain_recv;
        conn->ssl = NULL;
        return conn;
    }

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (ctx == NULL) {
        free(conn);
        return NULL;
    }

    conn->ssl = SSL_new(ctx);
    if (conn->ssl == NULL) {
        SSL_CTX_free(ctx);
        free(conn);
        return NULL;
    }

    if (SSL_set_fd(conn->ssl, sock) != 1) {
        SSL_free(conn->ssl);
        SSL_CTX_free(ctx);
        free(conn);
        return NULL;
    }

    /* SNI: necesario para que el servidor sepa a qué dominio te querés
     * conectar durante el handshake, antes de que exista un header
     * Host: (que llega recién después, ya cifrado). Sin esto, muchos
     * servidores rechazan la conexión con "unrecognized name". */
    SSL_set_tlsext_host_name(conn->ssl, host);

    if (SSL_connect(conn->ssl) != 1) {
        SSL_free(conn->ssl);
        SSL_CTX_free(ctx);
        free(conn);
        return NULL;
    }

    conn->send_fn = tls_send;
    conn->recv_fn = tls_recv;

    /* ssl mantiene su propia referencia interna a ctx, así que podemos
     * liberarlo ahora sin esperar a que termine toda la conexión. */
    SSL_CTX_free(ctx);

    return conn;
}

void free_memory_conn(Connection *conn) {
    if (conn == NULL) {
        return;
    }
    SSL_free(conn->ssl); /* seguro incluso si conn->ssl es NULL */
    close(conn->sock);
    free(conn);
}

/*
 * Resuelve el host y devuelve un socket ya conectado.
 * Devuelve el file descriptor del socket si conecta bien,
 * o -1 si ninguna dirección funcionó.
 *
 * Recibe el resultado de getaddrinfo por referencia (res_out)
 * porque quien llama a esta función es responsable de liberarlo
 * después con freeaddrinfo(), una vez que ya no lo necesite.
 */
int connect_to_host(const char *host, const char *port, struct addrinfo **res_out) {
    struct addrinfo hints;
    struct addrinfo *p;
    int state;
    int sock;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    state = getaddrinfo(host, port, &hints, res_out);
    if (state != 0) {
        fprintf(stderr, "Error en getaddrinfo: %s\n", gai_strerror(state));
        return -1;
    }

    for (p = *res_out; p != NULL; p = p->ai_next) {
        sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sock == -1) {
            continue;
        }

        if (connect(sock, p->ai_addr, p->ai_addrlen) != -1) {
            /* Conexión exitosa con esta dirección */
            return sock;
        }

        close(sock);
    }

    /* Ninguna dirección de la lista funcionó */
    return -1;
}

/*
 * Arma y manda una petición GET simple sobre la conexión ya establecida.
 * Devuelve 0 si se mandó todo bien, -1 si hubo error.
 */
int send_get_request(Connection *conn, const char *host, const char *path) {
    char request[BUFFER_SIZE];
    int written;
    ssize_t sent;

    written = snprintf(request, sizeof(request),
                        "GET %s HTTP/1.1\r\n"
                        "Host: %s\r\n"
                        "Connection: close\r\n"
                        "\r\n",
                        path, host);

    if (written < 0 || (size_t)written >= sizeof(request)) {
        fprintf(stderr, "Error armando la peticion (demasiado larga para el buffer)\n");
        return -1;
    }

    sent = conn->send_fn(conn, request, strlen(request));
    if (sent == -1) {
        perror("Error en send");
        return -1;
    }

    return 0;
}

/*
 * Lee la respuesta completa en un loop, acumulando los pedazos que van
 * llegando en un bloque de memoria dinámica que crece según haga falta.
 */
Acumulator receive_response(Connection *conn) {
    Acumulator error_result = {NULL, 0, 0};
    Acumulator acumulator_var;
    ssize_t bytes_recibidos;

    acumulator_var.memoryBlockPointer = malloc(4096);
    if (acumulator_var.memoryBlockPointer == NULL) {
        fprintf(stderr, "Error de malloc en receive_response\n");
        return error_result;
    }
    acumulator_var.reservedMemory = 4096;
    acumulator_var.usedMemory = 0;

    do {
        if ((acumulator_var.usedMemory + 4096) > acumulator_var.reservedMemory) {
            acumulator_var.reservedMemory *= 2;
            char *temp = realloc(acumulator_var.memoryBlockPointer, acumulator_var.reservedMemory);
            if (temp == NULL) {
                fprintf(stderr, "Error de realloc en receive_response\n");
                free(acumulator_var.memoryBlockPointer);
                return error_result;
            }
            acumulator_var.memoryBlockPointer = temp;
        }

        bytes_recibidos = conn->recv_fn(conn, acumulator_var.memoryBlockPointer + acumulator_var.usedMemory,
                                         acumulator_var.reservedMemory - acumulator_var.usedMemory);
        if (bytes_recibidos > 0) {
            acumulator_var.usedMemory += bytes_recibidos;
        }
    } while (bytes_recibidos > 0);

    if (bytes_recibidos < 0) {
        fprintf(stderr, "Error en recv/SSL_read\n");
        free(acumulator_var.memoryBlockPointer);
        return error_result;
    }

    acumulator_var.memoryBlockPointer[acumulator_var.usedMemory] = '\0';
    return acumulator_var;
}

/*
 * Separa una URL completa (esquema://host/path) en sus 3 partes.
 * Devuelve una RedirectUrl con host == NULL si algo falla.
 */
RedirectUrl parser_redirect_url(char *URLToParsear) {
    RedirectUrl redirect_error = {NULL, NULL, NULL};
    RedirectUrl redirect_var;
    int longitudHost;

    redirect_var.port = (strncmp(URLToParsear, "https", 5) == 0) ? "443" : "80";

    char *startHost = strstr(URLToParsear, "://");
    if (startHost == NULL) {
        fprintf(stderr, "Error al parsear el esquema de la URL de redireccion\n");
        return redirect_error;
    }
    startHost = startHost + strlen("://");

    char *startPath = strstr(startHost, "/");
    if (startPath == NULL) {
        longitudHost = strlen(startHost);
        redirect_var.path = malloc(2);
        if (redirect_var.path == NULL) {
            fprintf(stderr, "Error de malloc en parserRedirectURL\n");
            return redirect_error;
        }
        strcpy(redirect_var.path, "/");
    } else {
        longitudHost = startPath - startHost;
        int longitudPath = strlen(startPath);
        redirect_var.path = malloc(longitudPath + 1);
        if (redirect_var.path == NULL) {
            fprintf(stderr, "Error de malloc en parserRedirectURL\n");
            return redirect_error;
        }
        strcpy(redirect_var.path, startPath);
    }

    redirect_var.host = malloc(longitudHost + 1);
    if (redirect_var.host == NULL) {
        fprintf(stderr, "Error de malloc en parserRedirectURL\n");
        free(redirect_var.path);
        return redirect_error;
    }
    memcpy(redirect_var.host, startHost, longitudHost);
    redirect_var.host[longitudHost] = '\0';

    return redirect_var;
}

/*
 * Busca el header Location: en una respuesta HTTP y devuelve su valor
 * como string propio (memoria dinámica). Devuelve NULL en error, con
 * el detalle en *error_code (1 = no se encontro Location, 2 = no se
 * encontro el fin de linea, 3 = fallo el malloc).
 */
char *get_new_url(char *respuesta, int *error_code) {
    char *startURL = strstr(respuesta, "Location: ");
    if (startURL == NULL) {
        *error_code = 1;
        return NULL;
    }
    startURL = startURL + strlen("Location: ");

    char *endURL = strchr(startURL, '\r');
    if (endURL == NULL) {
        *error_code = 2;
        return NULL;
    }

    int longitud = endURL - startURL;
    char *newURL = malloc(longitud + 1);
    if (newURL == NULL) {
        *error_code = 3;
        return NULL;
    }
    memcpy(newURL, startURL, longitud);
    newURL[longitud] = '\0';
    return newURL;
}

/* Devuelve la categoria del status code (2, 3, 4, 5...), o -1 si no se
 * pudo encontrar la linea de estado en la respuesta. */
int get_status_code(char *resultado) {
    char *startStatusCode = strstr(resultado, "HTTP/1.1 ");
    if (startStatusCode == NULL) {
        fprintf(stderr, "Error al encontrar el status code\n");
        return -1;
    }
    startStatusCode = startStatusCode + strlen("HTTP/1.1 ");
    return atoi(startStatusCode) / 100;
}