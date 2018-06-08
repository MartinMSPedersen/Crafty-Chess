#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

#ifdef USE_ASSEMBLY_A

/* Diag info offsets */

#  define D_ATTACKS        0    /* Must be 0 - hard-coded (see comments) */

#  define D_MOBILITY       4
#  define D_WHICH_ATTACK   8
#  define D_SHIFT         12
#  define D_MASK          13
#  define AD_SHIFT        14
#  define AD_MASK         15
#  define AD_WHICH_ATTACK 16
#  define AD_MOBILITY     20
#  define AD_ATTACKS      24

/* Position offsets */

#  define W_OCCUPIED        0
#  define B_OCCUPIED        8
#  define RL90             16
#  define RL45             24
#  define RR45             32

/* Struct at offsets:
        struct at {
          unsigned char which_attack[8][64];
          BITBOARD      file_attack_bitboards[8][12]; 
          unsigned char rank_attack_bitboards[8][12]; 
          unsigned char length8_mobility[8][12]; 
          unsigned char short_mobility[116];
        } at;
*/

#  define WHICH_ATTACK       0
#  define FILE_ATTACKS     512
#  define RANK_ATTACKS    1280
#  define LEN8_MOBILITY   1376
#  define SHRT_MOBILITY   1472

/*
  BITBOARD AttacksDiaga1Func (DIAG_INFO *diag, POSITION *board)
*/

__declspec(naked)
BITBOARD __cdecl AttacksDiaga1Func(DIAG_INFO * diag, POSITION * board)
{

  __asm {
    push esi mov esi, 8[esp]    /* diag_info     */
    mov eax, 12[esp]            /* boardp        */
    mov cl, AD_SHIFT[esi]       /* shift         */
    cmp cl, 32 sbb edx, edx mov eax, RL45 + 4[eax + edx * 4]    /* occupied      */
    movzx edx, byte ptr AD_MASK[esi]    /* mask          */
    shr eax, cl and eax, edx add eax, AD_WHICH_ATTACK[esi]      /* which_attack  */
    mov ecx, AD_ATTACKS[esi]    /* attack_vector */
    movzx edx, byte ptr[eax]    /* attack_index  */
    pop esi mov eax,[ecx + edx * 8]
    mov edx, 4[ecx + edx * 8]
 ret}}
/*
  BITBOARD AttacksDiagh1Func(DIAG_INFO *diag, POSITION *board)
*/ __declspec(naked)
BITBOARD __cdecl AttacksDiagh1Func(DIAG_INFO * diag, POSITION * board)
{

  __asm {
    push esi mov esi, 8[esp]    /* diag_info     */
    mov eax, 12[esp]            /* boardp        */
    mov cl, D_SHIFT[esi]        /* shift         */
    cmp cl, 32 sbb edx, edx mov eax, RR45 + 4[eax + edx * 4]    /* occupied      */
    movzx edx, byte ptr D_MASK[esi]     /* mask          */
    shr eax, cl and eax, edx add eax, D_WHICH_ATTACK[esi]       /* which_attack  */
    mov ecx,[esi]
/* D_ATTACKS     */
/* attack_vector */
    movzx edx, byte ptr[eax]    /* attack_index  */
    pop esi mov eax,[ecx + edx * 8]
    mov edx, 4[ecx + edx * 8]
 ret}}
