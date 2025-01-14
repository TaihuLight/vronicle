#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/pem.h>

#include <fstream>
#include <iostream>

#include "RawBase.h"

using namespace std;

typedef void CRYPTO_RWLOCK;

struct evp_pkey_st {
    int type;
    int save_type;
    int references;
    const EVP_PKEY_ASN1_METHOD *ameth;
    ENGINE *engine;
    union {
        char *ptr;
# ifndef OPENSSL_NO_RSA
        struct rsa_st *rsa;     /* RSA */
# endif
# ifndef OPENSSL_NO_DSA
        struct dsa_st *dsa;     /* DSA */
# endif
# ifndef OPENSSL_NO_DH
        struct dh_st *dh;       /* DH */
# endif
# ifndef OPENSSL_NO_EC
        struct ec_key_st *ec;   /* ECC */
# endif
    } pkey;
    int save_parameters;
    STACK_OF(X509_ATTRIBUTE) *attributes; /* [ 0 ] */
    CRYPTO_RWLOCK *lock;
} /* EVP_PKEY */ ;

EVP_PKEY *evp_pkey = NULL;
RSA *keypair = NULL;
char hash_of_file[65];
char* base64signature;

unsigned char* image_buffer = NULL;	/* Points to large array of R,G,B-order data */
unsigned char* pure_input_image_str = NULL; /* for signature verification purpose */
pixel* image_pixels;    /* also RGB, but all 3 vales in a single instance (used for processing filter) */
int image_height = 0;	/* Number of rows in image */
int image_width = 0;		/* Number of columns in image */

char* processed_img_str;

pixel* unsigned_chars_to_pixels(unsigned char* uchars, int num_of_pixels){
    // This will allocate new memory and return it (pixels)
    // Return NULL on error
    if(uchars == NULL){
        return NULL;
    }
    pixel* results = (pixel*)malloc(sizeof(pixel) * num_of_pixels);
    memcpy(results, uchars, num_of_pixels * 3);
    return results;
}

int pixels_to_linked_pure_str(pixel* pixels_to_be_converted, int total_number_of_rgb_values, char* output_str){
	// Return the len of (fake) str
	char* temp_output_str = output_str;
	int len_of_str = 0;
	for(int i = 0; i < total_number_of_rgb_values - 1; ++i){
        memcpy(temp_output_str++, &pixels_to_be_converted[i].r, 1);
        memcpy(temp_output_str++, &pixels_to_be_converted[i].g, 1);
        memcpy(temp_output_str++, &pixels_to_be_converted[i].b, 1);
		len_of_str += 3;
	}
	// printf("Testing if we copy it successfully: %s\n", &(output_str[7692]));
	return len_of_str;
}

int read_raw_file(const char* file_name){
    // Return 0 on success, return 1 on failure
    // cout << "Going to read raw file: " << file_name << endl;
    FILE* input_raw_file = fopen(file_name, "r");
    if(input_raw_file == NULL){
        return 1;
    }
    char buff[257]; // Plus one for eof
    int counter_for_image_info = 0; // First two are width and height
    int counter_for_checking_if_all_rgb_values_read_properly = 0;
    char* info;
    while(fgets(buff, 257, input_raw_file) != NULL){
        // printf("buff: %s\n", buff);
        info = strtok(buff, ",");
        while(info != NULL){
            // printf("Info: %d\n", atoi(info));
            if(counter_for_image_info == 0){
                image_width = atoi(info);
                ++counter_for_image_info;
            } else if (counter_for_image_info == 1){
                image_height = atoi(info);
                ++counter_for_image_info;
                // printf("The image has width: %d, and height: %d.\n", image_width, image_height);
                image_buffer = (unsigned char*)malloc(sizeof(unsigned char) * image_width * image_height * 3);
            } else {
                if(counter_for_checking_if_all_rgb_values_read_properly + 10 >= image_width * image_height * 3){
                    // printf("Current counter: %d, current limit: %d.\n", counter_for_checking_if_all_rgb_values_read_properly, image_width * image_height * 3);
                    // printf("Current info: %d\n", atoi(info));
                }
                image_buffer[counter_for_checking_if_all_rgb_values_read_properly++] = atoi(info);
            }
            info = strtok(NULL, ",");
        }
        // printf("Current buff: %s\n", buff);
    }
    if(image_buffer == NULL || image_height == 0 || image_width == 0 || 
        counter_for_checking_if_all_rgb_values_read_properly != image_width * image_height * 3){
            return 1;
        }
    // printf("The very first pixel has RGB value: (%d, %d, %d).\n", image_buffer[0], image_buffer[1], image_buffer[2]);

    int total_number_of_pixels = image_width * image_height;
    image_pixels = unsigned_chars_to_pixels(image_buffer, total_number_of_pixels);

    return 0;
}

