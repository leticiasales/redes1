/***************************************************************************
  by Leticia Sales and Matheus Lima
****************************************************************************/

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <inttypes.h>
#include <libgen.h>
#include <limits.h>
#include <linux/if.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
// #include <netpacket/packet.h>
#include <poll.h>
#include <pwd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>


#define LSH_RL_BUFSIZE 1024

const int s_init = 8;
const int s_size = 5;
const int s_seq = 6;
const int s_type = 5;
const int s_pair = 8;

const unsigned char init = 0x7E;

const unsigned char ack = 0x0;
const unsigned char size = 0x2;
const unsigned char ok = 0x3;
const unsigned char cd = 0x6;
const unsigned char ls = 0x7;
const unsigned char get = 0x8;
const unsigned char put = 0x9;
const unsigned char fim = 0xA;
const unsigned char tela = 0xC;
const unsigned char erro = 0xE;
const unsigned char nack = 0xF;


const int erro1 = 1; // diretório não existe
const int erro2 = 2; // permissao negada

int rsocket;
fd_set condicao; // variável usada para checar a condição do socket (ready, writing or pending)
unsigned char received[37];

typedef struct bloco bloco;

struct bloco
{
  unsigned char size;
  unsigned char seq;
  unsigned char type;
  unsigned char data[32];
};

unsigned int dec_to_bin(int n)
{
  int remainder, i = 1;
  unsigned int binary = 0;
  while(n != 0) {
    remainder = n%2;
    n = n/2;
    binary= binary + (remainder*i);
    i = i*10;
  }
  return binary;
}

unsigned char organizer(int left, int right, unsigned char byte)
{
  // printf("org: %d, %d, %u\n", left, right, dec_to_bin(byte));
  if(right == 0)
    return(byte>>(8-left));
  unsigned char tmp = byte<<(8-right);
  return tmp>>(8-right);
}

void packet(unsigned char ret[37], bloco msg)
{
  // printf("packet: %d, %d, %d, %d\n", dec_to_bin(size), dec_to_bin(seq), dec_to_bin(type), dec_to_bin(pair));
  int i = 0;
  ret[0] = init;
  ret[1] = msg.size << 3;
  ret[1] |= organizer(5, 0, msg.seq);
  ret[2] = organizer(0, 3, msg.seq);
  ret[2] |= organizer(0, 5, msg.type);
  for (; i < msg.size; ++i)
  {
    ret[3 + i] = msg.data[i];
  }
  ret[3 + i] = 0;
  ret[3 + i] |= ret[1];
  ret[3 + i] |= ret[2];
  for (int j = 0; j < msg.size; ++j)
  {
    ret[3 + i] |= msg.data[j];
  }
}

void topack()
{
  bloco meubloco;
  unsigned char tmp[37];

  meubloco.size = organizer(0,s_size,'g');
  meubloco.seq = organizer(0,s_seq,'h');
  meubloco.type = organizer(0,s_type,'i');
  strcpy(meubloco.data, "jota");

  packet(tmp, meubloco);

  printf("%08u.", dec_to_bin(tmp[0]));
  printf("%08u.", dec_to_bin(tmp[1]));
  printf("%08u.", dec_to_bin(tmp[2]));
  printf("%08u\n", dec_to_bin(tmp[3]));
}

/*
  Function Declarations for builtin shell commands:
 */

int cli_cd(char **args);
int cli_help(char **args);
int cli_ls(char **args);
int serv_cd(char **args);
int serv_ls(char **args);
int all_exit(char **args);

/*
  List of builtin commands, followed by their corresponding functions.
 */

char *builtin_str[] = {
  "cd",
  "help",
  "ls",
  "scd",
  "sls",
  "exit"
};

int (*builtin_func[]) (char **) = {
  &cli_cd,
  &cli_help,
  &cli_ls,
  &serv_cd,
  &serv_ls,
  &all_exit
};

int cli_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/

int serv_cd(char **args) {
  unsigned char msg[37];
  bloco dados;
  dados.size = 0;
  dados.seq = 0;
  dados.type = cd;
  if (args[1] != NULL)
  {
    dados.size = strlen(args[1]);
    strcpy(dados.data, args[1]);
  }
  //else dados.data = NULL;
  packet(msg, dados);
  send(rsocket, msg, dados.size + 4, 0);
  return 1;
}

int serv_ls(char **args) {
  unsigned char msg[37];
  bloco dados;
  dados.size = 0;
  dados.seq = 0x0;
  dados.type = ls;
  if (args[1] != NULL)
  {
    dados.size = strlen(args[1]);
    strcpy(dados.data, args[1]);
  }
  //else dados.data = NULL;
  packet(msg, dados);
  send(rsocket, msg, dados.size + 4, 0);
  return 1;
}

