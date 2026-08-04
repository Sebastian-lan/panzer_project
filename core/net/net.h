#ifndef NET_H
#define NET_H

#include <stddef.h>
#include <openssl/ssl.h>
#include <netdb.h>

/* Acumulador dinámico de bytes crudos (usado para juntar una respuesta
 * HTTP completa a medida que llega en pedazos por recv()/SSL_read()). */
typedef struct {
    char *memoryBlockPointer;
    size_t reservedMemory;
    size_t usedMemory;
} Acumulator;

/* Resultado de parsear una URL de redirección en sus 3 partes. */
typedef struct {
    char *host;
    char *port;
    char *path;
} RedirectUrl;

/* Abstracción de transporte: agrupa el socket y, opcionalmente, la sesión
 * TLS, junto con punteros a función que deciden si send/recv van cifrados
 * o en texto plano. El resto del programa llama siempre a conn->send_fn/
 * conn->recv_fn, sin necesidad de saber cuál de los dos casos es. */
typedef struct Connection {
    int sock;
    SSL *ssl;
    ssize_t (*send_fn)(struct Connection *conn, const char *data, size_t len);
    ssize_t (*recv_fn)(struct Connection *conn, char *buffer, size_t max_len);
} Connection;

ssize_t plain_send(Connection *conn, const char *data, size_t len);
ssize_t plain_recv(Connection *conn, char *buffer, size_t max_len);
ssize_t tls_send(Connection *conn, const char *data, size_t len);
ssize_t tls_recv(Connection *conn, char *buffer, size_t max_len);
Connection *create_connect(int sock, int usa_tls, const char *host);
void free_memory_conn(Connection *conn);
int connect_to_host(const char *host, const char *port, struct addrinfo **res_out);
int send_get_request(Connection *conn, const char *host, const char *path);
Acumulator receive_response(Connection *conn);
RedirectUrl parser_redirect_url(char *URLToParsear);
char *get_new_url(char *respuesta, int *error_code);
int get_status_code(char *resultado);
#endif