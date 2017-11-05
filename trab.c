#include <stdio.h>
#include <stdlib.h>

const int s_init = 8;
const int s_size = 5;
const int s_seq = 6;
const int s_type = 5;
const int s_pair = 8;

const unsigned char init = 0x7E;

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

int main(int argc, char const *argv[])
{
	int left = 1;
	int right = 1;
	bloco meubloco;
	unsigned char tmp[4];

	meubloco.size = organizer(0,s_size,'g');
	meubloco.seq = organizer(0,s_seq,'h');
	meubloco.type = organizer(0,s_type,'i');
	meubloco.pair = organizer(0,s_pair,'j');

	packet(tmp, meubloco.size, meubloco.seq, meubloco.type, meubloco.pair);

	printf("0 %u\n", dec_to_bin(tmp[0]));
	printf("1 %u\n", dec_to_bin(tmp[1]));
	printf("2 %u\n", dec_to_bin(tmp[2]));
	printf("3 %u\n", dec_to_bin(tmp[3]));
	/*
	scanf("%c", &teste);
	while((left+right)!=0) {
		printf("char: %c int: %u bin: %u\n", teste, teste, decToBin(teste));
		scanf("%d", &left);
		scanf("%d", &right);
		tmp = organizer(teste, left, right);
		printf("organizer = char: %c int: %u bin: %u\n", tmp, tmp, decToBin(tmp));
	}
	*/
	return 0;
}