//It will print file Information in lines with File_permissions and XYZ..!!
void long_listing(char* fname)
{
  DIR *mydir;
  struct dirent *myfile;
  struct stat mystat;

  char buf[512];
  mydir = opendir(".");
  while((myfile = readdir(mydir)) != NULL)
  {
      sprintf(buf, "%s/%s", ".", myfile->d_name);
      stat(buf, &mystat);
      printf("%zu",mystat.st_size);
      printf(" %s\n", myfile->d_name);
  }
  closedir(mydir);
}

int cli_ls(char **args)
{
  char *curr_dir = NULL; 
  DIR *dp = NULL; 
  struct dirent *dptr = NULL; 
  unsigned int count = 0; 

  curr_dir = getcwd(curr_dir, LSH_RL_BUFSIZE); 
  if(NULL == curr_dir){return 1;} 
   
  dp = opendir((const char*)curr_dir);    
  if(NULL == dp){return 0;} 
 
  if (args[1] == NULL) {
    for(count = 0; NULL != (dptr = readdir(dp)); count++) 
    { 
        if(dptr->d_name[0] != '.') 
        { 
          printf("%16s", dptr->d_name);
        } 
    } 
    printf("\n");
  }
  else if (strcmp(args[1],"-l")==0) {
    long_listing(curr_dir);
  }
  else if (strcmp(args[1],"-a")==0)
  {
    for(count = 0; NULL != (dptr = readdir(dp)); count++) 
    { 
      printf("%16s", dptr->d_name);
    } 
    printf("\n");
  }
  return 1;
}

/**
   @brief Bultin command: change directory.
   @param args List of args.  args[0] is "cd".  args[1] is the directory.
   @return Always returns 1, to continue executing.
 */
int cli_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("Erro");
    }
  }
  return 1;
}

