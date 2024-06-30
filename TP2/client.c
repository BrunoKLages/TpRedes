#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

#define BUFSZ 1024                       // tamanho maximo do buffer
#define CLIENT_LAT -19.948860462028136   // latitude do cliente
#define CLIENT_LONGT -43.92003247504384  // longitude do cliente

typedef struct Coordinate {  // struct de coordenada
    double latitude;
    double longitude;
} Coordinate;

void usage(int argc, char **argv) {  // tratamento de argumentos da linha de comando
    printf("usage:%s <server IP> <server port>\n", argv[0]);
    printf("example:%s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if (argc < 3) {         // confere se qtd de parametros são suficientes
        usage(argc, argv);  // tratamento de erro
    }

    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage)) {  // confere parametros
        usage(argc, argv);                             // tratamento de erro
    }

    int s = 0;           // armazena socket
    unsigned total = 0;  // contabiliza quantidade de bytes transmitidos

    char buf[BUFSZ];      // buffer que recebe dados inseridos
    char pck[BUFSZ];      // array de pacotes recebidos
    char coord[BUFSZ];    // coordenadas do cliente em string
    char addrstr[BUFSZ];  // endereço do server em string

    memset(buf, 0, BUFSZ);
    memset(pck, 0, BUFSZ);
    memset(coord, 0, BUFSZ);
    memset(addrstr, 0, BUFSZ);  // limpa buffers

    printf("----------------------------------------------------\n");  // interface

    while (1) {
        printf("0 - Sair\n1 - Solicitar corrida\n");
        printf("----------------------------------------------------\n");  // interface
        fgets(buf, 5, stdin);                                              // resposta do cliente

        if (buf[0] == '1') {                                // se cliente solicitou a corrida,
            s = socket(storage.ss_family, SOCK_STREAM, 0);  // cria socket com devido protocolo
            if (s == -1) {                                  // tratamento de erro
                logexit("socket");
            }
            struct sockaddr *addr = (struct sockaddr *)(&storage);  // define protocolo ao socket (IPV4 ou IPV6),
            if (0 != connect(s, addr, sizeof(storage))) {           // conecta ao server
                logexit("connect");                                 // tratamento de erro
            }
            snprintf(coord, BUFSZ,
                     "%.15f/%.15f/", CLIENT_LAT, CLIENT_LONGT);  // converte para string
            sendMessage(s, coord, "send");                       // envia coordenadas para servidor

            addrtostr(addr, addrstr, BUFSZ);
            printf("connected to %s\n", addrstr);                              // imprime endereço do server conectado
            printf("----------------------------------------------------\n");  // interface
            printf("Procurando motorista...\n");                    //
            printf("----------------------------------------------------\n");  // interface

            memset(buf, 0, BUFSZ);
            recv(s, buf, BUFSZ, 0);  // le confirmação do servidor quanto a solicitação

            if (buf[0] == '1') {                                 // se servidor confirmou a corrida,
                for (size_t i = 1; i > 0; total += i) {          // le resposta do servidor
                    i = recv(s, pck + total, BUFSZ - total, 0);  // recebe pacote a pacote
                    printf("%s\n", (pck + total));               // printa pacote recebido
                }
                printf("received %d bytes\n", total);
                exit(EXIT_SUCCESS);

                break;
            } else {                                          // se servidor recusou a corrida,
                printf("Não foi encontrado um motorista\n");  // interface
            }
        } else {    // se não solicitou corrida,
            break;  // finaliza programa
        }
    }
}