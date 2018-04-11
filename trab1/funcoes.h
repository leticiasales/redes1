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
#define ERRO3 3 // arquivo inexistente
#define ERRO4 4 // tamanho do arquivo é gigante
#define ERRO5 5 // mensagem nao entregue
#define ERRO6 6 // Não foi possível criar o arquivo
#define ERRO7 7 // Não foi possível ler o arquivo
#define ERRO8 8 // Erro na leitura do arquivo
#define ERRO9 9 // Erro ao abrir a pasta
#define TIMEOUT 5 // tempo do timeout
#define DADOS 0x0 // 000000 = 0
#define CD 0x1 // 000001  = 1
#define LCD 0x06 //000110 cd local = 6
#define LS 0x2 //000010 = 2
#define LLS 0x7 //000111 //ls local = 7
#define TELA 0x3 //000011 = 3
#define GET 0x4 //000100 = 4
#define TAMANHO 0x5 //000101 = 5
#define PUT 0x8 //001000 = 8
#define SUCESSO 0x10 //010000 = 16
#define FIM 0x21 //100001 = 33
#define NACK 0x20 //100000 = 32
#define ACK 0x30 //110000 = 48
#define ERRO 0x3f //111111 = 63
#define ECHO 0x3e //111110 = 62 // comando adicional para echo
#define EXIT 0x3c //111100 =60 // comando adicional para finalizar o programa
#define INICIO 0x7E // 01111110 = 126 // delimitador de inicio

int abrirRawSocket(char *interface);

char converte_comando(char comando[512]);

void criaMensagem(uint8_t *mensagem, uint8_t *buffer, uint8_t tamanho, uint8_t sequencia, uint8_t tipo);

int timeout( int rsocket );

void espera_timeout (int rsocket, uint8_t *dados, int tamanho);

void get (int rsocket, uint8_t *dados);

void put (int rsocket, uint8_t *dados);

void get_server (int rsocket, uint8_t *dados, uint8_t *recebido, uint8_t tipo, uint8_t tamanho, uint8_t paridade);

void put_server (int rsocket, uint8_t *dados, uint8_t *recebido, uint8_t tipo, uint8_t tamanho, uint8_t paridade);