void cli_get (int rsocket, uint8_t *dados){

	FILE *get;
	
	int flag_g;
		
	int tamanho;
	
	uint8_t tamanho_arquivo[63];
	
	long tamanhoarq;
	
	int y, size;
	
	int paridade;
	
	uint8_t mensagem_erro[37];
	
	unsigned char buffer[37];
		
	uint8_t arquivo[31], tipo;

	bzero(arquivo,31);

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
		bzero(received,67);
		recv(rsocket,received,67,0);
		if(received[0]==INICIO) {
			tipo = received[2] & 63; //
			if(tipo==TAMANHO) {
				tamanho = 0;
				tamanho = received[1] >> 2;
				printf("%d\n",tamanho);
				paridade = 0;
				for (y=1; y<3+tamanho; y++) {
					paridade ^= received[y];
				}
				if (paridade == received[y]) { //
					criaMensagem(NULL, dados, 0, 0, ACK);
					write(rsocket, dados, 67); 
		
					bzero(tamanho_arquivo,63);
		
					for(y=0;y<tamanho; y++) {
						tamanho_arquivo[y] = received[3+y];
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
	recv(rsocket,received,67,0);
	while(1) {
		if(received[0]==INICIO) {
			tipo = received[2] & 63; //
			if(tipo==DADOS) {
				tipo = 0;
				tipo = received[2] & 63; //
				if(tipo == FIM)
				{
					criaMensagem(NULL, dados, 0, 0, ACK);
					write(rsocket, dados, 67);
					fclose(get);
					return;
				}
				tamanho = 0;
				tamanho = received[1] >> 2;
				paridade = 0;
				for (y=1; y<3+tamanho; y++) {
					paridade ^= received[y];
				}
				if (paridade == received[y]) {
					for(y=0;y<tamanho;y++) {
						buffer[y] = received[3+y];	
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
		recv(rsocket,received,67,0);
	}
}

void put (int rsocket, unsigned char *dados) {
	FILE *put;
	
	long tam;
	
	int y, size;
	
	unsigned char buffer[31], sequencia;
	
	unsigned char arquivo[31];
	
	scanf(" %s", arquivo);

	if((put=fopen(arquivo,"rb"))==NULL){
		fprintf(stderr, "Erro ao ler o arquivo.\n");
		return;
	}

	//Envia nome do arquivo

	criaMensagem(arquivo, dados, strlen(arquivo), 0, PUT);
	write(rsocket, dados, 37); 
	espera_timeout(rsocket, dados,37); 

	fseek(put, 0, SEEK_END);
	tam = ftell(put);
	fseek(put, 0, SEEK_SET);

	bzero(arquivo,63);

	sprintf(arquivo, "%ld\0", tam);

	//Envia o tamanho do arquivo

	criaMensagem(arquivo, dados, strlen(arquivo), 0, TAMANHO);
	write(rsocket, dados, 37); 
	espera_timeout(rsocket, dados,67); 
	
	y = 0;
	int x = 0;
	while(feof(put)==0){	
		x++;
		if((size = fread(buffer,sizeof(unsigned char),31,put)) != 31) {
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

void serv_get (int rsocket, bloco dados, unsigned char *received, unsigned char tipo, unsigned char tamanho, unsigned char paridade) {

	FILE *getr;
	
	unsigned char arquivo[31], buffer[31];
	
	int y, size;
	
	long tam;
	
	unsigned char mensagem_erro[67];
	
	bzero(arquivo,63);
	
	for(y=0; y<tamanho; y++) {
		arquivo[y] = received[3+y];
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

void serv_get (int rsocket, uint8_t *dados, uint8_t *received, uint8_t tipo, uint8_t tamanho, uint8_t paridade) {

	FILE *rec;

	uint8_t tamanho_arquivo[31], arquivo[31], buffer[31];
	
	long tamanhoarq;
		
	uint8_t mensagem_erro[31], sequencia = 0, sequencia_old = 0;
	
	int y, flag_r;
	
	sequencia = received[2] >> 6;
	sequencia |= received[1] << 2;
	sequencia &= 15;
	
	printf("%d\n",sequencia); 
							
	bzero(arquivo,31);
	
	for(y=0; y<tamanho; y++) {
		arquivo[y] = received[3+y];
	}
	printf("Arquivo = %s\n",arquivo);
	if (!(rec = fopen (arquivo, "wb"))) {
		fprintf(stderr, "Não foi possível criar o arquivo\n");
		// mensagem de erro
		mensagem_erro[0] = ERRO6;
		criaMensagem(mensagem_erro, dados, 1, 0, ERRO);
		write(rsocket, dados, 37); 
		return;
	}
	else {
		criaMensagem(NULL, dados, 0, 0, ACK);
		write(rsocket, dados, 37); 
		flag_r = 0;
		while(flag_r==0) {
			recv(rsocket,received,37,0);
			if(received[0]==INICIO) {
				tipo = received[2] & 31; //
				if(tipo==TAMANHO) {
					tamanho = 0;
					tamanho = received[1] >> 2;
					paridade = 0;
					for (y=1; y<3+tamanho; y++) {
						paridade ^= received[y];
					}
					if (paridade == received[y]) { //
						criaMensagem(NULL, dados, 0, 0, ACK);
						send(rsocket, dados, 67, 0);
						//write(rsocket, dados, 67); 
						for(y=0;y<tamanho; y++) {
							tamanho_arquivo[y] = received[3+y];
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
		recv(rsocket,received,67,0);
		
		sequencia = 0;
		sequencia = received[2] >> 6;
		sequencia |= received[1] << 2;
		sequencia &= 15;
	
		while(1) {
			if(received[0]==INICIO) {
				tipo = 0;
				tipo = received[2] & 63; //
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
					tamanho = received[1] >> 2;
					paridade = 0;
					for (y=1; y<3+tamanho; y++) {
						paridade ^= received[y];
					}
					if (paridade == received[y]) {
						sequencia_old = sequencia;
						
						sequencia = 0;
						sequencia = received[2] >> 6;
						sequencia |= received[1] << 2;
						sequencia &= 15;
	
						if(sequencia_old == sequencia) {
							criaMensagem(NULL, dados, 0, 0, ACK);
							write(rsocket, dados, 67);
						}
						else {
							for(y=0;y<tamanho;y++) {
								buffer[y] = received[3+y];	
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
			recv(rsocket,received,67,0);
		}
	}
}

/**
   @brief Builtin command: print help.
   @param args List of args.  Not examined.
   @return Always returns 1, to continue executing.
 */
int cli_help(char **args)
{
  int i;
  printf("lgsm15&mlas12's Trab1 based on Stephen Brennan's LSH\n");
  printf("Type on of the following built in commands:\n");

  for (i = 0; i < cli_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  return 1;
}

/**
   @brief Builtin command: exit.
   @param args List of args.  Not examined.
   @return Always returns 0, to terminate execution.
 */
int all_exit(char **args)
{
  return 0;
}

/**
   @brief Execute shell built-in or launch program.
   @param args Null terminated list of arguments.
   @return 1 if the shell should continue running, 0 if it should terminate
 */
int cli_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < cli_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }
  printf("Comando invalido\n");

  return 1;
  //return cli_launch(args);
}

int serv_execute(char type, char data[32])
{
  char* args[2];
  args[0] = malloc(sizeof(char)*3);
  args[1] = malloc(sizeof(char)*33);
  if(type == ls)strcpy(args[0], "ls");
  else if(type == cd)strcpy(args[0], "cd");
  strcpy(args[1], data);
  if(type==ls)
  {
    printf("op\n");
    cli_ls(args);
  }
  else if(type==cd) serv_cd(args);
  return 1;
  //return cli_launch(args);
}

/**
   @brief Read a line of input from stdin.
   @return The line from stdin.
 */
char *cli_read_line(void)
{
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    if (c == EOF) {
      exit(EXIT_SUCCESS);
    } else if (c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

#define cli_TOK_BUFSIZE 64
#define cli_TOK_DELIM " \t\r\n\a"
/**
   @brief Split a line into tokens (very naively).
   @param line The line.
   @return Null-terminated array of tokens.
 */
char **cli_split_line(char *line)
{
  int bufsize = cli_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, cli_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += cli_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        free(tokens_backup);
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, cli_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

/**
   @brief Loop getting input and executing it.
 */
void cli_loop(void)
{
  char *line;
  char **args;
  int status;

  do {
    printf("> ");
    line = cli_read_line();
    args = cli_split_line(line);
    status = cli_execute(args);

    free(line);
    free(args);
  } while (status);
}

void serv_loop(void)
{
  int i;
  unsigned char size, seq, type, pair;
  unsigned char data[32];
  while(listen(rsocket,2)) {
    FD_ZERO(&condicao); //socket vazio
    recv(rsocket, received, 37, 0);
    if(received[0] == init) {
      size = organizer(5, 0, received[1]);
      seq = organizer(0, 3, received[1]);
      seq |= organizer(3, 0, received[2]);
      type = organizer(0, 5, received[2]);
      for (; i < size; ++i)
      {
        data[i] = received[3 + i];
      }
      pair = 0;
      pair |= received[1];
      pair |= received[2];
      for (int j = 0; j < size; ++j)
      {
        pair |= data[j];
      }
      if (pair != received[3 + i])
      {
        printf("pairing error\n");
      }
      serv_execute(type, data);
    }
    else {
      //packet(NULL, NULL); //envia nack
      // write(rsocket, data, 36);  
    }
  }
}

//todo: limpar buffer + checar o arg[1] do ls
int ConexaoRawSocket(char *device)
{
  int soquete;
  struct ifreq ir;
  struct sockaddr_ll endereco;
  struct packet_mreq mr;

  soquete = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));    /*cria socket*/
  if (soquete == -1) {
    printf("Erro no Socket\n");
    exit(-1);
  }


  memset(&ir, 0, (size_t)sizeof(struct ifreq));   /*dispositivo eth0*/
  memcpy(ir.ifr_name, device, sizeof(device));
  if (ioctl(soquete, SIOCGIFINDEX, &ir) == -1) {
    printf("Erro no ioctl\n");
    exit(-1);
  }
  

  memset(&endereco, 0, sizeof(endereco));   /*IP do dispositivo*/
  endereco.sll_family = AF_PACKET;
  endereco.sll_protocol = htons(ETH_P_ALL);
  endereco.sll_ifindex = ir.ifr_ifindex;
  if (bind(soquete, (struct sockaddr *)&endereco, sizeof(endereco)) == -1) {
    printf("Erro no bind\n");
    exit(-1);
  }


  memset(&mr, 0, sizeof(mr));          /*Modo Promiscuo*/
  mr.mr_ifindex = ir.ifr_ifindex;
  mr.mr_type = PACKET_MR_PROMISC;
  if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1)  {
    printf("Erro ao fazer setsockopt\n");
    exit(-1);
  }

  return soquete;
}

/**
   @brief Main entry point.
   @param argc Argument count.
   @param argv Argument vector.
   @return status code
 */
int main(int argc, char **argv)
{
  int mode = 0;

  /* Verifica entradas */
  if (argc == 1 || strcmp(argv[1],"-s") != 0) 
  {
    printf("You're the client\n");
  }
  else
  {
    mode = 1; //seleciona o servidor
    printf("You're the server\n");
  }

  /* Cria o socket e o liga a interface */
  //if((rsocket = ConexaoRawSocket("eno1")) < 0){
  //  if(rsocket==-1) 
  //  {
  //    fprintf(stderr, "Erro ao abrir o raw socket (não é root).\n");
  //    system("sudo su");
  //  }
  //  exit(-1);
  //}
  
  /* Cria o terminal */

  if(mode == 0) {
    /* CLIENTE */   
    cli_loop();
  }
  else {
    /* SERVIDOR */
    serv_loop();
  }

  // Perform any shutdown/cleanup.
  return 1;
}