void sha256_hash_string (unsigned char hash[SHA256_DIGEST_LENGTH], char outputBuffer[65])
{
    int i = 0;

    for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }

    outputBuffer[64] = 0;
}

int str_to_hash(char* str_for_hashing, size_t size_of_str_for_hashing, char* hash_out){
    // Return 0 on success, otherwise, return 1

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str_for_hashing, size_of_str_for_hashing);
    SHA256_Final(hash, &sha256);

    sha256_hash_string(hash, hash_out);
    return 0;
}

int read_rsa_pub_key(const char* publickey_file_name){
    // Return 0 on success, otherwise, return 1
    FILE* publickey_file = fopen(publickey_file_name, "r");
    if(publickey_file == NULL){
        printf("what? No file?\n");
        return 1;
    }
    printf("Potential public file: {%s} has been read\n", publickey_file_name);
    /*
    keypair = PEM_read_RSAPublicKey(publickey_file, &keypair, NULL, NULL);
    if(keypair == NULL){
        return 1;
    }
    */
    evp_pkey = PEM_read_PUBKEY(publickey_file, &evp_pkey, NULL, NULL);
    if(evp_pkey == NULL){
        printf("A NULL key: %s\n", ERR_error_string(ERR_get_error(), NULL));
        return 1;
    }


    // public key - string
	int len = i2d_PublicKey(evp_pkey, NULL);
	unsigned char *buf = (unsigned char *) malloc (len + 1);
	unsigned char *tbuf = buf;
	i2d_PublicKey(evp_pkey, &tbuf);

    printf ("{\"public\":\"");
	int i;
	for (i = 0; i < len; i++) {
	    printf("%02x", (unsigned char) buf[i]);
	}
	printf("\"}\n");

	free(buf);

    fclose(publickey_file);

    return 0;
}

