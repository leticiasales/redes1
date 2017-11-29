#include "funcoes.h"

/* Referências:
- http://www.minek.com/files/unix_examples/poll.html



*/
int abrirRawSocket(char *interface){
	int rsocket;
	struct packet_mreq mreq;
	struct ifreq ifr; // 
	struct sockaddr_ll local;

	/* Abre/cria o socket */
	if((rsocket = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) < 0){ // AF_PACKET = low level packet interface; 
								      // SOCK_RAW = packets are passed to and from the device driver without any changes in the packet data.
		return -1;
	}
	/* Tenta encontrar a interface */
	/* struct ifreq {
	    char ifr_name[IFNAMSIZ];         // Interface name
	    union {
		struct sockaddr ifr_addr;    // Interface address (we use this)
		struct sockaddr ifr_dstaddr;
		struct sockaddr ifr_broadaddr;
		struct sockaddr ifr_netmask;
		struct sockaddr ifr_hwaddr;
		short ifr_flags;
		int ifr_ifindex;
		int ifr_metric;
		int ifr_mtu;
		struct ifmap ifr_map;
		char ifr_slave[IFNAMSIZ];
		char ifr_newname[IFNAMSIZ];
		char *ifr_data;
	    };
	};

	SOCK_RAW packets are passed to and from the device driver without any changes in the packet data. When receiving a packet, the address
	is still parsed and passed in a standard sockaddr_ll address structure. When transmitting a packet, the user supplied buffer should
	contain the physical layer header. That packet is then queued unmodified to the network driver of the interface defined by the
	destination address. Some device drivers always add other headers. SOCK_RAW is similar to but not compatible with the obsolete PF_INET/
	SOCK_PACKET of Linux 2.0.
	*/

	memset(&ifr, 0, sizeof(ifr)); // "aloca" um espaço para os dados da interface
	strcpy(ifr.ifr_name, interface); // copia o nome da interface em ifr.ifr_name
	if(ioctl(rsocket, SIOCGIFINDEX, &ifr) < 0){ // o indice da interface deve ter sido retornado em ifr.ifr_ifindex
		return -2;
	}

	/* 
	"sockaddr_ll is a device independent physical layer address."
	struct sockaddr_ll {
	     unsigned short sll_family;	  // Always AF_PACKET 
	     unsigned short sll_protocol; // Physical layer protocol 
	     int	    sll_ifindex;  // Interface number
	     unsigned short sll_hatype;	  // Header type
	     unsigned char  sll_pkttype;  // Packet type
	     unsigned char  sll_halen;	  // Length of address
	     unsigned char  sll_addr[8];  // Physical layer address
	 }; */

	/* Liga o socket a interface */
	memset(&local, 0, sizeof(local)); // "aloca" um espaço para o 'local'
	local.sll_family = AF_PACKET;
	local.sll_ifindex = ifr.ifr_ifindex;
	local.sll_protocol = htons(ETH_P_ALL);
	if(bind(rsocket, (struct sockaddr *) &local, sizeof(local)) < 0){
        	return -3;
	}

	/* Liga o modo promiscuo (envia mensagem para todos na rede) */
	memset(&mreq, 0, sizeof(mreq));
	mreq.mr_ifindex = ifr.ifr_ifindex;
	mreq.mr_type = PACKET_MR_PROMISC;
	if(setsockopt(rsocket, SOL_PACKET, PACKET_ADD_MEMBERSHIP,&mreq, sizeof(mreq)) < 0){
		return -4;
	}
	return rsocket;
}

