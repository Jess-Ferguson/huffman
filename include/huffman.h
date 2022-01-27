/* 
 *	Filename:	huffman.h
 *	Author:	 	Jess Ferguson
 *	Date:		27/01/22
 *	Licence:	GNU GPL V3
 *
 *	Encode and decode a byte stream using Huffman coding
 *
 *	Return/exit codes:
 *		EXIT_SUCCESS	- No error
 *		MEM_ERROR		- Memory allocation error
 *		INPUT_ERROR		- No input given
 *
 *	Interface Functions:
 *		- huffman_encode()	- Encodes a string using Huffman coding. Returns the size of the compressed data or an error code.
 *		- huffman_decode()	- Decodes a Huffman encoded string. Returns the size of the decompressed data or an error code.
 *
 */

#ifndef HUFFMAN_H
#define HUFFMAN_H

/* Header files */

#include <stdint.h>

/* Return values */

#define EXIT_SUCCESS 0
#define MEM_ERROR -1
#define INPUT_ERROR -2

/* Interface Functions */

int huffman_decode(const uint8_t * input, uint8_t ** output);
int huffman_encode(const uint8_t * input, uint8_t ** output, const uint32_t decompressed_length);

#endif
