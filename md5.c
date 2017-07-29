#include <openssl/evp.h>
#include <stdio.h>

static char input[65536];
static char hashtext[(EVP_MAX_MD_SIZE*2)+1];

char* md5(char* pathname)
{
    FILE* file = NULL;
    EVP_MD_CTX *ctx = NULL;
    unsigned char output[EVP_MAX_MD_SIZE];
    unsigned int outlen = 0;
    int i;

    file = fopen(pathname,"rb");
    if (file == NULL)
        return "";

    ctx = EVP_MD_CTX_create();
    
    EVP_DigestInit(ctx,EVP_md5());

    while ((i = fread(input,1,sizeof input,file)) > 0)
        EVP_DigestUpdate(ctx,input,i);
    fclose(file);

    EVP_DigestFinal(ctx,output,&outlen);

    EVP_MD_CTX_destroy(ctx);
    ctx = NULL;

    for (i = 0; i < outlen; i++)
        sprintf(hashtext+(i*2),"%02x",output[i]);
    return hashtext;
}