void criaMensagem(uint8_t *mensagem, uint8_t *buffer, uint8_t tamanho, uint8_t sequencia, uint8_t tipo) {
/*	Protocolo:

	Delimitador de inicio = 8 bits; 1
	Tamanho = 6 bits; 1
	Sequencia = 4 bits;
	Tipo = 6 bits; 1
	Mensagem = tamanho bits;
	Paridade = 8 bits; 1
*/
	buffer[0] = INICIO; // delimitador - 8 bits (não é necessário deslocar) 8


	buffer[1] = tamanho << 2; // tamanho - 6 bits (desloca-se para a esquerda 2 bits); 14
	buffer[1] |= sequencia >> 2; // tamanho - 4 bits 18

	buffer[2] = sequencia << 6;
	buffer[2] |= tipo;

	int i;
	for(i=0; i<tamanho; i++)
		buffer[3+i] = mensagem[i];

	int j;
	i = tamanho + 3;
	buffer[i]=0; //paridade vertical
	for (j=1; j<i; j++) {
		buffer[i] ^= buffer[j];
	}

}

char converte_comando(char comando[512]) {
	if(strcmp(comando,"cd")==0){ // comando local
		return LCD;
	}
	else if(strcmp(comando,"ls")==0) // comando local
		return LLS;
	else if(strcmp(comando,"get")==0)
		return GET;
	else if(strcmp(comando,"put")==0)
		return PUT;
	else if(strcmp(comando,"lsr")==0)
		return LS;
	else if(strcmp(comando,"cdr")==0)
		return CD;
	else if(strcmp(comando,"echo")==0)
		return ECHO;
	else if(strcmp(comando,"exit")==0)
		return EXIT;
	else 
		return ERRO;
}


int timeout( int rsocket ) {
	struct timeval inicio, fim;
	int dif=0, y;
	uint8_t recebido[67], tamanho, paridade, tipo;
	gettimeofday(&inicio, NULL);
	while(dif < TIMEOUT) {
		recv(rsocket,recebido,67,MSG_DONTWAIT);
		if(recebido[0]==INICIO) {
			tamanho = 0;
			tamanho = recebido[1] >> 2;
			paridade = 0;
			for (y=1; y<3+tamanho; y++) {
				paridade ^= recebido[y];
			}
			if(paridade == recebido[y]) {
				tipo = 0;
				tipo = recebido[2] & 63; //
				if(tipo == ACK) {
					return 1;
				}
				else if (tipo == ERRO) {
					if(recebido[3] == ERRO2) 
						return 3;
					else if (recebido[3] == ERRO1)
						return 4;
					else if (recebido[3] == ERRO3)
						return 5;
					else if (recebido[3] == ERRO6)
						return 6;
					else if (recebido[3] == ERRO7)
						return 7;
					else if (recebido[3] == ERRO8)
						return 8;
					else if (recebido[3] == ERRO9)
						return 9;
				}
				else {
					return 2;
				}
			} 
		}
		gettimeofday(&fim, NULL);
		dif = fim.tv_sec - inicio.tv_sec;
	} 
	return 0;

}
void espera_timeout (int rsocket, uint8_t *dados, int tamanho) {
	int recebeu = 0;
	while (recebeu == 0) {
		int teste=timeout(rsocket); 
		if (teste==1) {
			recebeu=1;
		}
		else if (teste == 2) {
			printf("Erro ao enviar mensagem, reenviando mensagem.\n");
			write(rsocket, dados, tamanho); 
		}
		else if (teste == 3) {
			fprintf(stderr, "Permissão negada\n");
			recebeu = 1;
		}
		else if (teste == 4) {
			fprintf(stderr, "Não é um diretório\n");
			recebeu = 1;
		}
		else if (teste == 5) {
			fprintf(stderr, "Arquivo ou diretório não encontrado\n");
			recebeu = 1;
		}
		else if (teste == 6) {
			fprintf(stderr, "Não foi possível criar o arquivo no servidor\n");
			recebeu = 1;
		}
		else if (teste == 7) {
			fprintf(stderr, "Não foi possível ler o arquivo no servidor\n");
			recebeu = 1;
		}
		else if (teste == 8) {
			fprintf(stderr, "Não foi possível completar a leitura do arquivo no servidor\n");
			recebeu = 1;
		}
		else if (teste == 9) {
			fprintf(stderr, "Erro ao abrir a pasta\n");
			recebeu = 1;
		}
		else {
			printf("Timeout, reenviando mensagem.\n");
			write(rsocket, dados, tamanho); 	
		}
	}
}

