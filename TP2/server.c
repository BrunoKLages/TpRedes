#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

#define BUFSZ 1024                    // tamanho máximo de buffer
#define PI 3.141592                   // valor de pi para calculo da distância
#define SERV_LAT -19.938936747175777  // latitude do server
#define SERV_LONGT -43.9264307661445  // longitude do server
#define VEL 400                       // velocidade de atualização do motorista

typedef struct Coordinate {  //// struct de coordenada
    double latitude;
    double longitude;
} Coordinate;

void usage(int agrc, char **argv) {  // tratamento de argumentos da linha de comando
    printf("usage:%s <cv4|v6> <server port>\n", argv[0]);
    printf("example:%s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

double haversine(double lat1, double lon1,
                 double lat2, double lon2) {  // funcao do calculo de harvesine para calculo de menor distancia

    double dLat = (lat2 - lat1) * PI / 180.0;
    double dLon = (lon2 - lon1) * PI / 180.0;  // distancia entre latitudes e longitudes

    lat1 = (lat1)*PI / 180.0;
    lat2 = (lat2)*PI / 180.0;  // converter para radianos

    double a = pow(sin(dLat / 2), 2) + pow(sin(dLon / 2), 2) * cos(lat1) * cos(lat2);
    double rad = 6371;
    double c = 2 * asin(sqrt(a));  // aplicar formula
    return rad * c * 1000;         // retorna em metros
}

Coordinate stringToCoordinate(char *buf) {  // converte string recebida para coordenadas
    char cLat[BUFSZ];
    char cLongt[BUFSZ];

    buf = strtok(buf, "/");
    strcpy(cLat, buf);
    buf = strtok(NULL, "/");
    strcpy(cLongt, buf);  // extrai a latitude e longitude

    char **eptr = 0;
    char **eptr1 = 0;
    double clienteLat = strtod(cLat, eptr);
    double clienteLongt = strtod(cLongt, eptr1);          // converte para double,
    Coordinate coordclient = {clienteLat, clienteLongt};  // retorna coordenada
    return coordclient;
};

int main(int argc, char **argv) {
    char buf[BUFSZ];          // buffer genérico
    char caddrstr[BUFSZ];     // endereco do cliente
    char confMsg[BUFSZ];      // mensagem de confirmação para enviar ao cliente
    char coordclient[BUFSZ];  // coordenada do cliente
    char strDist[BUFSZ];      // distancia do cliente no formato string
    char msg[BUFSZ];          // mensagem de confirmação
    char msgError[BUFSZ];     // mensagem para tratamento de erro

    memset(buf, 0, BUFSZ);
    memset(caddrstr, 0, BUFSZ);
    memset(confMsg, 0, BUFSZ);
    memset(coordclient, 0, BUFSZ);
    memset(strDist, 0, BUFSZ);
    memset(msg, 0, BUFSZ);
    memset(msgError, 0, BUFSZ);  // limpa buffers

    if (argc < 3) {         // confere se qtd de parametros são suficientes
        usage(argc, argv);  // tratamento de erro
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) {  // confere parametros
        usage(argc, argv);                                        // tratamento de erro
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);  // define protocolo ao socket (IPV4 ou IPV6)
    if (s == -1) {                                  // tratamento de erro
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {  // inicializa socket
        logexit("setsockopt");                                                 // tratamento de erro
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);  // cria endereço para o socket
    if (0 != bind(s, addr, sizeof(storage))) {              // atribui endereço ao socket
        logexit("failed");                                  // tratamento de erro
    }

    if (0 != listen(s, 10)) {  // confere se socket esta apto a receber requisicoes
        logexit("listen");     // tratamento de erro
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connection\n", addrstr);  // Imprime o endereço do server

    while (1) {  // servidor funcionando

        printf("----------------------------------------------------\n");
        printf("Aguardando solicitação.\n");
        printf("----------------------------------------------------\n");  // interface

        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t cdaddrlen = sizeof(cstorage);  // cria struct para receber endereço do cliente futuro

        int csock = accept(s, caddr, &cdaddrlen);  // aguarda requisição de conexão
        if (csock == -1) {                         // tratamento de erro
            logexit("accept");
        }

        addrtostr(caddr, caddrstr, BUFSZ);
        printf("[log] connection from %s\n", caddrstr);  // imprime endereço do cliente conectado

        recv(csock, coordclient, BUFSZ, 0);                        // recebe coordenadas do cliente
        Coordinate coordClient = stringToCoordinate(coordclient);  // cria struct da localização do cliente
        Coordinate coordServ = {SERV_LAT, SERV_LONGT};             // cria struct da localização do servidor

        int dist = haversine(coordServ.latitude, coordServ.longitude,
                             coordClient.latitude, coordClient.longitude);  // função que calcula distância

        printf("----------------------------------------------------\n");
        printf("Corrida Disponível: %d metros\n0 - Recusar\n1 - Aceitar\n", dist);
        printf("----------------------------------------------------\n");  // interface
        fgets(confMsg, BUFSZ - 1, stdin);                                  // inserir resposta à requisicao

        if (confMsg[0] == '1') {                                        // se motorista aceitar,
            sendMessage(csock, confMsg, "send positive confirmation");  // confirma positivo com cliente
            for (; dist > -VEL; dist -= VEL) {                          // loop que simula a movimentação do motorista,
                if (dist > 0) {                                         // caso motorista à caminho
                    sprintf(msg, "Motorista a %d metros", dist);        // configura mensagem a ser enviada
                    sprintf(msgError, "send distance");
                } else {                                    // caso motorista tenha chegado
                    sprintf(msg, "Seu motorista chegou!");  // configura mensagem a ser enviada
                    sprintf(msgError, "send done");
                }
                printf("%s\n", msg);
                sendMessage(csock, msg, msgError);  // envia mensagem para cliente
                sleep(2);                           // taxa de atualização da posição do motorista
            }
        } else {                                                    // se nao aceitar,
            sendMessage(csock, "0", "send negative confirmation");  // confirma negativo com cliente
        }
        close(csock);
    }

    exit(EXIT_SUCCESS);
}