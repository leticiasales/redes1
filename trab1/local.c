/***************************************************************************
  @author       Stephen Brennan
  @date         Thursday,  8 January 2015
  @brief        LSH (Libstephen SHell)

  @adapted by Leticia Sales and Matheus Lima
****************************************************************************/

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <inttypes.h>
#include <libgen.h>
#include <limits.h>
// #include <linux/if_packet.h>
// #include <linux/if.h>
#include <ncurses.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
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
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

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
unsigned char received[36];

typedef struct bloco bloco;

struct bloco
{
  unsigned char size;
  unsigned char seq;
  unsigned char type;
  unsigned char pair;
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

void packet(unsigned char ret[4], unsigned char size, unsigned char seq, unsigned char type, unsigned char pair)
{
  // printf("packet: %d, %d, %d, %d\n", dec_to_bin(size), dec_to_bin(seq), dec_to_bin(type), dec_to_bin(pair));
  ret[0] = init;
  ret[1] = size << 3;
  ret[1] |= organizer(5, 0, seq);
  ret[2] = organizer(0, 3, seq);
  ret[2] |= organizer(0, 5, type);
  ret[3] = pair;
}

int topack()
{
  bloco meubloco;
  unsigned char tmp[4];

  meubloco.size = organizer(0,s_size,'g');
  meubloco.seq = organizer(0,s_seq,'h');
  meubloco.type = organizer(0,s_type,'i');
  meubloco.pair = organizer(0,s_pair,'j');

  packet(tmp, meubloco.size, meubloco.seq, meubloco.type, meubloco.pair);

  printf("%08u.", dec_to_bin(tmp[0]));
  printf("%08u.", dec_to_bin(tmp[1]));
  printf("%08u.", dec_to_bin(tmp[2]));
  printf("%08u\n", dec_to_bin(tmp[3]));
}

/*
  Function Declarations for builtin shell commands:
 */

int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int tish_ls(char **args);
int serv_cd(char **args);
int serv_ls(char **args);

/*
  List of builtin commands, followed by their corresponding functions.
 */

char *builtin_str[] = {
  "cd",
  "help",
  "exit",
  "ls",
  "scd",
  "sls"
};

int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit,
  &tish_ls,
  &serv_cd,
  &serv_ls
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/

int serv_cd(char **args) {
  return 1;
}

int serv_ls(char **args) {
  tish_ls(args);
}

//It will print file Information in lines with File_permisiions and XYZ..!!
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

int tish_ls(char **args)
{
  char *curr_dir = NULL; 
  DIR *dp = NULL; 
  struct dirent *dptr = NULL; 
  unsigned int count = 0; 

  curr_dir = getenv("PWD"); 
  if(NULL == curr_dir){return 0;} 
   
  dp = opendir((const char*)curr_dir);    
  if(NULL == dp){return 0;} 
 
  if (args[1] == NULL) {
    for(count = 0; NULL != (dptr = readdir(dp)); count++) 
    { 
        if(dptr->d_name[0] != '.') 
        { 
          printf("%15s", dptr->d_name);
        } 
    } 
    printf("\n");
  }
  else if (strcmp(args[1],"-l")==0) {
    long_listing(".");
  }
  else if (strcmp(args[1],"-a")==0)
  {
    for(count = 0; NULL != (dptr = readdir(dp)); count++) 
    { 
      printf("%15s", dptr->d_name);
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
int lsh_cd(char **args)
{
  printf(getcwd());
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
int lsh_help(char **args)
{
  int i;
  printf("Stephen Brennan's LSH + lgsm15&mlas12's Trab1\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  return 1;
}

/**
   @brief Builtin command: exit.
   @param args List of args.  Not examined.
   @return Always returns 0, to terminate execution.
 */
int lsh_exit(char **args)
{
  return 0;
}

/**
   @brief Execute shell built-in or launch program.
   @param args Null terminated list of arguments.
   @return 1 if the shell should continue running, 0 if it should terminate
 */
int lsh_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }
  printf("Comando invalido\n");

  return 1;
  //return lsh_launch(args);
}

int serv_execute(char *args)
{
  int i;
  unsigned int type = args[0];

  if(type==ls) serv_ls(&args);
  else if(type==cd)printf("cd");

  return 0;
  //return lsh_launch(args);
}

#define LSH_RL_BUFSIZE 1024
/**
   @brief Read a line of input from stdin.
   @return The line from stdin.
 */
char *lsh_read_line(void)
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

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
/**
   @brief Split a line into tokens (very naively).
   @param line The line.
   @return Null-terminated array of tokens.
 */
char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
		    free(tokens_backup);
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

/**
   @brief Loop getting input and executing it.
 */
void lsh_loop(void)
{
  char *line;
  char **args;
  int status;

  do {
    printf("> ");
    line = lsh_read_line();
    args = lsh_split_line(line);
    status = lsh_execute(args);

    free(line);
    free(args);
  } while (status);
}

void serv_loop(void)
{
  int i;
  unsigned char size, seq, type, data, pair;
  while(listen(rsocket,2)) {
    FD_ZERO(&condicao); //socket vazio
    recv(rsocket, received, 36, 0);
    if(received[0] == init) {
      size = organizer(5, 0, received[1]);
      seq = organizer(0, 3, received[1]);
      seq |= organizer(3, 0, received[2]);
      type = 0x7;//organizer(0, 5, received[2]);
      data = ""; //@@@@@@@
      pair = 0;
      serv_execute(&type);
    }
    else {
      packet(NULL, 0, 0, nack, 0); //envia nack
      // write(rsocket, data, 36);  
    }
  }
}

// int abrirRawSocket(char *interface){
//   struct packet_mreq mreq;
//   struct ifreq ifr; // 
//   struct sockaddr_ll local;

//   /* Abre/cria o socket */
//   if((rsocket = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) < 0){ // AF_PACKET = low level packet interface; 
//                       // SOCK_RAW = packets are passed to and from the device driver without any changes in the packet data.
//     return -1;
//   }

//   memset(&ifr, 0, sizeof(ifr)); // "aloca" um espaço para os dados da interface
//   strcpy(ifr.ifr_name, interface); // copia o nome da interface em ifr.ifr_name
//   if(ioctl(rsocket, SIOCGIFINDEX, &ifr) < 0){ // o indice da interface deve ter sido retornado em ifr.ifr_ifindex
//     return -2;
//   }

//   /* Liga o socket a interface */
//   memset(&local, 0, sizeof(local)); // "aloca" um espaço para o 'local'
//   local.sll_family = AF_PACKET;
//   local.sll_ifindex = ifr.ifr_ifindex;
//   local.sll_protocol = htons(ETH_P_ALL);
//   if(bind(rsocket, (struct sockaddr *) &local, sizeof(local)) < 0){
//           return -3;
//   }

//   /* Liga o modo promiscuo (envia mensagem para todos na rede) */
//   memset(&mreq, 0, sizeof(mreq));
//   mreq.mr_ifindex = ifr.ifr_ifindex;
//   mreq.mr_type = PACKET_MR_PROMISC;
//   if(setsockopt(rsocket, SOL_PACKET, PACKET_ADD_MEMBERSHIP,&mreq, sizeof(mreq)) < 0){
//     return -4;
//   }
//   return rsocket;
// }

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

  int i = 1;
  if(strcmp(argv[i],"-s") == 0) { // é servidor
    mode = 1; //seleciona o servidor
    printf("You're the server\n");
  }
  else{printf("You're the client\n");}


  /* Cria o socket e o liga a interface */

  // if((rsocket = abrirRawSocket("localhost")) < 0){
  //   if(rsocket==-1) {
  //     fprintf(stderr, "Erro ao abrir o raw socket (não é root).\n");
  //     system("sudo su");
  //   }
  //   exit(-1);
  // }

  /* Cria o terminal */

  if(mode == 0) { //é cliente
    
    /* CLIENTE */   

    // Run command loop.
    lsh_loop();
  }
  else { // é servidor

    /* SERVIDOR */
    serv_loop();
  }

  // Perform any shutdown/cleanup.
  return 1;
}

