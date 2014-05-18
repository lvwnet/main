/*
 * strtoh.h - Convert a 2 digit hexadecimal string to its value in a unsigned char
 */

#ifndef MAC_STRTOH_H
#define MAC_STRTOH_H

#define STRMACLEN 12

/*
 * ctoh(): Recebe um digito (char) e retorna seu valor em hexadecimal.
 * Valor de retorno: 0 a 16 em situacoes normais. Em caso de falha, -1.
 */
int ctoh (char c)
{
	char* hexd = "0123456789abcdef";
	int hexdlen = 16;
	int i;

	for (i=0;i<hexdlen;i++) {
		if (hexd[i]==c) return i;
	}
	
	return -1;
}


//unsigned char strtoh (unsigned char* s)
int mac_strtoh (char* hexmac, char* strmac)
{
	int i, pos;
	char digit1, digit2, c;
	unsigned char octet;

	pos = 0;

	for (i=0;i<STRMACLEN;i++) {
		digit1 = strmac[i];
		digit2 = strmac[i+1];
		
		if ( (c = ctoh(digit1)) < 0) {
			//printf ("falhou ao calcular\n");
			return 0;
		}
		octet = (unsigned char) c;

		if ( (c = ctoh(digit2)) < 0) {
			//printf ("falhou ao calcular\n");
			return 0;
		}
		octet = octet * 16 + c;
		hexmac[pos] = octet;
		i++;
		pos++;
	}

	return 0;
}

#endif // MAC_STRTOH_H
