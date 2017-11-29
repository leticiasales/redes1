/***************************************************************************
  @author       Stephen Brennan
  @date         Thursday,  8 January 2015
  @brief        LSH (Libstephen SHell)

  @adapted by Leticia Sales and Matheus Lima
****************************************************************************/
#ifndef MY_HEADER_FILE_
#define MY_HEADER_FILE_

// here is your header file code
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

#endif


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
  printf("args[1] = %s\n", args[1]);
  char *curr_dir = NULL; 
  DIR *dp = NULL; 
  struct dirent *dptr = NULL; 
  unsigned int count = 0; 

  curr_dir = getcwd(curr_dir, LSH_RL_BUFSIZE); 
  if(NULL == curr_dir){printf("oi"); return 1;} 
   
  dp = opendir((const char*)curr_dir);    
  if(NULL == dp){printf("oi"); return 0;} 
 
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
  if((rsocket = ConexaoRawSocket("eno1")) < 0){
    if(rsocket==-1) 
    {
      fprintf(stderr, "Erro ao abrir o raw socket (não é root).\n");
      system("sudo su");
    }
    exit(-1);
  }
  
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

