#include <iostream>
#include <wmmintrin.h>
#include <immintrin.h>
#include <emmintrin.h>
#include <pmmintrin.h>

#define ALIGN(n) __attribute__ ((aligned(n)))
#define pipeline 1

#define EXPAND_ASSIST(v1,v2,v3,v4,shuff_const,aes_const)                    \
    v2 = _mm_aeskeygenassist_si128(v4,aes_const);                           \
    v3 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(v3),              \
                                         _mm_castsi128_ps(v1), 16));        \
    v1 = _mm_xor_si128(v1,v3);                                              \
    v3 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(v3),              \
                                         _mm_castsi128_ps(v1), 140));       \
    v1 = _mm_xor_si128(v1,v3);                                              \
    v2 = _mm_shuffle_epi32(v2,shuff_const);                                 \
    v1 = _mm_xor_si128(v1,v2)

using namespace std;

static void PMAC(unsigned char *K_1, unsigned char *N,unsigned char *M, int size, unsigned char *T);
static void AES_128_Key_Expansion(const unsigned char *userkey, void *key);
static inline void AES_encrypt(__m128i tmp, __m128i *out,__m128i *key, unsigned rounds);
static void imprimiArreglo(int tam, unsigned char *in );

int main(){

    ALIGN(64) unsigned char plaintext[128]=  {
                                             0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
                                             0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
                                             0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
                                             0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
                                             
                                             0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
                                             0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
                                             0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
                                             0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 

                                            };
    ALIGN(16) unsigned char tag[16 ]={ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
    ALIGN(16) unsigned char K_1[16 ]={ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
    ALIGN(16) unsigned char N[16 ]={ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};

    PMAC(K_1, N, plaintext, 64, tag);
    
    printf("\n");
    imprimiArreglo(16, tag);
    return 0;
}


static void PMAC(unsigned char *K_1, unsigned char *N,unsigned char *M, int size, unsigned char *T){

    int m_blocks = 0;
    if (size%16==0)
        m_blocks=(size/16);
    else
        m_blocks=(size/16) + 1;

    __m128i * plain_text = (__m128i*) M;
    __m128i nonce;
    __m128i nonce_temp[1];
    __m128i Tag;
    __m128i S;
    __m128i keys_128[11];
    __m128i keys_0[2];
    __m128i sum_nonce= _mm_set_epi32(0,0,0,1);

    S = _mm_setzero_si128();
    keys_0[0] = _mm_setzero_si128();
    keys_0[1] = _mm_setzero_si128();

    AES_128_Key_Expansion(K_1, keys_128);

    nonce = _mm_setzero_si128();
    nonce = _mm_load_si128((__m128i *)&N[0]);
    
    AES_encrypt(nonce, &nonce, keys_128, 10);


    for (size_t i = 0; i < m_blocks; i++){

        nonce_temp[0]=nonce; 
        
        AES_encrypt(nonce_temp[0], &nonce_temp[0], keys_128, 2);
        
        plain_text[i]=_mm_xor_si128(plain_text[i],nonce_temp[0]);
        
        AES_encrypt(plain_text[i], &plain_text[i], keys_128, 10);

        S=_mm_xor_si128(plain_text[i],S);
        nonce=_mm_add_epi64(nonce, sum_nonce);

    }


    
    Tag=_mm_xor_si128(Tag,S);
    AES_encrypt(Tag, &Tag, keys_128, 10);
	_mm_store_si128 ((__m128i*)T,Tag);
}


static inline void AES_encrypt(__m128i tmp, __m128i *out,__m128i *key, unsigned rounds){
	int j;
	tmp = _mm_xor_si128 (tmp,key[0]);
	for (j=1; j<rounds; j++)  tmp = _mm_aesenc_si128 (tmp,key[j]);
	tmp = _mm_aesenclast_si128 (tmp,key[j]);
	_mm_store_si128 ((__m128i*)out,tmp);
}


static void AES_128_Key_Expansion(const unsigned char *userkey, void *key)
{
    __m128i x0,x1,x2;
    __m128i *kp = (__m128i *)key;
    kp[0] = x0 = _mm_loadu_si128((__m128i*)userkey);
    x2 = _mm_setzero_si128();
    EXPAND_ASSIST(x0,x1,x2,x0,255,1);   kp[1]  = x0;
    EXPAND_ASSIST(x0,x1,x2,x0,255,2);   kp[2]  = x0;
    EXPAND_ASSIST(x0,x1,x2,x0,255,4);   kp[3]  = x0;
    EXPAND_ASSIST(x0,x1,x2,x0,255,8);   kp[4]  = x0;
    EXPAND_ASSIST(x0,x1,x2,x0,255,16);  kp[5]  = x0;
    EXPAND_ASSIST(x0,x1,x2,x0,255,32);  kp[6]  = x0;
    EXPAND_ASSIST(x0,x1,x2,x0,255,64);  kp[7]  = x0;
    EXPAND_ASSIST(x0,x1,x2,x0,255,128); kp[8]  = x0;
    EXPAND_ASSIST(x0,x1,x2,x0,255,27);  kp[9]  = x0;
    EXPAND_ASSIST(x0,x1,x2,x0,255,54);  kp[10] = x0;
}



void imprimiArreglo(int tam, unsigned char *in )
{

    for (int i = 0; i<tam; i++){
        printf("%02x", in[i] );
    }
    printf("\n" );

}