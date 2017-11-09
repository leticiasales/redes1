#include "lib.h"

struct bloco
{
	unsigned char size;
	unsigned char seq;
	unsigned char type;
	//unsigned char* data;
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
	if(left) return(byte>>(7-left));
	unsigned char tmp = byte<<(9-right);
	return tmp>>(9-right);
}

unsigned char *packet(char size, char seq, char type, char pair)
{
	printf("%d, %d, %d, %d\n", dec_to_bin(size), dec_to_bin(seq), dec_to_bin(type), dec_to_bin(pair));
	unsigned char tmp[4];
	tmp[0] = init;
	tmp[1] = size << 3;
	tmp[1] |= organizer(3, 0, seq);;
	tmp[2] = organizer(0, 3, seq)<<5;
	tmp[2] |= organizer(0, 5, type);
	tmp[3] = pair;
	return tmp;
}