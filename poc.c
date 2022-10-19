#include <err.h>
#include <stdio.h>
#include <string.h>
#include <byteswap.h>
#include <efi/efi.h>
#include <efi/efilib.h>

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

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

static EFI_GUID mm_var_guid = {
	0xed32d533,
	0x99e6,
	0x4209,
	{
		0x9c, 0xc0, 0x2d, 0x72, 0xcd, 0xd9, 0x98, 0xa7
	}
};

static void get_max_payload(TEEC_Context *ctx, TEEC_Session *sess)
{
	TEEC_Result res;
	TEEC_Operation op;
	TEEC_SharedMemory shm_mm = {0};
	efi_mm_communicate_header *mm_hdr;
	smm_variable_communicate_header *var_hdr;
	smm_variable_payload_size *payload;
	uint32_t err_origin;

	shm_mm.size = MM_COMMUNICATE_HEADER_SIZE + MM_VARIABLE_COMMUNICATE_SIZE + sizeof(smm_variable_payload_size);
	shm_mm.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;

	printf("shm_mm.size: %u\n", shm_mm.size);

	res = TEEC_AllocateSharedMemory(ctx, &shm_mm);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_AllocateSharedMemory for shm_mm failed with code 0x%x",
			res);

	memset(shm_mm.buffer, 0, shm_mm.size);
	mm_hdr = (efi_mm_communicate_header *)shm_mm.buffer;
	memcpy(&mm_hdr->header_guid, &mm_var_guid, sizeof(EFI_GUID));
	mm_hdr->message_len = MM_VARIABLE_COMMUNICATE_SIZE + sizeof(smm_variable_payload_size);

	var_hdr = (smm_variable_communicate_header *)mm_hdr->data;
	var_hdr->function = SMM_VARIABLE_FUNCTION_GET_PAYLOAD_SIZE;

	payload = (smm_variable_payload_size *)var_hdr->data;

	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, 
					TEEC_VALUE_OUTPUT,
					TEEC_NONE, TEEC_NONE);
	op.params[0].memref.parent = &shm_mm;
	op.params[0].memref.size = shm_mm.size;

	printf("Invoking TA to get max payload\n");
	res = TEEC_InvokeCommand(sess, PTA_STMM_CMDID_COMMUNICATE, &op,
				 &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
			res, err_origin);

	printf("TA returns status: 0x%08x\n", var_hdr->ret_status);
	printf("TA returns value: 0x%08x\n", op.params[0].value.a);
	printf("TA returns max payload size: %u\n", payload->size);

	TEEC_ReleaseSharedMemory(&shm_mm);
}


int main(void)
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_UUID uuid = TA_SMM_UUID;
	uint32_t err_origin;

	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	res = TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, err_origin);

	get_max_payload(&ctx, &sess);

	TEEC_CloseSession(&sess);

	TEEC_FinalizeContext(&ctx);

	return 0;
}
