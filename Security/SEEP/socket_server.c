#include "tomcrypt.h"

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
 
#define PORTNUM 3333

unsigned char pt[20], out[1024], pt2[20], serv_out_private[1024], serv_out_public[1024];
int err, hash_idx, prng_idx;
ecc_key serv_mykey, serv_private_key, serv_public_key, client_public_key, client_private_key;
unsigned long ptlen, outlen;
prng_state prng;
symmetric_key skey;
FILE *f;
unsigned long nonceA, K_key;

	/* export_to_file() 							*/
	/* function creates a file which contains a key				*/
	/* 									*/
	/* INPUTS: path_to_file, int isPublic (1=public, 0=private)		*/
	/* OUTPUTS:								*/
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
		        fprintf(f, "%c", serv_out_public[i]);
		}
	}
	else{
		for (i = 0; i < outlen; ++i) {
		        fprintf(f, "%c", serv_out_private[i]);
		}
	}
	fclose(f);
}

	/* generate_keys() 							*/
	/* Function initializes pseudo-rundom-number-generator, hash, defines 	*/
	/* math functions and generates public/private key pair			*/
	/* INPUTS:								*/
	/* OUTPUTS:								*/
void generate_keys(void){

	/*Pseudo-Random-Number-Generator definition*/
	if (register_prng(&sprng_desc) == -1) {
		printf("Error registering SPRNG\n");
		return -1;
	}
	/* Math Library Configuration */
	ltc_mp = ltm_desc;
	/* Hash regestration */
	if (register_hash(&sha1_desc) == -1) {
		printf("Error registering sha1");
		return EXIT_FAILURE;
	}
	
	/* Hash an PRNG indexes generation */
	hash_idx = find_hash("sha1");
	prng_idx = find_prng("sprng");
	/* Creation of ECC key */
	if ((err = ecc_make_key(NULL, find_prng("sprng"), 24, &serv_mykey))
		!= CRYPT_OK) {
		printf("Error making key: %s\n", error_to_string(err));
		return -1;
	}
	/* Create an exported public key from ECC key */
	outlen = sizeof(serv_out_public);
	ecc_export(serv_out_public, &outlen, 0, &serv_mykey);			// 0 means Public key
	/* Export the public key to a file to share with Client */
	export_to_file("/home/ilya/Documents/RES/crypto/serv_public_key", 1);
	/* Create an exported public key from ECC key */
	outlen = sizeof(serv_out_private);
	ecc_export(serv_out_private, &outlen, 1, &serv_mykey);			// 1 means Private key
	/* Import ECC private and public keys and store them */
	outlen = sizeof(serv_out_public);
	ecc_import(serv_out_public, outlen, &serv_public_key);
	outlen = sizeof(serv_out_private);
	ecc_import(serv_out_private, outlen, &serv_private_key);
}

void import_from_file(char path[],int isPublic){
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

	printf("FROM FILE%s\n", import_key_array);
	fclose(f);
	
	outlen = sizeof(import_key_array);
	if (isPublic){
		if((err = ecc_import(import_key_array, outlen, &client_public_key)) != CRYPT_OK){
			 printf("Export error: %s\n", error_to_string(err));
			 return -1;
		}
	}
	else{
		if((err = ecc_import(import_key_array, outlen, &client_private_key)) != CRYPT_OK){
			 printf("Export error: %s\n", error_to_string(err));
			 return -1;
		}
	}
}

void encrypt_msg(void){
	ptlen = sizeof(pt);
	outlen = sizeof(out);
	err = ecc_encrypt_key(	pt,
				ptlen,
				out,
				&outlen,
				NULL,
				find_prng("sprng"),
				hash_idx,
				&client_public_key);
	
	printf("PT => %s\n", pt);
	printf("OUT => %s\n", out);
	printf("PT2 => %s\n", pt2);
}

void decrypt_msg(void){
	ptlen = sizeof(pt2);
	outlen = sizeof(out);
	err = ecc_decrypt_key(	out,
				outlen,
				pt2,
				&ptlen,
				&serv_private_key);

	printf("PT => %s\n", pt);
	printf("OUT => %s\n", out);
	printf("PT2 => %s\n", pt2);
}

void generate_session_key(void){

	K_key = rand();
	printf("K_key: %d\n", K_key);
	sprintf(pt,"%ld",K_key);
	printf("K_key_string: %s\n", pt);
}

