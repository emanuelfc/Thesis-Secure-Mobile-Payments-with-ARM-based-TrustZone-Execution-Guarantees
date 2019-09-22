#ifndef TA_TRUSTED_DATA_STORE_H
#define TA_TRUSTED_DATA_STORE_H

/*
 * This UUID is generated with uuidgen, version 4
 * the ITU-T UUID generator at http://www.itu.int/ITU-T/asn1/uuid.html
 */
#define TA_TRUSTED_DATA_STORE_UUID \
	{ 0xa404e568, 0x54cc, 0x4c6f, \
		{ 0xa6, 0xf0, 0x60, 0x29, 0x96, 0x11, 0xe3, 0x0d} }


/* The function IDs implemented in this TA */

#define TA_TDS_CONTAINS_KEY	4
#define TA_TDS_CONTAINS_FIELD	5
#define TA_TDS_GET		0
#define TA_TDS_INSERT		1
#define TA_TDS_DELETE_FIELD	2
#define TA_TDS_DELETE_KEY	3

#endif /*TA_TRUSTED_DATA_STORE_H*/