int espera_timeout_cliente (int rsocket, uint8_t *dados, int tamanho) {
	int recebeu = 0;
	while (recebeu == 0) {
		int teste=timeout(rsocket); 
		if (teste==1) {
			recebeu=1;
		}
		else if (teste == 2) {
			printf("Erro ao enviar mensagem, reenviando mensagem.\n");
			write(rsocket, dados, tamanho); 
		}
		else if (teste == 3) {
			fprintf(stderr, "Permissão negada\n");
			recebeu = 1;
		}
		else if (teste == 4) {
			fprintf(stderr, "Não é um diretório\n");
			recebeu = 1;
		}
		else if (teste == 5) {
			fprintf(stderr, "Arquivo ou diretório não encontrado\n");
			recebeu = 1;
		}
		else if (teste == 6) {
			fprintf(stderr, "Não foi possível criar o arquivo no servidor\n");
			recebeu = 1;
		}
		else if (teste == 7) {
			fprintf(stderr, "Não foi possível ler o arquivo no servidor\n");
			return 0;
		}
		else if (teste == 8) {
			fprintf(stderr, "Não foi possível completar a leitura do arquivo no servidor\n");
			return 0;
		}
		else if (teste == 9) {
			fprintf(stderr, "Erro ao abrir pasta\n");
			return 0;
		}
		else {
			printf("Timeout, reenviando mensagem.\n");
			write(rsocket, dados, tamanho); 	
		}
	}
	return 1;
}

void get (int rsocket, uint8_t *dados){

	FILE *get;
	
	int flag_g;
	
	uint8_t recebido[67];
	
	int tamanho;
	
	uint8_t tamanho_arquivo[63];
	
	long tamanhoarq;
	
	int y, size;
	
	int paridade;
	
	uint8_t mensagem_erro[67];
	
	unsigned char buffer[67];
		
	uint8_t arquivo[63], tipo;

	bzero(arquivo,63);

	scanf(" %s",arquivo);

	//Envia nome do arquivo

	criaMensagem(arquivo, dados, strlen(arquivo), 0, GET);
	write(rsocket, dados, 67); 
	int test = espera_timeout_cliente(rsocket, dados,67); 

	if(test == 0) {
		return;
	}
	
	if (!(get = fopen (arquivo, "wb"))) {
		fprintf(stderr, "Não foi possível criar o arquivo\n");
		// mensagem de erro
		mensagem_erro[0] = ERRO6;
		criaMensagem(mensagem_erro, dados, 1, 0, ERRO);
		write(rsocket, dados, 67); 
		return;
	}

	flag_g=0;
	while(flag_g==0) { // pega tamanho do arquivo
		bzero(recebido,67);
		recv(rsocket,recebido,67,0);
		if(recebido[0]==INICIO) {
			tipo = recebido[2] & 63; //
			if(tipo==TAMANHO) {
				tamanho = 0;
				tamanho = recebido[1] >> 2;
				printf("%d\n",tamanho);
				paridade = 0;
				for (y=1; y<3+tamanho; y++) {
					paridade ^= recebido[y];
				}
				if (paridade == recebido[y]) { //
					criaMensagem(NULL, dados, 0, 0, ACK);
					write(rsocket, dados, 67); 
		
					bzero(tamanho_arquivo,63);
		
					for(y=0;y<tamanho; y++) {
						tamanho_arquivo[y] = recebido[3+y];
						printf("%c",tamanho_arquivo[y]);
					}
					tamanho_arquivo[y]="\0";
					tamanhoarq = atol(tamanho_arquivo);
					//verificar tamanho disponivel **
					printf("\nTamanho do arquivo: %ld\n",tamanhoarq);
					flag_g = 1;
				}	
				else {
					criaMensagem(NULL, dados, 0, 0, NACK);
					write(rsocket, dados, 67); 
				}
			}
		}
	}
	
	// recebe o arquivo
	recv(rsocket,recebido,67,0);
	while(1) {
		if(recebido[0]==INICIO) {
			tipo = recebido[2] & 63; //
			if(tipo==DADOS) {
				tipo = 0;
				tipo = recebido[2] & 63; //
				if(tipo == FIM)
				{
					criaMensagem(NULL, dados, 0, 0, ACK);
					write(rsocket, dados, 67);
					fclose(get);
					return;
				}
				tamanho = 0;
				tamanho = recebido[1] >> 2;
				paridade = 0;
				for (y=1; y<3+tamanho; y++) {
					paridade ^= recebido[y];
				}
				if (paridade == recebido[y]) {
					for(y=0;y<tamanho;y++) {
						buffer[y] = recebido[3+y];	
					}
					fwrite(buffer,1,tamanho,get);
					criaMensagem(NULL, dados, 0, 0, ACK);
					write(rsocket, dados, 67);
				}
				else {
					printf("Erro na paridade\n");
					criaMensagem(NULL, dados, 0, 0, NACK);
					write(rsocket, dados, 67);
				}
			}
		}
		recv(rsocket,recebido,67,0);
	}
}