void aes_decrypt(){
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
	
	aes_ecb_decrypt(	pt,		/* decrypt this 8-byte array */
				out,		/* store decrypted data here */
				&skey);		/* our previously scheduled key */

	printf("PT => %s\n", pt);
	printf("OUT => %s\n", out);
/* now we have decrypted ct to the original plaintext in pt */

}

int main(int argc, char *argv[])
{
	srand(time(NULL));
	/* Generate ECC Server public and private keys */
	generate_keys();
	/* Socket initialization */
	int len;
	struct sockaddr_in dest; 			/* socket info about the machine connecting to us */
	struct sockaddr_in serv; 			/* socket info about our server */
	int mysocket;            			/* socket used to listen for incoming connections */
	socklen_t socksize = sizeof(struct sockaddr_in);

	memset(&serv, 0, sizeof(serv));           	/* zero the struct before filling the fields */
	serv.sin_family = AF_INET;                	/* set the type of connection to TCP/IP */
	serv.sin_addr.s_addr = htonl(INADDR_ANY); 	/* set our address to any interface */
	serv.sin_port = htons(PORTNUM);           	/* set the server port number */    

	mysocket = socket(AF_INET, SOCK_STREAM, 0);

	/* bind serv information to mysocket */
	bind(mysocket, (struct sockaddr *)&serv, sizeof(struct sockaddr));

	/* start listening, allowing a queue of up to 1 pending connection */
	listen(mysocket, 1);
	int consocket = accept(mysocket, (struct sockaddr *)&dest, &socksize);

	while(consocket)
	{
		/* Import of Client Public key */
		import_from_file("/home/ilya/Documents/RES/crypto/client_public_key",1);
		/* Import of Client private key for debugging purpose */
		//import_from_file("/home/ilya/Documents/RES/crypto/client_private_key",0);
		memset(out, 0, sizeof(out));
		printf("Incoming connection from %s - sending welcome\n", inet_ntoa(dest.sin_addr));
// 4B	------------------------------------------------------------------------
		/* Receive "NonceA" from client */
		len = recv(consocket, out, sizeof(out), 0);
		out[len] = '\0';
		printf("PT_nonce => %s\n",out);
		/* Decrypt "NonceA", Decrypted message stored in pt2. Store "NonceA" in "nonceA" variable */
		decrypt_msg();
		memset(out, 0, sizeof(out));
		sscanf(pt2, "%d", &nonceA);
// 5B	---------------------------------------------------------------------------
		memset(pt2, 0, sizeof(pt2));
		printf("nonce_int: %d\n", nonceA);
		/* Increment "NonceA" by one */
		nonceA++;
		printf("nonce_int: %d\n", nonceA);
		/* Generate random number(session key) and store it in K_key variable */
		generate_session_key();
// 6B	-----------------------------------------------------------------------------
		/* Create string {nonceA+1;K_key}*/
		memset(pt, 0, sizeof(pt));
		snprintf(pt, sizeof(pt)+3, "%d,%d,\0", nonceA, K_key);
		printf("{nonceA+1,K}: %s\n", pt);
		/* Encrypt the message */
		encrypt_msg();
		memset(pt, 0, sizeof(pt));
		printf("beforeSend %s\n",out);
		memset(pt2, 0, sizeof(pt2));
		/* Send {nonceA+1;K_key} to Client */
		write(consocket, out, sizeof(out));
// 10B	----------------------------------------------------------------------------
// From this step it doesn't work properly. AES encryption and decryption is working within one program. Sent message doesn't decrypts properly.
// One of the possible problems were differences in K_key while sending it, however I was trying to implement AES separately, using constant keys and 
// messages, but still it works only within one program. Because of the lack of time the we have stopped working on this assignment.
		memset(pt, 0, sizeof(pt));
		len = recv(consocket, pt, sizeof(pt), 0);
		//out[len] = '\0';
		printf("Encrtd:{nonceA+1}: %s\n", pt);
		printf("Encrtd:{nonceA+1}: %d\n", sizeof(pt));
		printf("sizeof K_key: %d\n", sizeof(K_key));
		memset(out, 0, sizeof(out));
		K_key = 1234;
		aes_decrypt();
		consocket = accept(mysocket, (struct sockaddr *)&dest, &socksize);
		
		
		
	}

	close(consocket);
	close(mysocket);
	return EXIT_SUCCESS;
}
