#include "stmm.h"
#include <tee_client_api.h>

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


typedef struct {
	TEEC_Context	tee_ctx;
	TEEC_Session	tee_sess;
	UINTN 		max_buffer_size;	/* comm + var + func + data */
	UINTN 		max_payload_size;	/* func + data */
	uint8_t		init;
}stmm_context;

static stmm_context stmm_ctx;

static EFI_GUID mm_var_guid = {
	0xed32d533,
	0x99e6,
	0x4209,
	{
		0x9c, 0xc0, 0x2d, 0x72, 0xcd, 0xd9, 0x98, 0xa7
	}
};

static uint32_t stmm_get_comm_buf_size(uint8_t func)
{
	uint32_t comm_buf_size = 0;

	switch (func)
	{
	case SMM_VARIABLE_FUNCTION_GET_PAYLOAD_SIZE:
		comm_buf_size = MM_COMMUNICATE_HEADER_SIZE + 
				MM_VARIABLE_COMMUNICATE_SIZE + 
				sizeof(smm_variable_payload_size);
		break;

	default:
		break;
	}

	return comm_buf_size;
}

static uint32_t stmm_setup_comm_buf_hdr(void *comm_buf,
				 uint32_t size,
				 uint8_t func,
				 void **payload_ptr)
{
	efi_mm_communicate_header 	*mm_hdr;
	smm_variable_communicate_header *var_hdr;

	/* Size must be greater than header size*/
	if (size <= MM_COMMUNICATE_HEADER_SIZE + 
		    MM_VARIABLE_COMMUNICATE_SIZE)
		return E_PARAM;

	memset(comm_buf, 0, size);

	mm_hdr = (efi_mm_communicate_header *)comm_buf;
	memcpy(&mm_hdr->header_guid, &mm_var_guid, sizeof(EFI_GUID));
	mm_hdr->message_len = size - MM_COMMUNICATE_HEADER_SIZE;

	var_hdr = (smm_variable_communicate_header *)mm_hdr->data;
	var_hdr->function = func;
	if (payload_ptr)
		*payload_ptr = var_hdr->data;

	return E_OK;
}

static unsigned int stmm_get_ret_status(void *comm_buf)
{
	efi_mm_communicate_header 	*mm_hdr;
	smm_variable_communicate_header *var_hdr;

	mm_hdr = (efi_mm_communicate_header *)comm_buf;
	var_hdr = (smm_variable_communicate_header *)mm_hdr->data;

	return var_hdr->ret_status;
}


static ret_code stmm_get_max_payload()
{
	TEEC_Result res;
	TEEC_Operation op;
	TEEC_SharedMemory shm_mm = {0};
	TEEC_Context *ctx = &stmm_ctx.tee_ctx;
	TEEC_Session *sess = &stmm_ctx.tee_sess;
	smm_variable_payload_size *payload;
	uint32_t err_origin;
	ret_code ret;

	shm_mm.size = stmm_get_comm_buf_size(
		SMM_VARIABLE_FUNCTION_GET_PAYLOAD_SIZE);
	shm_mm.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;

	res = TEEC_AllocateSharedMemory(ctx, &shm_mm);
	if (res != TEEC_SUCCESS) {
		printf("TEEC_AllocateSharedMemory for shm_mm failed with code 0x%x",
			res);
		ret = E_TEEC;
		goto out;
	}

	stmm_setup_comm_buf_hdr(shm_mm.buffer,
				shm_mm.size,
				SMM_VARIABLE_FUNCTION_GET_PAYLOAD_SIZE,
				(void **)&payload);

	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, 
					TEEC_VALUE_OUTPUT,
					TEEC_NONE, TEEC_NONE);
	op.params[0].memref.parent = &shm_mm;
	op.params[0].memref.size = shm_mm.size;

	printf("Invoking TA to get max payload\n");
	res = TEEC_InvokeCommand(sess, PTA_STMM_CMDID_COMMUNICATE, &op,
				 &err_origin);
	if (res != TEEC_SUCCESS) {
		printf("TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
			res, err_origin);
		ret = E_TEEC;
		goto release;
	}


	printf("TA returns status: 0x%08x\n", stmm_get_ret_status(shm_mm.buffer));
	printf("TA returns value: 0x%08x\n", op.params[0].value.a);
	printf("TA returns max payload size: %u\n", payload->size);

	stmm_ctx.max_payload_size = payload->size - 2;
	stmm_ctx.max_buffer_size =  MM_COMMUNICATE_HEADER_SIZE +
				      MM_VARIABLE_COMMUNICATE_SIZE +
				      stmm_ctx.max_payload_size

	ret = E_OK;
release:
	TEEC_ReleaseSharedMemory(&shm_mm);
out:
	return ret;
}

ret_code stmm_init()
{
	TEEC_Result res;
	TEEC_Context *ctx = &stmm_ctx.tee_ctx;
	TEEC_Session *sess = &stmm_ctx.tee_sess;
	TEEC_UUID uuid = TA_SMM_UUID;
	uint32_t err_origin;

	if (stmm_ctx.init)
		return E_OK;

	res = TEEC_InitializeContext(NULL, ctx);
	if (TEEC_SUCCESS != res) {
		printf("TEEC_InitializeContext failed with code 0x%x\n",
		       res);
		return E_TEEC;
	}

	res = TEEC_OpenSession(ctx, sess, &uuid, TEEC_LOGIN_PUBLIC,
			       NULL, NULL, &err_origin);
	if (TEEC_SUCCESS != res) {
		printf("TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, err_origin)
		TEEC_FinalizeContext(ctx);
		return E_TEEC;
	}
	
	if (E_OK != stmm_get_max_payload())
	{
		TEEC_CloseSession(sess);
		TEEC_FinalizeContext(ctx);
		return E_STMM_PLD_SZ;
	}

	stmm_ctx.init = 1;

	return E_OK;
}

void stmm_destroy() {
	if (!stmm_ctx.init)
		return;
	
	TEEC_CloseSession(&stmm_ctx.tee_sess);
	TEEC_FinalizeContext(&stmm_ctx.tee_ctx);
	stmm_ctx.init = 0;

	return;
}