void put (int rsocket, uint8_t *dados) {
	FILE *put;
	
	long tam;
	
	int y, size;
	
	uint8_t buffer[63], sequencia;
	
	uint8_t arquivo[63];
	
	scanf(" %s", arquivo);

	if((put=fopen(arquivo,"rb"))==NULL){
		fprintf(stderr, "Erro ao ler o arquivo.\n");
		return;
	}

	//Envia nome do arquivo

	criaMensagem(arquivo, dados, strlen(arquivo), 0, PUT);
	write(rsocket, dados, 67); 
	espera_timeout(rsocket, dados,67); 

	fseek(put, 0, SEEK_END);
	tam = ftell(put);
	fseek(put, 0, SEEK_SET);

	bzero(arquivo,63);

	sprintf(arquivo, "%ld\0", tam);

	//Envia o tamanho do arquivo

	criaMensagem(arquivo, dados, strlen(arquivo), 0, TAMANHO);
	write(rsocket, dados, 67); 
	espera_timeout(rsocket, dados,67); 
	
	y = 0;
	int x = 0;
	while(feof(put)==0){	
		x++;
		if((size = fread(buffer,sizeof(uint8_t),63,put)) != 63) {
			if(ferror(put)!=0) {
				fprintf(stderr,"Erro na leitura do arquivo\n");	
				return;
			}
			else if(feof(put)!=0);
		}
		
		criaMensagem(buffer, dados, size, y, DADOS);
		
		sequencia = 0;
		sequencia = dados[2] >> 6;
		sequencia |= dados[1] << 2;
		sequencia &= 15;
		
		write(rsocket, dados, 67);
		espera_timeout(rsocket, dados,67); 
		y = (++y)%16;
		bzero(buffer,63);
	}
	printf("FIM\n");
	
	fclose(put);
	
	criaMensagem(NULL, dados, 0, 0, FIM);
	write(rsocket, dados, 67); 
	espera_timeout(rsocket, dados,67); 					

}

