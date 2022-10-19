

#include "stmm-helper.h"

uint32_t stmm_get_comm_buf_size(uint8_t func)
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

uint32_t stmm_setup_comm_buf_hdr(void *comm_buf,
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