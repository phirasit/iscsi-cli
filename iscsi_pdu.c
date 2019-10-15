#include "iscsi_pdu.h"

void iscsi_pdu_set_response_header(byte* pdu, struct iSCSIConnection* connection) {
  iscsi_byte_int2byte(pdu + 24, iscsi_connection_stat_sn(connection));
  iscsi_byte_int2byte(pdu + 28, iscsi_connection_exp_cmd_sn(connection));
  iscsi_byte_int2byte(pdu + 32, iscsi_connection_max_cmd_sn(connection));
}

int iscsi_pdu_valid(byte* pdu, int length) {
  if (length < BASIC_HEADER_SEGMENT_LENGTH) return 0;
  return iscsi_pdu_length(pdu) <= length;
}

void iscsi_pdu_pad0(byte* pdu, int length) {
  int pad_size = (iscsi_pdu_padded_length(length) - length) & 0x03;
  memset(pdu + length, 0, pad_size);
}