void get_server (int rsocket, uint8_t *dados, uint8_t *recebido, uint8_t tipo, uint8_t tamanho, uint8_t paridade) {

	FILE *getr;
	
	uint8_t arquivo[63], buffer[63];
	
	int y, size;
	
	long tam;
	
	uint8_t mensagem_erro[67];
	
	bzero(arquivo,63);
	
	for(y=0; y<tamanho; y++) {
		arquivo[y] = recebido[3+y];
	}
	printf("%s\n",arquivo);
	if (!(getr = fopen (arquivo, "rb+"))) {
		fprintf(stderr, "Não foi possível ler o arquivo\n");
		// mensagem de erro
		mensagem_erro[0] = ERRO7;
		criaMensagem(mensagem_erro, dados, 1, 0, ERRO);
		write(rsocket, dados, 67); 
		return;
	}
	else {
		criaMensagem(NULL, dados, 0, 0, ACK);
		write(rsocket, dados, 67); 
		
		fseek(getr, 0, SEEK_END);
		tam = ftell(getr);
		fseek(getr, 0, SEEK_SET);

		bzero(arquivo,63);

		sprintf(arquivo, "%d", tam);

		//Envia o tamanho do arquivo

		criaMensagem((uint8_t *) arquivo, dados, strlen(arquivo), 0, TAMANHO);
		write(rsocket, dados, 67); 
		espera_timeout(rsocket, dados,67); 		
	}
	
	y = 0;

	while(feof(getr)==0){	
		if((size = fread(buffer,1,63,getr)) != 63) {
			if(ferror(getr)!=0) {
				fprintf(stderr,"Erro na leitura do arquivo\n");	
				mensagem_erro[0] = ERRO8;
				criaMensagem(mensagem_erro, dados, 1, 0, ERRO);
				write(rsocket, dados, 67); 
				return;
			}
		}
		criaMensagem(buffer, dados, size, y, DADOS);
		write(rsocket, dados, 67);
		espera_timeout(rsocket, dados,67); 
		y = (++y)%16;
		bzero(buffer,63);
	}

	criaMensagem(NULL, dados, 0, 0, FIM);
	write(rsocket, dados, 67); 
	espera_timeout(rsocket, dados,67); 					

	fclose(getr);
}

void put_server (int rsocket, uint8_t *dados, uint8_t *recebido, uint8_t tipo, uint8_t tamanho, uint8_t paridade) {

	FILE *rec;

	uint8_t tamanho_arquivo[63], arquivo[63], buffer[63];
	
	long tamanhoarq;
		
	uint8_t mensagem_erro[63], sequencia = 0, sequencia_old = 0;
	
	int y, flag_r;
	
	sequencia = recebido[2] >> 6;
	sequencia |= recebido[1] << 2;
	sequencia &= 15;
	
	printf("%d\n",sequencia); 
							
	bzero(arquivo,63);
	
	for(y=0; y<tamanho; y++) {
		arquivo[y] = recebido[3+y];
	}
	printf("Arquivo = %s\n",arquivo);
	if (!(rec = fopen (arquivo, "wb"))) {
		fprintf(stderr, "Não foi possível criar o arquivo\n");
		// mensagem de erro
		mensagem_erro[0] = ERRO6;
		criaMensagem(mensagem_erro, dados, 1, 0, ERRO);
		write(rsocket, dados, 67); 
		return;
	}
	else {
		criaMensagem(NULL, dados, 0, 0, ACK);
		write(rsocket, dados, 67); 
		flag_r = 0;
		while(flag_r==0) {
			recv(rsocket,recebido,67,0);
			if(recebido[0]==INICIO) {
				tipo = recebido[2] & 63; //
				if(tipo==TAMANHO) {
					tamanho = 0;
					tamanho = recebido[1] >> 2;
					paridade = 0;
					for (y=1; y<3+tamanho; y++) {
						paridade ^= recebido[y];
					}
					if (paridade == recebido[y]) { //
						criaMensagem(NULL, dados, 0, 0, ACK);
						send(rsocket, dados, 67, 0);
						//write(rsocket, dados, 67); 
						for(y=0;y<tamanho; y++) {
							tamanho_arquivo[y] = recebido[3+y];
						}
						tamanho_arquivo[y]="\0";
						tamanhoarq = atol(tamanho_arquivo);
						//verificar tamanho disponivel **
						printf("Tamanho do arquivo: %d bytes\n",tamanhoarq);
						flag_r = 1;
					}	
					else {
						criaMensagem(NULL, dados, 0, 0, NACK);
						write(rsocket, dados, 67); 
					}
				}
			}
		}

		// recebe o arquivo
		recv(rsocket,recebido,67,0);
		
		sequencia = 0;
		sequencia = recebido[2] >> 6;
		sequencia |= recebido[1] << 2;
		sequencia &= 15;
	
		while(1) {
			if(recebido[0]==INICIO) {
				tipo = 0;
				tipo = recebido[2] & 63; //
				if(tipo == FIM)
				{
					criaMensagem(NULL, dados, 0, 0, ACK);
					write(rsocket, dados, 67);
					fclose(rec);
					return;
				}
				else if (tipo == DADOS) 
				{
					tamanho = 0;
					tamanho = recebido[1] >> 2;
					paridade = 0;
					for (y=1; y<3+tamanho; y++) {
						paridade ^= recebido[y];
					}
					if (paridade == recebido[y]) {
						sequencia_old = sequencia;
						
						sequencia = 0;
						sequencia = recebido[2] >> 6;
						sequencia |= recebido[1] << 2;
						sequencia &= 15;
	
						if(sequencia_old == sequencia) {
							criaMensagem(NULL, dados, 0, 0, ACK);
							write(rsocket, dados, 67);
						}
						else {
							for(y=0;y<tamanho;y++) {
								buffer[y] = recebido[3+y];	
							}
							fwrite(buffer,sizeof(uint8_t),tamanho,rec);
							criaMensagem(NULL, dados, 0, 0, ACK);
							write(rsocket, dados, 67);
							//printf("%d\n", sequencia);
						}
					}
					else {
						printf("Erro na paridade\n");
						criaMensagem(NULL, dados, 0, 0, NACK);
						write(rsocket, dados, 67);
					}
				}
			}
			recv(rsocket,recebido,67,0);
		}
	}
}


