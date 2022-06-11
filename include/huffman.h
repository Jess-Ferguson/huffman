/* 
 *	Filename:	huffman.h
 *	Author:	 	Jess Ferguson
 *	Date:		11/06/22
 *	Licence:	GNU GPL V3
 *
 *	Encode and decode a byte stream using Huffman coding
 *
 *	Return/exit codes:
 *		EXIT_SUCCESS	- No error
 *		MEM_ERROR		- Memory allocation error
 *		INPUT_ERROR		- Input buffer is empty or produces an encoding tree deeper than MAX_CODE_LENGTH
 *		LENGTH_ERROR	- Length of the decoding buffer is less than the length required to decode the input
 *
 *	Interface Functions:
 *		- huffman_encode()						- Encodes a string using Huffman coding. Returns the size of the compressed data or an error code.
 *		- huffman_decode()						- Decodes a Huffman encoded string. Returns the size of the decompressed data or an error code.
 *		- huffman_decode_to_existing_buffer()	- Decode a Huffman encoded string to a pre-allocated buffer.
 *
 */

#ifndef HUFFMAN_H
#define HUFFMAN_H

/* Header files */

#include <stdint.h>

/* Return values */

#define MEM_ERROR		-1
#define INPUT_ERROR		-2
#define LENGTH_ERROR	-3

/* Interface Functions */

int huffman_decode(const uint8_t * input, uint8_t ** output);
int huffman_encode(const uint8_t * input, uint8_t ** output, const uint32_t decompressed_length);

int huffman_decode_to_existing_buffer(const uint8_t * input, uint8_t * output, const uint32_t output_length);

#endif
