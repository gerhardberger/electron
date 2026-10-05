#ifndef FE_H
#define FE_H

#include "fixedint.h"


/*
    fe means field element.
    Here the field is \Z/(2^255-19).
    An element t, entries t[0]...t[9], represents the integer
    t[0]+2^26 t[1]+2^51 t[2]+2^77 t[3]+2^102 t[4]+...+2^230 t[9].
    Bounds on each t[i] vary depending on context.
*/


typedef int32_t fe[10];


void fe_0(fe h);
void fe_1(fe h);

void fe_frombytes(fe h, const unsigned char *s);
void fe_tobytes(unsigned char *s, const fe h);

#define FE_TO_BYTES(output, input)                            \
  do {                                                        \
    unsigned char* const fe_output = (output);                \
    const int32_t* const fe_input = (input);                  \
    int32_t h0 = fe_input[0];                                 \
    int32_t h1 = fe_input[1];                                 \
    int32_t h2 = fe_input[2];                                 \
    int32_t h3 = fe_input[3];                                 \
    int32_t h4 = fe_input[4];                                 \
    int32_t h5 = fe_input[5];                                 \
    int32_t h6 = fe_input[6];                                 \
    int32_t h7 = fe_input[7];                                 \
    int32_t h8 = fe_input[8];                                 \
    int32_t h9 = fe_input[9];                                 \
    int32_t q;                                                \
    int32_t carry0;                                           \
    int32_t carry1;                                           \
    int32_t carry2;                                           \
    int32_t carry3;                                           \
    int32_t carry4;                                           \
    int32_t carry5;                                           \
    int32_t carry6;                                           \
    int32_t carry7;                                           \
    int32_t carry8;                                           \
    int32_t carry9;                                           \
    q = (19 * h9 + (((int32_t) 1) << 24)) >> 25;              \
    q = (h0 + q) >> 26;                                       \
    q = (h1 + q) >> 25;                                       \
    q = (h2 + q) >> 26;                                       \
    q = (h3 + q) >> 25;                                       \
    q = (h4 + q) >> 26;                                       \
    q = (h5 + q) >> 25;                                       \
    q = (h6 + q) >> 26;                                       \
    q = (h7 + q) >> 25;                                       \
    q = (h8 + q) >> 26;                                       \
    q = (h9 + q) >> 25;                                       \
    h0 += 19 * q;                                             \
    carry0 = h0 >> 26;                                        \
    h1 += carry0;                                             \
    h0 -= carry0 << 26;                                       \
    carry1 = h1 >> 25;                                        \
    h2 += carry1;                                             \
    h1 -= carry1 << 25;                                       \
    carry2 = h2 >> 26;                                        \
    h3 += carry2;                                             \
    h2 -= carry2 << 26;                                       \
    carry3 = h3 >> 25;                                        \
    h4 += carry3;                                             \
    h3 -= carry3 << 25;                                       \
    carry4 = h4 >> 26;                                        \
    h5 += carry4;                                             \
    h4 -= carry4 << 26;                                       \
    carry5 = h5 >> 25;                                        \
    h6 += carry5;                                             \
    h5 -= carry5 << 25;                                       \
    carry6 = h6 >> 26;                                        \
    h7 += carry6;                                             \
    h6 -= carry6 << 26;                                       \
    carry7 = h7 >> 25;                                        \
    h8 += carry7;                                             \
    h7 -= carry7 << 25;                                       \
    carry8 = h8 >> 26;                                        \
    h9 += carry8;                                             \
    h8 -= carry8 << 26;                                       \
    carry9 = h9 >> 25;                                        \
    h9 -= carry9 << 25;                                       \
    fe_output[0] = (unsigned char) (h0 >> 0);                 \
    fe_output[1] = (unsigned char) (h0 >> 8);                 \
    fe_output[2] = (unsigned char) (h0 >> 16);                \
    fe_output[3] = (unsigned char) ((h0 >> 24) | (h1 << 2));  \
    fe_output[4] = (unsigned char) (h1 >> 6);                 \
    fe_output[5] = (unsigned char) (h1 >> 14);                \
    fe_output[6] = (unsigned char) ((h1 >> 22) | (h2 << 3));  \
    fe_output[7] = (unsigned char) (h2 >> 5);                 \
    fe_output[8] = (unsigned char) (h2 >> 13);                \
    fe_output[9] = (unsigned char) ((h2 >> 21) | (h3 << 5));  \
    fe_output[10] = (unsigned char) (h3 >> 3);                \
    fe_output[11] = (unsigned char) (h3 >> 11);               \
    fe_output[12] = (unsigned char) ((h3 >> 19) | (h4 << 6)); \
    fe_output[13] = (unsigned char) (h4 >> 2);                \
    fe_output[14] = (unsigned char) (h4 >> 10);               \
    fe_output[15] = (unsigned char) (h4 >> 18);               \
    fe_output[16] = (unsigned char) (h5 >> 0);                \
    fe_output[17] = (unsigned char) (h5 >> 8);                \
    fe_output[18] = (unsigned char) (h5 >> 16);               \
    fe_output[19] = (unsigned char) ((h5 >> 24) | (h6 << 1)); \
    fe_output[20] = (unsigned char) (h6 >> 7);                \
    fe_output[21] = (unsigned char) (h6 >> 15);               \
    fe_output[22] = (unsigned char) ((h6 >> 23) | (h7 << 3)); \
    fe_output[23] = (unsigned char) (h7 >> 5);                \
    fe_output[24] = (unsigned char) (h7 >> 13);               \
    fe_output[25] = (unsigned char) ((h7 >> 21) | (h8 << 4)); \
    fe_output[26] = (unsigned char) (h8 >> 4);                \
    fe_output[27] = (unsigned char) (h8 >> 12);               \
    fe_output[28] = (unsigned char) ((h8 >> 20) | (h9 << 6)); \
    fe_output[29] = (unsigned char) (h9 >> 2);                \
    fe_output[30] = (unsigned char) (h9 >> 10);               \
    fe_output[31] = (unsigned char) (h9 >> 18);               \
  } while (0)

void fe_copy(fe h, const fe f);
int fe_isnegative(const fe f);
int fe_isnonzero(const fe f);
void fe_cmov(fe f, const fe g, unsigned int b);
void fe_cswap(fe f, fe g, unsigned int b);

void fe_neg(fe h, const fe f);
void fe_add(fe h, const fe f, const fe g);
void fe_invert(fe out, const fe z);
void fe_sq(fe h, const fe f);
void fe_sq2(fe h, const fe f);
void fe_mul(fe h, const fe f, const fe g);
void fe_mul121666(fe h, fe f);
void fe_pow22523(fe out, const fe z);
void fe_sub(fe h, const fe f, const fe g);

#endif