void ls_server (int rsocket, uint8_t *dados, uint8_t *recebido, unsigned char *diretorioatual) {
	DIR *dir;
	struct dirent *dit;
	
	uint8_t mensagem[63];
	
	if((dir = opendir(diretorioatual)) == NULL)
	{
		criaMensagem(NULL, dados, 0, 0, ACK);
		write(rsocket, dados, 67);	
		fprintf(stderr,"Erro ao abrir o diretório");
	
	}
	
	
	criaMensagem(NULL, dados, 0, 0, ACK);
	write(rsocket, dados, 67);	
		
	int y = 0;
	
	while((dit = readdir(dir)) != NULL )
	{
		if(strcmp(dit->d_name,"..")==0 || strcmp(dit->d_name,".")==0) {
		
		}
		else {
	        	sprintf(mensagem, "%s\0", dit->d_name);
	        	printf("%s\n",mensagem);
			criaMensagem(mensagem, dados, strlen(mensagem), y, LS);
			write(rsocket, dados, 67); 
			espera_timeout(rsocket, dados,67); 
			bzero(mensagem,63);
			y++;
	        }
        }
        
        closedir(dir);
        printf("\n");
	criaMensagem(NULL, dados, 0, 0, FIM);
	write(rsocket, dados, 67);
	espera_timeout(rsocket, dados,67); 	
}

void ls (int rsocket, uint8_t *dados) {

	uint8_t tipo, tamanho, paridade, buffer[63];

	int y;
	
	uint8_t recebido[67];

	criaMensagem(NULL, dados, 0, 0, LS);
	write(rsocket, dados, 67);
	espera_timeout(rsocket, dados,67); 	
	
	while (1) {
		recv(rsocket,recebido,67,0);	
		if(recebido[0]==INICIO) {
			tipo = 0;
			tipo = recebido[2] & 63; //
			if(tipo == FIM)
			{		
				return;
			}
			else if (tipo == LS) 
			{		
				tamanho = 0;
				tamanho = recebido[1] >> 2;
				paridade = 0;
				for (y=1; y<3+tamanho; y++) {
					paridade ^= recebido[y];
				}
				if (paridade == recebido[y]) {
					for(y=0;y<tamanho;y++) {
						buffer[y] = recebido[3+y];	
					}	
					buffer[y]="\0";
					printf("%s  ",buffer);
					criaMensagem(NULL, dados, 0, 0, ACK);
					write(rsocket, dados, 67);		
				}
				else {
					criaMensagem(NULL, dados, 0, 0, NACK);
					write(rsocket, dados, 67);		
				}						
			}
		}
		printf("\n");
		bzero(buffer,63);
		bzero(dados,67);
	}
}