int read_file_as_hash(char* file_path){
    // Return 0 on success, otherwise, return 1
    FILE *file = fopen(file_path, "rb");
    if(file == NULL){
        return 1;
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    const int bufSize = 32768;
    unsigned char* buffer = (unsigned char*)malloc(bufSize);
    int bytesRead = 0;
    if(!buffer) return ENOMEM;
    while((bytesRead = fread(buffer, 1, bufSize, file)))
    {
        SHA256_Update(&sha256, buffer, bytesRead);
    }
    SHA256_Final(hash, &sha256);

    sha256_hash_string(hash, hash_of_file);
    fclose(file);
    free(buffer);
    return 0;
}

int read_signature(const char* sign_file_name){
    // Return 0 on success, otherwise, return 1
    FILE* signature_file = fopen(sign_file_name, "r");
    if(signature_file == NULL){
        return 1;
    }

    /*
    ifstream signature_file;
    signature_file.open(sign_file_name);

    if(!signature_file.is_open()){
        return 1;
    }
    */

    /*
    while(!signature_file.eof()){
        signature_file >> base64signature;
    }
    */
    
    //fgets(base64signature, 2048, signature_file);

    fseek(signature_file, 0, SEEK_END);
    long length = ftell(signature_file);
    fseek(signature_file, 0, SEEK_SET);

    base64signature = (char*)malloc(length);

    fread(base64signature, 1, length, signature_file);

    fclose(signature_file);
    return 0;
}

size_t calcDecodeLength(const char* b64input) {
  size_t len = strlen(b64input), padding = 0;
  printf("The len in calc is: %d\n", (int)len);

  if (b64input[len-1] == '=' && b64input[len-2] == '=') //last two chars are =
    padding = 2;
  else if (b64input[len-1] == '=') //last char is =
    padding = 1;

  printf("The padding in calc is: %d\n", (int)padding);
  return (len*3)/4 - padding;
}

void Base64Decode(const char* b64message, unsigned char** buffer, size_t* length) {
  BIO *bio, *b64;

  int decodeLen = calcDecodeLength(b64message);
  printf("decodeLen is: %d\n", decodeLen);
  *buffer = (unsigned char*)malloc(decodeLen + 1);
  (*buffer)[decodeLen] = '\0';

  bio = BIO_new_mem_buf(b64message, -1);
  b64 = BIO_new(BIO_f_base64());
  bio = BIO_push(b64, bio);

  *length = BIO_read(bio, *buffer, strlen(b64message));
  printf("The length is: %d\n", (int)*length);
  printf("The buffer is: %s\n", buffer);
  BIO_free_all(bio);
}

int verify_signature(){
    // Return 1 on success, otherwise, return 0

    EVP_MD_CTX *mdctx;
	const EVP_MD *md;
	unsigned char md_value[EVP_MAX_MD_SIZE];
	unsigned int md_len, i;
	int ret;

	OpenSSL_add_all_digests();

    md = EVP_get_digestbyname("SHA256");

	if (md == NULL) {
         printf("Unknown message digest %s\n", "SHA256");
         exit(1);
    }

	mdctx = EVP_MD_CTX_new();
	EVP_DigestInit_ex(mdctx, md, NULL);

	ret = EVP_VerifyInit_ex(mdctx, EVP_sha256(), NULL);
	if(ret != 1){
		printf("EVP_VerifyInit_ex error. \n");
        exit(1);
	}

    printf("hash_of_file to be verified: %s\n", hash_of_file);

	ret = EVP_VerifyUpdate(mdctx, (void*)hash_of_file, sizeof(hash_of_file));
	if(ret != 1){
		printf("EVP_VerifyUpdate error. \n");
        exit(1);
	}

    // printf("base64signature: %s\n", base64signature);
    unsigned char* encMessage;
    size_t encMessageLength;
    Base64Decode(base64signature, &encMessage, &encMessageLength);

    cout << "(lalala)size of raw signature is: " << (int)encMessageLength << endl;
    cout << "(lalala)signature: " << (char*)encMessage << endl;

	ret = EVP_VerifyFinal(mdctx, encMessage, encMessageLength, evp_pkey);
	printf("EVP_VerifyFinal result: %d\n", ret);

	// Below part is for freeing data
	// For freeing evp_md_ctx
	EVP_MD_CTX_free(mdctx);

    return ret;
}

unsigned char* read_signature_n(const char* sign_file_name, size_t* signatureLength){
    // Return signature on success, otherwise, return NULL
    // Need to free the return after finishing using
    FILE* signature_file = fopen(sign_file_name, "r");
    if(signature_file == NULL){
        return NULL;
    }

    fseek(signature_file, 0, SEEK_END);
    long length = ftell(signature_file);
    printf("read_signature: length of file from ftell is: %d\n", length);
    fseek(signature_file, 0, SEEK_SET);

    base64signature = (char*)malloc(length + 1);

    int success_read_count = fread(base64signature, 1, length, signature_file);
    base64signature[success_read_count] = '\0';
    printf("success_read_count is %d\n", success_read_count);

    fclose(signature_file);

    printf("base64signautre: {%s}\n", base64signature);
    
    unsigned char* signature;
    Base64Decode(base64signature, &signature, signatureLength);

    free(base64signature);

    return signature;
}

int main(int argc, char *argv[]){
    if(argc != 4){
        printf("Usage: [raw_file_name] [sign_file_name] [publickey_file_name]");
        return 1;
    }


    printf("Going to read hash...\n");

    // if(read_file_as_hash(argv[1]) != 0){
    //     // https://stackoverflow.com/questions/2262386/generate-sha256-with-openssl-and-c
    //     printf("File(as hash): %s cannot be read.\n", argv[1]);
    //     return 1;
    // }

    if(read_raw_file(argv[1]) != 0){
        printf("read_raw_file: %s cannot be read.\n", argv[1]);
        return 1;
    }

    processed_img_str = (char*) malloc(image_height * image_width * 3 + 1);
    size_t len_of_processed_img_str = pixels_to_linked_pure_str(image_pixels, image_width * image_height, processed_img_str);

    printf("len_of_processed_img_str: %zu\n", len_of_processed_img_str);
    if(str_to_hash(processed_img_str, len_of_processed_img_str, hash_of_file) != 0){
        printf("str_to_hash: %s cannot be read.\n", argv[1]);
        return 1;
    }

    printf("Going to read public key...\n");

    if(read_rsa_pub_key(argv[3]) != 0){
        printf("Publickey file: %s cannot be read.\n", argv[3]);
        return 1;
    }

    printf("Going to read signature...\n");

    if(read_signature(argv[2]) != 0){
        printf("signature file: %s cannot be read.\n", argv[2]);
	    EVP_PKEY_free(evp_pkey);
        return 1;
    }

    cout << "base64signature: " << base64signature << endl;

    printf("Going to verify signature...\n");

    if(verify_signature() != 1){
        printf("signautre verfied failed.\n");
        free(base64signature);
        EVP_PKEY_free(evp_pkey);
        return 1;
    }

    // // Read Signature
    // unsigned char* raw_signature;
    // size_t raw_signature_length;

    // raw_signature = read_signature_n("../data/out_raw_sign/camera_sign_0", &raw_signature_length);

    // printf("base64signature: %s\n", base64signature);
    // unsigned char* encMessage;
    // size_t encMessageLength;
    // Base64Decode(base64signature, &encMessage, &encMessageLength);

    // cout << "(lalala)size of raw signature is: " << (int)encMessageLength << endl;
    // cout << "(lalala)signature: " << (char*)encMessage << endl;

    // cout << "(outside enclave)size of raw signature is: " << (int)raw_signature_length << endl;
    // cout << "(outside enclave)signature: " << (char*)raw_signature << endl;

    return 0;
}