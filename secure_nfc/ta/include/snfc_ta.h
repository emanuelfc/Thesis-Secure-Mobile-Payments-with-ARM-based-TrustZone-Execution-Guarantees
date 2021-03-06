#pragma once

#define TA_SNFC_UUID \
	{ 0x7c86cbfa, 0xf01a, 0x4170, \
		{ 0xaa, 0x59, 0xbe, 0x0c, 0x65, 0x3c, 0xdd, 0x6b} }

#define TA_SNFC_SET_SECURITY_PARAMETERS			0
#define TA_SNFC_CREATE_HANDSHAKE_REQUEST		1
#define TA_SNFC_PROCESS_HANDSHAKE_REQUEST		2
#define TA_SNFC_PROCESS_HANDSHAKE_REPLY			3
#define TA_SNFC_PROCESS_AUTHENTICATION_PROOF		4
#define TA_SNFC_CREATE_SNFC_PACKET			5
#define TA_SNFC_PROCESS_SNFC_PACKET			6
#define TA_SNFC_SECURITY_CONTEXT_CLOSE			7

