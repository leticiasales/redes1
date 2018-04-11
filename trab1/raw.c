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
#include "funcoes.h"


void main(int argc, char *argv[]) {
	unsigned char interface[32], diretorioatual[1024],comando[512], pasta[63], cmdcd[512], buffer[63];
	int codigo, recebeu;
	char *usuario;
	int rsocket, y, size;
	int seleciona_interface=0, seleciona_modo=1;
	uint8_t dados[67];
	char arquivo[63];
	uint8_t mensagem_erro[63];
	unsigned char tamanho_arquivo[63];
	long tamanhoarq;
	uint8_t recebido[67];
	uint8_t tipo;
	char mensagem[63];
	long tam;
	uint8_t paridade;
	uint8_t tamanho;
	int flag_r, flag_g;
	fd_set condicao; // variável usada para checar a condição do socket (ready, writing, ou pending)
	/* Seleção da interface */

	/* Verifica entradas */

	/*----------------------*/



	/* Cria o socket e o liga a interface */

	rsocket = abrirRawSocket("eth0");

	/*----------------------*/

	/* Cria o terminal */

	if(seleciona_modo==0) { //é cliente
		
		/* CLIENTE */		
	
		getcwd(diretorioatual, sizeof(diretorioatual)); // diretorio atual
		usuario = getlogin();
		printf("%s:%s$ ", usuario,diretorioatual);

		while(1) {
			FD_ZERO(&condicao); //socket vazio
			recebeu = 0;
			scanf("%s",&comando);

			codigo = converte_comando(comando);

			switch(codigo) {
				case LCD:
					scanf("%s",&pasta);
					if(chdir(pasta) < 0){
					    switch(errno){
						case EACCES:
						    fprintf(stderr, "bash: cd: Permissão negada\n");
						    break;

						case ELOOP:
						    fprintf(stderr, "Symbolic link loop.\n");
						    break;

						case ENAMETOOLONG:
						    fprintf(stderr, "bash: cd: %s: Nome de arquivo muito longo\n", pasta);
						    break;

						case ENOTDIR:
						    fprintf(stderr, "bash: cd: %s: Não é um diretório\n", pasta);
						    break;

						case ENOENT:
						    fprintf(stderr, "bash: cd: %s: Arquivo ou diretório não encontrado\n", pasta);
					    }
					}
					getcwd(diretorioatual, sizeof(diretorioatual));
					break;
				case LLS:
					system("ls");
					break;
				case GET:
					printf("GET\n");
					
					get(rsocket,dados);

					bzero(dados,67);
					
					break;
				case PUT:
					printf("PUT\n");
					
					put(rsocket,dados);
					
					bzero(dados,67);
					
					break;

				case LS:
					printf("LS remoto\n");
					ls (rsocket, dados);
					break;
				case CD: 
					printf("CD remoto\n");
					scanf("%s",pasta);
					criaMensagem((uint8_t *) pasta, dados, strlen(pasta), 0, CD);
					write(rsocket, dados, 67); 
					espera_timeout(rsocket, dados,67);
					bzero(dados,67);
					bzero(pasta,63);
					break;
				case ECHO:
					printf("ECHO\n");
					scanf("%s",&mensagem);
					criaMensagem((uint8_t *) mensagem, dados, strlen(mensagem), 0, ECHO);
					//printf("Delimitador: %d\n",dados[0]);
					//printf("TAMANHO = %d\n", strlen(mensagem));
					printf("%d\n",dados[8]);
					write(rsocket, dados, 67); 
					espera_timeout(rsocket, dados,67);
					bzero(dados,67);
					bzero(mensagem,63);
					break;
				case EXIT:
					exit(0);
				default:
					printf("%s: comando não encontrado\n",comando);
			}
			printf("%s:%s$ ", usuario,diretorioatual);
		}

		/*------------------------*/

	}
	else { // é servidor

		/* SERVIDOR */

		getcwd(diretorioatual, sizeof(diretorioatual));
		while(listen(rsocket,2)) {
			FD_ZERO(&condicao); //socket vazio
			recv(rsocket,recebido,67,0);
			if(recebido[0]==INICIO) {
				tamanho = recebido[1] >> 2;
				paridade = 0;
				for (y=1; y<3+tamanho; y++) {
					paridade ^= recebido[y];
				}
				if(paridade == recebido[y]) 
				{
					tipo = recebido[2] & 63; //
					switch(tipo) {
						case GET:
							printf("GET\n");
							get_server(rsocket,dados,recebido,tipo,tamanho,paridade);
							
							tipo = 0;
							tamanho = 0;
							paridade = 0;
							bzero(recebido,67);
							bzero(dados,67);
							
							
							break;
						case PUT:
							printf("PUT\n");
							put_server(rsocket,dados,recebido,tipo,tamanho,paridade);
							
							tipo = 0;
							tamanho = 0;
							paridade = 0;
							bzero(recebido,67);
							bzero(dados,67);
							
							break;
						case LS:
							printf("LS remoto\n");
							ls_server (rsocket, dados, recebido, diretorioatual);
							getcwd(diretorioatual, sizeof(diretorioatual));
							break;
						case CD: 
							for(y=0;y<tamanho;y++) {
								pasta[y] = recebido[3+y];
							}
							pasta[y]='\0';
							printf("pasta = %s\n",pasta);
							bzero(dados,67);
							if(chdir(pasta) < 0){
							    switch(errno){
								case EACCES:
								 // permissao negada
								    mensagem_erro[0] = ERRO2;
								    criaMensagem(mensagem_erro, dados, 1, 0, ERRO);
								    write(rsocket, dados, 67); 
								    fprintf(stderr, "bash: cd remoto: Permissão negada\n");
								    break;

								case ENOTDIR: 
								 // diretorio nao existe
								    mensagem_erro[0] = ERRO1;
								    criaMensagem(mensagem_erro, dados, 1, 0, ERRO);
								    write(rsocket, dados, 67);
								    fprintf(stderr, "bash: cd remoto: %s: Não é um diretório\n", pasta);
								    break;

								case ENOENT:
								 // diretorio nao existe
								    mensagem_erro[0] = ERRO3;
								    criaMensagem(mensagem_erro, dados, 1, 0, ERRO);
								    write(rsocket, dados, 67);
								    fprintf(stderr, "bash: cd remoto: %s: Arquivo ou diretório não encontrado\n", pasta);
								    break;
							    }
							}
							else {
						        	criaMensagem(NULL, dados, 0, 0, ACK);
						        	write(rsocket, dados, 67); 
							}
							getcwd(diretorioatual, sizeof(diretorioatual));
							printf("Diretório atual: %s\n", diretorioatual);
							bzero(dados,67);
							bzero(pasta,63);
							bzero(mensagem_erro,63);
							break;
						case ECHO:
							criaMensagem(NULL, dados, 0, 0, ACK);
							write(rsocket, dados, 67); 
							printf("%s - %s: ",__DATE__,__TIME__);
							for(y=0; y<tamanho; y++)
							{
								printf("%c",recebido[3+y]);
							}		
							printf("\n");					
							break;
						case EXIT:
							exit(0);
						default:
							printf("%s: comando não encontrado\n",comando);
					}
				}
				else {
					criaMensagem(NULL, dados, 0, 0, NACK);
					write(rsocket, dados, 67); 	
				}
			}
		}

		/*---------------------*/

	}

	/*---------------------*/

}
