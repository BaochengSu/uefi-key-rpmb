
#include "global.h"

#include <efi/efi.h>
#include <efi/efilib.h>

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

#include "stmm-helper.h"


static void get_max_payload(TEEC_Context *ctx, TEEC_Session *sess)
{
	TEEC_Result res;
	TEEC_Operation op;
	TEEC_SharedMemory shm_mm = {0};
	efi_mm_communicate_header *mm_hdr;
	smm_variable_communicate_header *var_hdr;
	smm_variable_payload_size *payload;
	uint32_t err_origin;

	shm_mm.size = stmm_get_comm_buf_size(SMM_VARIABLE_FUNCTION_GET_PAYLOAD_SIZE);
	shm_mm.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;

	res = TEEC_AllocateSharedMemory(ctx, &shm_mm);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_AllocateSharedMemory for shm_mm failed with code 0x%x",
			res);

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
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
			res, err_origin);

	printf("TA returns status: 0x%08x\n", stmm_get_ret_status(shm_mm.buffer));
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