/*
  BITBOARD AttacksBishopFunc(DIAG_INFO *diag, POSITION *board)
*/ __declspec(naked)
BITBOARD __cdecl AttacksBishopFunc(DIAG_INFO * diag, POSITION * board)
{

  __asm {
    push ebx push esi push edi mov esi, 16[esp] /* diag_info     */
    mov edi, 20[esp]            /* boardp        */
    mov cl, byte ptr AD_SHIFT[esi]      /* shift         */
    cmp cl, 32 sbb edx, edx movzx ebx, byte ptr AD_MASK[esi]    /* mask          */
    mov edi, RL45 + 4[edi + edx * 4]    /* occupied      */
    shr edi, cl and edi, ebx add edi, AD_WHICH_ATTACK[esi]      /* which_attack  */
    mov ecx, AD_ATTACKS[esi]    /* attack_vector */
    movzx ebx, byte ptr[edi]    /* attack_index  */
    mov edi, 20[esp]            /* again boardp  */
    lea edx,[ecx + ebx * 8]
    mov cl, D_SHIFT[esi]        /* shift         */
    cmp cl, 32 sbb ebx, ebx mov edi, RR45 + 4[edi + ebx * 4]    /* occupied      */
    movzx ebx, byte ptr D_MASK[esi]     /* mask          */
    shr edi, cl mov ecx,[esi]
/* D_ATTACKS     */
/* attack_vector */
    and edi, ebx mov eax,[edx]
    add edi, D_WHICH_ATTACK[esi]        /* which_attack  */
    mov edx, 4[edx]
    movzx ebx, byte ptr[edi]    /* attack_index  */
    pop edi or eax,[ecx + ebx * 8]
    pop esi or edx, 4[ecx + ebx * 8]
pop ebx ret}}
/*
  unsigned MobilityDiaga1Func(DIAG_INFO *diag, POSITION *board)
*/ __declspec(naked)
unsigned __cdecl MobilityDiaga1Func(DIAG_INFO * diag, POSITION * board)
{

  __asm {
    push esi mov esi, 8[esp]    /* diag_info     */
    mov eax, 12[esp]            /* boardp        */
    mov cl, AD_SHIFT[esi]       /* shift         */
    cmp cl, 32 sbb edx, edx mov eax, RL45 + 4[eax + edx * 4]    /* occupied      */
    movzx edx, byte ptr AD_MASK[esi]    /* mask          */
    shr eax, cl and eax, edx add eax, AD_WHICH_ATTACK[esi]      /* which_attack  */
    mov ecx, AD_MOBILITY[esi]   /* mobility_vector */
    movzx edx, byte ptr[eax]    /* attack_index  */
    pop esi movzx eax, byte ptr[ecx + edx * 1]
 ret}}
/*
  unsigned MobilityDiagh1Func(DIAG_INFO *diag, POSITION *board)
*/ __declspec(naked)
unsigned __cdecl MobilityDiagh1Func(DIAG_INFO * diag, POSITION * board)
{

  __asm {
    push esi mov esi, 8[esp]    /* diag_info     */
    mov eax, 12[esp]            /* boardp        */
    mov cl, D_SHIFT[esi]        /* shift         */
    cmp cl, 32 sbb edx, edx mov eax, RR45 + 4[eax + edx * 4]    /* occupied      */
    movzx edx, byte ptr D_MASK[esi]     /* mask          */
    shr eax, cl and eax, edx add eax, D_WHICH_ATTACK[esi]       /* which_attack  */
    mov ecx, D_MOBILITY[esi]    /* mobility_vector */
    movzx edx, byte ptr[eax]    /* attack_index  */
    pop esi movzx eax, byte ptr[ecx + edx * 1]
 ret}}
/*
  BITBOARD AttacksRankFunc(int square, POSITION *board)
*/ __declspec(naked)
BITBOARD __cdecl AttacksRankFunc(int square, POSITION * board)
{

  __asm {

    mov ecx, 4[esp]             /* square               */
    mov edx, 8[esp]             /* boardp               */
    push esi mov esi, ecx and esi, 7    /* file                 */
     not ecx and ecx, 0x38 push ebp cmp ecx, 32 push edi sbb ebp, ebp lea edi,[esi + esi * 2]   /* file * 3             */
    shl esi, 6                  /* file * 64            */
     mov eax, W_OCCUPIED + 4[edx + ebp * 4]
    inc ecx or eax, B_OCCUPIED + 4[edx + ebp * 4]
    shr eax, cl and eax, 0x3f movzx edx, byte ptr at + WHICH_ATTACK[eax + esi]
    dec ecx movzx eax, byte ptr at + RANK_ATTACKS[edx + edi * 4]
shl eax, cl pop edi mov edx, eax and eax, ebp not ebp and edx,
        ebp pop ebp pop esi ret}}
