#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <netpacket/packet.h>
#include <inttypes.h>
#include <sys/select.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>

#define ERRO1 1 // diretório não existe
#define ERRO2 2 // permissao negada

#define TIMEOUT 5 // tempo do timeout
#define DADOS 0x0 // 000000 = 0
#define LCD 0x06 //000110 cd local = 6
#define LLS 0x7 //000111 //ls local = 7
#define SUCESSO 0x10 //010000 = 16

#define INICIO 0x7E // 01111110 = 126 // delimitador de inicio

#define ACK 0x0
#define TAMANHO 0x2
#define SUCESSO 0x3
#define CD 0x6
#define LS 0x7
#define GET 0x8
#define PUT 0x9
#define FIM 0xA
#define TELA 0xC
#define ERRO 0xE
#define NACK 0xF


int abrirRawSocket(char *interface);

char converte_comando(char comando[512]);

void criaMensagem(unsigned char *mensagem, unsigned char *buffer, unsigned char tamanho, unsigned char sequencia, unsigned char tipo);

int timeout( int rsocket );

void espera_timeout (int rsocket, unsigned char *dados, int tamanho);

void get (int rsocket, unsigned char *dados);

void put (int rsocket, unsigned char *dados);

void get_server (int rsocket, unsigned char *dados, unsigned char *recebido, unsigned char tipo, unsigned char tamanho, unsigned char paridade);

void put_server (int rsocket, unsigned char *dados, unsigned char *recebido, unsigned char tipo, unsigned char tamanho, unsigned char paridade);


