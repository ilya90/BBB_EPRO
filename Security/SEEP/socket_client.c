#include "tomcrypt.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
 
#define MAXRCVLEN 500
#define PORTNUM 3333
 
	unsigned char pt[20], out[1024], pt2[20], client_out_private[1024], client_out_public[1024];
	int err, hash_idx, prng_idx, len;
	unsigned long nonceA, nonceA1;
	ecc_key client_mykey, client_private_key, client_public_key, serv_public_key;
	unsigned long ptlen, outlen, K_key;
	prng_state prng;
	eax_state eax;
	symmetric_key skey;
	FILE *f;


void export_to_file(char path[],int isPublic){
	f = fopen(path, "w");
	if (f == NULL)
	{
	    printf("Error opening file!\n");
	    exit(1);
	}
	unsigned long i = 0;
	if (isPublic){
		for (i = 0; i < outlen; ++i) {
		        fprintf(f, "%c", client_out_public[i]);
		}
	}
	else{
		for (i = 0; i < outlen; ++i) {
		        fprintf(f, "%c", client_out_private[i]);
		}
	}
	fclose(f);
}

void generate_keys(void){

	/*Pseudo-Random-Number-Generator definition*/
	if (register_prng(&sprng_desc) == -1) {
		printf("Error registering SPRNG\n");
		return -1;
	}

	ltc_mp = ltm_desc;

	if (register_hash(&sha1_desc) == -1) {
		printf("Error registering sha1");
		return EXIT_FAILURE;
	}

	hash_idx = find_hash("sha1");
	prng_idx = find_prng("sprng");
	
	if ((err = ecc_make_key(NULL, find_prng("sprng"), 24, &client_mykey))
		!= CRYPT_OK) {
		printf("Error making key: %s\n", error_to_string(err));
		return -1;
	}

	outlen = sizeof(client_out_public);
	ecc_export(client_out_public, &outlen, 0, &client_mykey);
	export_to_file("/home/ilya/Documents/RES/crypto/client_public_key", 1);
	
	outlen = sizeof(client_out_private);
	ecc_export(client_out_private, &outlen, 1, &client_mykey);
	export_to_file("/home/ilya/Documents/RES/crypto/client_private_key", 0);
	outlen = sizeof(client_out_public);
	ecc_import(client_out_public, outlen, &client_public_key);
	outlen = sizeof(client_out_private);
	ecc_import(client_out_private, outlen, &client_private_key);
}

void generate_nonce(void){
	/* Generate random number "Nonce" and put the number to the character array "pt" for sending */
	nonceA = rand();
	printf("NONCE: %d\n", nonceA);
	sprintf(pt,"%ld",nonceA);
	printf("nonce_string: %s\n", pt);
}

void encrypt_msg(){
	
	ptlen = sizeof(pt);
	outlen = sizeof(out);
	err = ecc_encrypt_key(	pt,
				ptlen,
				out,
				&outlen,
				NULL,
				find_prng("sprng"),
				hash_idx,
				&serv_public_key);
	
	printf("PT => %s\n", pt);
	printf("OUT => %s\n", out);
	printf("PT2 => %s\n", pt2);
	ptlen = sizeof(pt2);
}

void decrypt_msg(void){
	ptlen = sizeof(pt2);
	outlen = sizeof(out);
	err = ecc_decrypt_key(	out,
				outlen,
				pt2,
				&ptlen,
				&client_private_key);

	printf("PT => %s\n", pt);
	printf("OUT => %s\n", out);
	printf("PT2 => %s\n", pt2);
}

void import_from_file(char path[]){
	f = fopen(path, "r");
	if (f == NULL)
	{
	    printf("Error opening file!\n");
	    exit(1);
	}

	fseek(f, 0, SEEK_END);         // seek to end of file
        int key_size = ftell(f);        // get current file pointer
        fseek(f, 0, SEEK_SET);         // seek back to beginning of file
        char import_key_array[key_size];
        unsigned long i = 0;
        for (i = 0; i < key_size; ++i) {
                fscanf(f, "%c", &import_key_array[i]);
                printf("%c",import_key_array[i]);
        }
	fclose(f);
	
	outlen = sizeof(import_key_array);
	if((err = ecc_import(import_key_array, outlen, &serv_public_key)) != CRYPT_OK){
		 printf("Export error: %s\n", error_to_string(err));
	         return -1;
	}
}	

