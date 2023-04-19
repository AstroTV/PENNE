#include <stdbool.h>
extern unsigned char *encryption_key;
extern bool using_encryption;

void handleErrors(void);

int gcm_encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *aad, int aad_len, unsigned char *key, unsigned char *iv, int iv_len,
                unsigned char *ciphertext, unsigned char *tag);

int gcm_decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *aad, int aad_len, unsigned char *tag, unsigned char *key, unsigned char *iv,
                int iv_len, unsigned char *plaintext);
