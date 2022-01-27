/* 
 *	Filename:	huffman.h
 *	Author:	 	Jess Ferguson
 *	Date:		17/07/20
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
 *		- huffman_encode()	- Encodes a string using Huffman coding
 *		- huffman_decode()	- Decodes a Huffman encoded string 
 *
 */

#ifndef HUFFMAN_H
#define HUFFMAN_H

/* Header files */

#include <stdint.h>

/* Return values */

#define EXIT_SUCCESS 0
#define MEM_ERROR 1
#define INPUT_ERROR 2

/* Interface Functions */

int huffman_decode(const uint8_t * input, uint8_t ** output);
int huffman_encode(const uint8_t * input, uint8_t ** output, const uint32_t decompressed_length, uint32_t * compressed_length);

#endif
