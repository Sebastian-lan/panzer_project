#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define BUFFER_SIZE 4096

typedef struct{
        char *memoryBlockPointer;
        size_t reservedMemory;
        size_t usedMemory;
    } Acumulator;
/*
 * Resuelve el host y devuelve un socket ya conectado.
 * Devuelve el file descriptor del socket si conecta bien,
 * o -1 si ninguna dirección funcionó.
 *
 * Recibe el resultado de getaddrinfo por referencia (res_out)
 * porque quien llama a esta función es responsable de liberarlo
 * después con freeaddrinfo(), una vez que ya no lo necesite.
 */
typedef struct{
    char *host;
    char *port;
    char *path;
} RedirectUrl;

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
 * Arma y manda una petición GET simple sobre el socket ya conectado.
 * Devuelve 0 si se mandó todo bien, -1 si hubo error.
 */
int send_get_request(int sock, const char *host, const char *path) {
    char request[BUFFER_SIZE];
    int written;
    ssize_t sent;

    /*
     * snprintf arma el string igual que printf, pero en vez de
     * imprimirlo en pantalla lo escribe en el buffer que le pasás.
     * El segundo argumento (sizeof(request)) evita que se pueda
     * desbordar el buffer aunque host/path sean muy largos.
     */
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

    sent = send(sock, request, strlen(request), 0);
    if (sent == -1) {
        perror("Error en send");
        return -1;
    }

    return 0;
}

/*
 * Lee la respuesta del socket en un loop, imprimiendo cada
 * pedazo a medida que llega.
 */
Acumulator receive_response(int sock) {
    Acumulator error_result = {NULL, 0, 0};
    Acumulator acumulator_var;
    acumulator_var.memoryBlockPointer = malloc(4096);
    
    if(acumulator_var.memoryBlockPointer == NULL){
        printf("ERROR DE MALLOC\n");
        return error_result;
    }
    acumulator_var.reservedMemory = 4096;
    acumulator_var.usedMemory = 0;
    ssize_t bytes_recibidos;

    do {
        if((acumulator_var.usedMemory + 4096) > acumulator_var.reservedMemory){
            acumulator_var.reservedMemory *= 2;
            char *temp = realloc(acumulator_var.memoryBlockPointer, acumulator_var.reservedMemory);
            if (temp == NULL) {
                printf("ERROR DE REALLOC");
                free(acumulator_var.memoryBlockPointer);
                return error_result;
            }else{
                acumulator_var.memoryBlockPointer = temp;
            }
        }
        bytes_recibidos = recv(sock, acumulator_var.memoryBlockPointer + acumulator_var.usedMemory, acumulator_var.reservedMemory - acumulator_var.usedMemory, 0);
        if(bytes_recibidos > 0){
        acumulator_var.usedMemory += bytes_recibidos;
        }
    }while (bytes_recibidos > 0);
    if(bytes_recibidos == -1){
        perror("Error de recv");
        free(acumulator_var.memoryBlockPointer);
        return error_result;
    }else{
        acumulator_var.memoryBlockPointer[acumulator_var.usedMemory] = '\0'; 
    }
    return acumulator_var;
}

RedirectUrl parserRedirectURL(char *URLToParsear){
    RedirectUrl redirect_var;
    int longitudHost;
    RedirectUrl redirect_error;
    redirect_error.host = NULL;
    redirect_error.path = NULL;
    redirect_error.port = NULL;

    if (strncmp(URLToParsear, "https", 5) == 0) {
        redirect_var.port = "443";
    }else {
        redirect_var.port = "80";
    }

    char *startHost = strstr(URLToParsear, "://");
    if (startHost == NULL) {
        printf("ERROR EN REDIRECCION");
        return redirect_error;
    }
    startHost = startHost + strlen("://");
    char *startPath = strstr(startHost, "/");
    if (startPath == NULL) {
        redirect_var.path = malloc(2);
        strcpy(redirect_var.path, "/");
        longitudHost = strlen(startHost);
    }else{
        longitudHost = startPath - startHost;
        int longitudPath = strlen(startPath);
        redirect_var.path = malloc(longitudPath + 1);
        if (redirect_var.path == NULL) {
            printf("ERROR DE MALLOC EN PARSER\n");
            return redirect_error;
        }else{
            strcpy(redirect_var.path, startPath);
        }
    }
    redirect_var.host = malloc(longitudHost + 1);
    if (redirect_var.host == NULL) {
        printf("ERROR DE MALLOC EN PARSER");
        free(redirect_var.path);
        return redirect_error;
    }
    memcpy(redirect_var.host, startHost, longitudHost);
    redirect_var.host[longitudHost] = '\0';

    return redirect_var;
}

char *getNewUrl(char *respuesta, int *error_code){
    char *newURL;
    char *startURL = strstr(respuesta, "Location: ");
    if (startURL == NULL) {
        *error_code = 1;
        return NULL;
    }
    startURL = startURL + strlen("Location: ");
    char *endURL = strchr(startURL, '\r');
    if (endURL == NULL) {
        *error_code = 2;
        return  NULL;
    }
    int longitud = endURL - startURL; 
    newURL = malloc(longitud + 1);
    if (newURL == NULL) {
        *error_code = 3;
        return NULL;
    }
    memcpy(newURL, startURL, longitud);
    newURL[longitud ] = '\0';
    return newURL;
}

int get_status_code(char *resultado){
    char *startStatusCode = strstr(resultado, "HTTP/1.1 ");
    if(startStatusCode == NULL){
        printf("Error al encontrar el status code");
        return -1;
    }
    startStatusCode = startStatusCode + strlen("HTTP/1.1 ");
    return (atoi(startStatusCode)/100);
}
int main (){
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
        sock = connect_to_host(host, port, &res);
        if (sock == -1) {
            return -1;
        }
        freeaddrinfo(res);
        logNumber = send_get_request(sock, host, path);
        if (logNumber == -1) {
            return -1;
        }
        resultado = receive_response(sock);
        if (resultado.memoryBlockPointer == NULL) {
            return -1;
        }
        close(sock);
        int status_code = get_status_code(resultado.memoryBlockPointer);
        switch (status_code) {
            case 2:{
                terminado = 0;
                printf("exito");
                break;
            }
            case 3:{
                char *new_url = getNewUrl(resultado.memoryBlockPointer, &error_code);
                if (new_url == NULL) {
                    fprintf(stderr, "ERROR tipo %d al obtener la URL de redireccion\n", error_code);
                    break;
                }
                free(resultado.memoryBlockPointer);
                RedirectUrl parsed = parserRedirectURL(new_url);
                if (parsed.host == NULL) {
                    free(new_url);
                    free(host);
                    free(path);
                    return -1;
                }           
                free(new_url);
                free(host);
                free(path);
                host = parsed.host;
                path = parsed.path;
                port = parsed.port;
                redirect_count += 1;
                terminado = 1;
                break;
            }
            case 4:{
                printf("ERROR codigo 4xx no soportado\n");
                free(path);
                free(host);
                free(resultado.memoryBlockPointer);
                return -1;
            }
            case 5:{
                printf("ERROR codigo 5xx no soportado\n");
                free(path);
                free(host);
                free(resultado.memoryBlockPointer);
                return -1;
            }
            default: 
                printf("ERROR codigo no valido\n");
                free(path);
                free(host);
                free(resultado.memoryBlockPointer);
                return -1;
        }

    }while (terminado != 0 && redirect_count < 20);

    printf("------resultado--------");
    printf("%s", resultado.memoryBlockPointer);
    free(resultado.memoryBlockPointer);
    free(path);
    free(host);

    return 0;
}