/*
  BITBOARD AttacksFileFunc(int square, POSITION *board)
*/ __declspec(naked)
BITBOARD __cdecl AttacksFileFunc(int square, POSITION * board)
{

  __asm {
    mov ecx, 4[esp]             /* square               */
    mov edx, 8[esp]             /* boardp               */
    push esi mov esi, ecx not ecx and ecx, 7    /* file                 */
     shr esi, 3                 /* rank                 */
     push edi lea edi,[esi + esi * 2]   /* rank * 3             */
    shl esi, 6                  /* rank * 64            */
     lea ecx, 1[ecx * 8]        /* (file << 3) + 1      */
    cmp ecx, 32 sbb eax, eax shl edi, 5 mov eax, RL90 + 4[edx + eax * 4]
    shr eax, cl and eax, 0x3f movzx edx, byte ptr at + WHICH_ATTACK[eax + esi]
    dec ecx mov eax, at + FILE_ATTACKS[edi + edx * 8]
    shr ecx, 3 mov edx, at + FILE_ATTACKS + 4[edi + edx * 8]
pop edi shl eax, cl pop esi shl edx, cl ret}}
/*
  BITBOARD AttacksRookFunc(int square, POSITION *board)
*/ __declspec(naked)
BITBOARD __cdecl AttacksRookFunc(int square, POSITION * board)
{

  __asm {
    mov ecx, 4[esp]             /* square               */
    mov edx, 8[esp]             /* boardp               */
    push ebp push esi mov esi, ecx not ecx and esi, 7   /* file                 */
     push ebx and ecx, 0x38     /* rank << 3            */
     push edi cmp ecx, 32 lea edi,[esi + esi * 2]       /* file * 3             */
    sbb eax, eax shl esi, 6     /* file * 64            */
     mov ebp, W_OCCUPIED + 4[edx + eax * 4]
    inc ecx or ebp, B_OCCUPIED + 4[edx + eax * 4]
    shr ebp, cl and ebp, 0x3f movzx ebx,
        byte ptr at + WHICH_ATTACK[esi + ebp * 1]
    dec ecx mov esi, 20[esp]    /* square               */
    movzx ebp, byte ptr at + RANK_ATTACKS[ebx + edi * 4]
    shl ebp, cl mov ecx, esi mov ebx, ebp not ecx and ebp, eax and ecx, 7       /* file                 */
     not eax shr esi, 3         /* rank                 */
     and ebx, eax
/* Now we have: ebp - ebx, lo - hi */
     lea ecx, 1[ecx * 8]        /* (file << 3) + 1      */
    lea edi,[esi + esi * 2]     /* rank * 3             */
    shl esi, 6                  /* rank * 64            */
     cmp ecx, 32 sbb eax, eax shl edi, 5 mov eax, RL90 + 4[edx + eax * 4]
    shr eax, cl and eax, 0x3f movzx edx, byte ptr at + WHICH_ATTACK[eax + esi]
    dec ecx mov eax, at + FILE_ATTACKS[edi + edx * 8]
    shr ecx, 3 shl eax, cl mov edx, at + FILE_ATTACKS + 4[edi + edx * 8]
shl edx, cl pop edi or edx, ebx pop ebx or eax, ebp pop esi pop ebp ret}}
/*
  unsigned MobilityRankFunc(int square, POSITION *board)
*/ __declspec(naked)
unsigned __cdecl MobilityRankFunc(int square, POSITION * board)
{

  __asm {
    mov ecx, 4[esp]             /* square               */
    push esi mov esi, ecx not ecx and ecx, 0x38 cmp ecx, 32 sbb edx, edx shl edx, 2 add edx, 12[esp]    /* boardp               */
    and esi, 7                  /* file                 */
     mov eax, W_OCCUPIED + 4[edx]
    inc ecx or eax, B_OCCUPIED + 4[edx]
    shr eax, cl lea ecx,[esi + esi * 2] /* file * 3             */
    shl esi, 6                  /* file * 64            */
     and eax, 0x3f movzx edx, byte ptr at + WHICH_ATTACK[eax + esi]
    pop esi movzx eax, byte ptr at + LEN8_MOBILITY[edx + ecx * 4]
 ret}}
/*
  unsigned MobilityFileFunc(int square, POSITION *board)
*/ __declspec(naked)
unsigned __cdecl MobilityFileFunc(int square, POSITION * board)
{

  __asm {
    mov ecx, 4[esp]             /* square               */
    push esi mov esi, ecx not ecx and ecx, 7    /* file                 */
     shr esi, 3                 /* rank                 */
     lea ecx, 1[ecx * 8]        /* (file << 3) + 1      */
    cmp ecx, 32 sbb edx, edx shl edx, 2 add edx, 12[esp]        /* boardp               */
    mov eax, RL90 + 4[edx]
    shr eax, cl lea ecx,[esi + esi * 2] /* rank * 3             */
    shl esi, 6                  /* rank * 64            */
     and eax, 0x3f movzx edx, byte ptr at + WHICH_ATTACK[eax + esi]
    pop esi movzx eax, byte ptr at + LEN8_MOBILITY[edx + ecx * 4]
 ret}}
#endif                          /* USE_ASSEMBLY_A */