void aes_encrypt(){
	const unsigned char K_key_char[20];
	sscanf(K_key_char, "%d", &K_key);
	if ((err = aes_setup	(	K_key_char, 	/* the key we will use */
					16, 	/* key is 8 bytes (64-bits) long */
					0, 	/* 0 == use default # of rounds */
					&skey) 	/* where to put the scheduled key */
	) != CRYPT_OK) {
	printf("Setup error: %s\n", error_to_string(err));
	return -1;
	}
	unsigned char zt[] = "123";
	/* encrypt the block */
	aes_ecb_encrypt(	zt, /* encrypt this 8-byte array */
				pt2, /* store encrypted data here */
				&skey); /* our previously scheduled key */
	printf("PT => %s\n", zt);
	printf("PT2 => %s\n", pt2);

	if ((err = aes_setup	(	K_key_char, 	/* the key we will use */
					16, 	/* key is 8 bytes (64-bits) long */
					0, 	/* 0 == use default # of rounds */
					&skey) 	/* where to put the scheduled key */
	) != CRYPT_OK) {
	printf("Setup error: %s\n", error_to_string(err));
	return -1;
	}

	aes_ecb_decrypt(	pt2,		/* decrypt this 8-byte array */
				out,		/* store decrypted data here */
				&skey);		/* our previously scheduled key */

	printf("PT2 => %s\n", pt2);
	printf("OUT => %s\n", out);
}

int main(int argc, char *argv[])
{
	srand(time(NULL));
	/* Generate ECC Client public and private keys */
	generate_keys();
	/* Socket initialization */
	unsigned char buffer[1024]; 
	int len, mysocket;
	struct sockaddr_in dest; 

	mysocket = socket(AF_INET, SOCK_STREAM, 0);

	memset(&dest, 0, sizeof(dest));                /* zero the struct */
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = inet_addr("127.0.0.1"); /* set destination IP number */ 
	dest.sin_port = htons(PORTNUM);                /* set destination port number */
	
/*-----------------------------------------------------------------------*/
	/* Import of Server Public key */
	import_from_file("/home/ilya/Documents/RES/crypto/serv_public_key");
/*-----------------------------------------------------------------------*/

// 1A	---------------------------------------------------------------
	printf("Enter any string ( upto 20 character ) \n");
    	scanf("%s", &pt);
// 2A -----------------------------------------------------------------
	generate_nonce();
// 3A	----------------------------------------------------------------
	/* Encrypt message in "pt"(NonceA) output is in the "out" character array */
	encrypt_msg();
	connect(mysocket, (struct sockaddr *)&dest, sizeof(struct sockaddr));
	printf("beforeSend %s\n",out);
	/* Send Nonce to the Server */
	write(mysocket, out, sizeof(out));
// 7A	-------------------------------------------------------------------
	memset(out, 0, sizeof(out));
	memset(pt2, 0, sizeof(pt2));
	len = recv(mysocket, out, sizeof(out), 0);
	out[len] = '\0';
	printf("crypt{Nonce+1,K} %s\n",out);
	decrypt_msg();
	char *pch;
	pch = strtok(pt2,",");
	printf("%s\n",pch);
	sscanf(pch, "%d", &nonceA1);
	printf("{nonceA1:} %d\n",nonceA1);
	pch = strtok(NULL,",");
	printf("%s\n",pch);
	sscanf(pch, "%ld", &K_key);
	printf("{K_key}: %d\n",K_key);
		K_key = 1234;
// 8A	----------------------------------------------------------------------------
// From this step it doesn't work properly. AES encryption and decryption is working within one program. Sent message doesn't decrypts properly.
// One of the possible problems were differences in K_key while sending it, however I was trying to implement AES separately, using constant keys and 
// messages, but still it works only within one program. Because of the lack of time the we have stopped working on this assignment.	
	memset(pt, 0, sizeof(pt));
	memset(pt2, 0, sizeof(pt2));
	sprintf(pt,"%ld",nonceA1);
	printf("before_send: %s\n", pt);
	aes_encrypt();
	printf("Encrtd:{nonceA+1}: %d\n", sizeof(pt2));
	printf("sizeof K_key: %d\n", sizeof(K_key));
		
	write(mysocket, pt2, sizeof(pt2));
// 9A	-----------------------------------------------------------------------------
	close(mysocket);
	return EXIT_SUCCESS;
}
