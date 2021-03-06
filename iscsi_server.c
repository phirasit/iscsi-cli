#include "iscsi_buffer.h"
#include "iscsi_connection.h"
#include "iscsi_pdu.h"
#include "iscsi_server.h"

#include "request/cmd.h"
#include "request/data_out.h"
#include "request/login.h"
#include "request/logout.h"
#include "request/nop.h"
#include "request/reject.h"
#include "request/text.h"

#include "iscsi_logger.h"

static int valid_command_numbering(struct iSCSIConnection* connection, byte* buffer) {
  struct iSCSISession* session = connection->session_reference;
  if (session == NULL) return 1;
  if (iscsi_pdu_has_cmdSN(buffer)) return 1; // NO cmdSN

  int cmdSN = iscsi_pdu_cmdSN(buffer);
  
  if (session->command_numbering_start) {
    if (cmdSN != session->ExpCmdSN) {
      return 0;
    }
  } else {
    session->ExpCmdSN = cmdSN;
    session->command_numbering_start = 1;
  }


  int opcode = iscsi_pdu_opcode(buffer);
  if (opcode == LOGOUT || opcode == TEXT || opcode == SCSI_CMD || opcode == REJECT) {
    if (!iscsi_pdu_immediate(buffer)) {
      ++session->ExpCmdSN;
    }
  }

  return 1;
}

// TODO add other responses here
int iscsi_server_process(
  byte* request,
  struct iSCSIConnection* connection,
  struct iSCSIBuffer* response
) {
  logger("[SERVER] Start PDU Processing\n");
  logger("[SERVER] PDU size = %d\n", iscsi_pdu_length(request));
  if (!valid_command_numbering(connection, request)) {
    // ignore this PDU
    logger("[SERVER] PDU IGNORE\n");
    return PDU_IGNORE;
  }

  // not login
  if (connection->session_reference == NULL || !connection->session_reference->is_full_feature_phase) {
    logger("[SERVER] Session has NOT been logined\n");
    if (iscsi_pdu_opcode(request) == LOGIN) {
      return iscsi_request_login_process(request, connection, response);
    }
    if (connection->session_reference == NULL) {
      return SOCKET_TERMINATE;
    }
    return iscsi_request_login_reject(request, connection, response);
  }

  // has login
  logger("[SERVER] Session has been logined\n");
  logger("[SERVER] PDU opcode = %d\n", iscsi_pdu_opcode(request));
  switch (iscsi_pdu_opcode(request)) {
    case LOGOUT:
      return iscsi_request_logout_process(request, connection, response);
    case TEXT:
      return iscsi_request_text_process(request, connection, response);
  }

  if (connection->session_reference->is_discovery) {
    return iscsi_request_reject(connection, PROTOCOL_ERROR, NULL, 0, response);
  }

  switch (iscsi_pdu_opcode(request)) {
    case NOP_OUT:
      return iscsi_request_nop_out_process(request, connection, response);
    case SCSI_DATA_OUT:
      return iscsi_request_data_out_process(request, connection, response);
    case SCSI_CMD:
      return iscsi_request_cmd_process(request, connection, response);
    case LOGIN:
      return iscsi_request_reject(connection, PROTOCOL_ERROR, NULL, 0, response);
    default:
      return iscsi_request_reject(connection, CMD_NOT_SUPPORTED, NULL, 0, response);
  }
}

