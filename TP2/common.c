#include <arpa/inet.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void logexit(const char *msg)  // tratamento de erro
{
    perror(msg);         // printa mensagem de erro
    exit(EXIT_FAILURE);  // termina programa com status de falha
}

int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage) {  // configura socket a partir do endereço em string
    if (addrstr == NULL) {                         // trata erros nos parametros
        return -1;
    }

    uint16_t port = (uint16_t)atoi(portstr);  // atribui valor a porta
    if (port == 0) {                          // trata erro na conversão ou parametro
        return -1;
    }
    port = htons(port);  // cria host para a porta

    struct in_addr inaddr4;   // IPV4, 32 bits
    struct in6_addr inaddr6;  // IPV6, 128-bits

    if (inet_pton(AF_INET, addrstr, &inaddr4)) {                    // se for IPV4,
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;  // inicializa socket V4
        addr4->sin_family = AF_INET;                                // define familia, porta e endereço
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;  // retorna sucesso
    }
    if (inet_pton(AF_INET6, addrstr, &inaddr6)) {                     // se IPV6,
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;  // inicializa socket V6
        addr6->sin6_family = AF_INET6;                                // define familia, porta e endereço
        addr6->sin6_port = port;
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;  // retorna sucesso
    }

    return -1;  // se protocolo não corresponde aos suportados, retorna falha
}

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize) {  // converte endereço para string

    int version;                              // variavel da versão
    char addrstr[INET6_ADDRSTRLEN + 1] = "";  // string para endereço com tamanho máximo de V6, V4 é sempre menor
    uint16_t port;                            // armazena porta

    if (addr->sa_family == AF_INET) {                            // se IPV4,
        version = 4;                                             // define versão 4,
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;  // cria struct da versão do socket
        if (!inet_ntop(AF_INET, &addr4->sin_addr, addrstr,       // cria socket
                       INET6_ADDRSTRLEN + 1)) {                  // tratamento de erro
            logexit("ntop");
        }
        port = ntohs(addr4->sin_port);  // converte para host IPV4

    } else if (addr->sa_family == AF_INET6) {                      // se IPV6,
        version = 6;                                               // define versão 6,
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;  // cria struct da versão do socket
        if (!inet_ntop(AF_INET6, &addr6->sin6_addr, addrstr,       // cria socket
                       INET6_ADDRSTRLEN + 1)) {                    // tratamento de erro
            logexit("ntop");
        }
        port = ntohs(addr6->sin6_port);  // converte para host IPV6
    }

    else {
        logexit("unknown protocol family");  // tratamento de erro
    }
    if (str) {
        snprintf(str, strsize, "IPv%d %s %hu", version, addrstr, port);  // imprime endereço convertido
    }
}

int server_sockaddr_init(const char *proto, const char *portstr,
                         struct sockaddr_storage *storage) {  // inicializa socket a partir de endereço

    uint16_t port = (uint16_t)atoi(portstr);  // converte porta para unsigned short
    if (port == 0) {                          // tratamento de erro
        return -1;
    }
    port = htons(port);                    // converte endereço para host
    memset(storage, 0, sizeof(*storage));  // limpa storage destino

    if (0 == strcmp(proto, "v4")) {                                 // se protocolo for V4,
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;  // cria socket IPV4,
        addr4->sin_family = AF_INET;                                // define familia, endereço e porta
        addr4->sin_addr.s_addr = INADDR_ANY;
        addr4->sin_port = port;
        return 0;  // retorna sucesso

    } else if (0 == strcmp(proto, "v6")) {                            // se V6,
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;  // cria socket IPV6,
        addr6->sin6_family = AF_INET6;                                // define familia, endereço e porta
        addr6->sin6_addr = in6addr_any;
        addr6->sin6_port = port;
        return 0;  // retorna sucesso

    } else {        // se protocolo não corresponde aos suportados,
        return -1;  // retorna fracasso
    }
}

void sendMessage(int csock, char *buf, char *error) {     // envio de mensagem para outra ponta da conexão
    size_t count = send(csock, buf, strlen(buf) + 1, 0);  // envia mensagem de confirmação
    if (count != strlen(buf) + 1) {                       // tratameto de erro
        logexit(error);
    }
}