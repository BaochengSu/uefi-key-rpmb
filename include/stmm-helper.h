
#ifndef STMM_HELPER_H
#define STMM_HELPER_H

#include "global.h"
#include <efi/efi.h>
#include <efi/efilib.h>


/* For the UUID (found in the TA's h-file(s)) */
#define TA_SMM_UUID { 0xed32d533, 0x99e6, 0x4209, {\
			0x9c, 0xc0, 0x2d, 0x72, 0xcd, 0xd9, 0x98, 0xa7 } }


#define PTA_STMM_CMDID_COMMUNICATE 0

#define SMM_VARIABLE_FUNCTION_GET_VARIABLE  1
/*
 * The payload for this function is
 * SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME.
 */
#define SMM_VARIABLE_FUNCTION_GET_NEXT_VARIABLE_NAME  2
/*
 * The payload for this function is SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE.
 */
#define SMM_VARIABLE_FUNCTION_SET_VARIABLE  3
/*
 * The payload for this function is
 * SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO.
 */
#define SMM_VARIABLE_FUNCTION_QUERY_VARIABLE_INFO  4
/*
 * It is a notify event, no extra payload for this function.
 */
#define SMM_VARIABLE_FUNCTION_READY_TO_BOOT  5
/*
 * It is a notify event, no extra payload for this function.
 */
#define SMM_VARIABLE_FUNCTION_EXIT_BOOT_SERVICE  6
/*
 * The payload for this function is VARIABLE_INFO_ENTRY.
 * The GUID in EFI_SMM_COMMUNICATE_HEADER is gEfiSmmVariableProtocolGuid.
 */
#define SMM_VARIABLE_FUNCTION_GET_STATISTICS  7
/*
 * The payload for this function is SMM_VARIABLE_COMMUNICATE_LOCK_VARIABLE
 */
#define SMM_VARIABLE_FUNCTION_LOCK_VARIABLE   8

#define SMM_VARIABLE_FUNCTION_VAR_CHECK_VARIABLE_PROPERTY_SET  9

#define SMM_VARIABLE_FUNCTION_VAR_CHECK_VARIABLE_PROPERTY_GET  10

#define SMM_VARIABLE_FUNCTION_GET_PAYLOAD_SIZE  11
/*
 * The payload for this function is
 * SMM_VARIABLE_COMMUNICATE_RUNTIME_VARIABLE_CACHE_CONTEXT
 */
#define SMM_VARIABLE_FUNCTION_INIT_RUNTIME_VARIABLE_CACHE_CONTEXT 12

#define SMM_VARIABLE_FUNCTION_SYNC_RUNTIME_CACHE  13
/*
 * The payload for this function is
 * SMM_VARIABLE_COMMUNICATE_GET_RUNTIME_CACHE_INFO
 */
#define SMM_VARIABLE_FUNCTION_GET_RUNTIME_CACHE_INFO  14


/**
 * struct efi_mm_communicate_header - Header used for SMM variable communication

 * @header_guid:  header use for disambiguation of content
 * @message_len:  length of the message. Does not include the size of the
 *                header
 * @data:         payload of the message
 *
 * Defined in EDK2 as EFI_MM_COMMUNICATE_HEADER.
 * To avoid confusion in interpreting frames, the communication buffer should
 * always begin with efi_mm_communicate_header.
 */
typedef struct __packed {
	EFI_GUID header_guid;
	UINTN     message_len;
	UINT8    data[];
}efi_mm_communicate_header;


#define MM_COMMUNICATE_HEADER_SIZE \
	(sizeof(efi_mm_communicate_header))

/**
 * struct smm_variable_communicate_header - Used for SMM variable communication

 * @function:     function to call in Smm.
 * @ret_status:   return status
 * @data:         payload
 *
 * Defined in EDK2 as SMM_VARIABLE_COMMUNICATE_HEADER.
 */
typedef struct  {
	UINTN  function;
	EFI_STATUS ret_status;
	UINT8           data[];
}smm_variable_communicate_header;

#define MM_VARIABLE_COMMUNICATE_SIZE \
	(sizeof(smm_variable_communicate_header))

/**
 * struct smm_variable_payload_size - Used to get the max allowed
 *                                    payload used in StMM.
 *
 * @size:  size to fill in
 *
 * Defined in EDK2 as SMM_VARIABLE_COMMUNICATE_GET_PAYLOAD_SIZE.
 *
 */
typedef struct {
	UINTN size;
}smm_variable_payload_size;

/**
 * struct smm_variable_access - Used to communicate with StMM by
 *                              SetVariable and GetVariable.

 * @guid:         vendor GUID
 * @data_size:    size of EFI variable data
 * @name_size:    size of EFI name
 * @attr:         attributes
 * @name:         variable name
 *
 * Defined in EDK2 as SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE.
 *
 */
struct smm_variable_access {
	EFI_GUID  guid;
	UINTN data_size;
	UINTN name_size;
	uint32_t         attr;
	uint16_t         name[];
};

EFI_GUID mm_var_guid = {
	0xed32d533,
	0x99e6,
	0x4209,
	{
		0x9c, 0xc0, 0x2d, 0x72, 0xcd, 0xd9, 0x98, 0xa7
	}
};

uint32_t stmm_get_comm_buf_size(uint8_t func);
uint32_t stmm_setup_comm_buf_hdr(void *comm_buf, uint32_t size)

#endif /* STMM_HELPER_H */
