#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdio.h>
#include <string.h>
#include <err.h>

#define		E_OK		0 
#define		E_PARAM		1 /* Wrong Parameter */
#define		E_TEEC		2 /* TEE Client Error */
#define		E_STMM_PLD_SZ		3 /* TBD */


typedef uint32_t ret_code;

#endif /* GLOBAL_H */
