/* 
 *	Filename:	huffman.c
 *	Author:	 	Jess Ferguson
 *	Date:		17/07/20
 *	Licence:	GNU GPL V3
 *
 *	Encode and decode a byte stream using Huffman coding
 *
 *	Internal Functions:
 *
 *		Encoding:
 *			- create_huffman_tree()		- Generate a Huffman tree from a frequency analysis
 *			- create_encoding_table()	- Generate a "code array" from the huffman tree, used for fast encoding
 *			- node_compare()			- Calculate the difference in frequency between two nodes
 *			- create_byte_node()		- Generate a byte node
 *			- create_internal_node()	- Generate an internal node
 *			- destroy_huffman_tree()	- Traverses a Huffman tree and frees all memory associated with it
 *			- write_k_bits()			- Write an arbitrary number of bits to a buffer
 *
 *		Decoding:
 *			- peek_buffer()	- Read a two bytes from a buffer at any given bit offset
 *
 *	Data structures:
 *
 *		Encoding table:
 *			- Fast way to encode data using the information generated from a Huffman tree and an easy way to store a representation of the tree
 *			- Table representing each byte to be encoded and how it is encoded allowing for O(1) time to determine how a given byte is encoded
 *			- Position in the table (i.e. encoding_table[0-255]) represents the byte to be encoded or an encoded byte
 *
 *		Decoding table:
 *			- Similar to the encoding table, it's provides a fast way to decode data using the information retrieved from the header and is an easy way to store a representation of the tree
 *			- Two bytes at a time are read from the input and only the lower bits will represent the current encoded character
 *			- This means that no matter what the upper bits are, the table is set up so that any two bytes that has in its lowest bits a valid encoding representation will always give the correct encoding character 
 *			- Position in the table (i.e. decoding_table[0-65536]) represents the byte to be encoded or an encoded byte
 *
 *		Huffman tree:
 *			- Binary tree that operates much like any other Huffman tree
 *			- Contains two types of nodes, internal nodes and byte nodes
 *			- Every node contains either the frequency of the byte it represents if it is a byte node or the combined frequencies of its child nodes if it is an internal node
 *
 *	Encoded data format:
 *
 *		- Header
 *			- Compressed string length (1x uint32_t)
 *			- Decompressed string length (1x uint32_t)
 *			- Header size (1x uint16_t)
 *			- Huffman tree stored as an encoding table (16 + (number of bits representing the encoded byte) bits per byte. Consists of the unencoded byte, the bit length of encoded representation, and the encoded representation)
 *		- Encoded data
 *
 *	The future:
 *		- Find ways to reduce header size
 * 			- Possibly using the huffman algorithm twice to encode the header?
 *			- Will look into canonical Huffman Coding to reduce codebook size
 *		- Combine with duplicate string removal and make full LZW compression
 *
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h> /* Internally includes stddef.h for size_t */
#include <string.h>

#include "huffman.h"

#define INTERNAL_NODE 0 /* Identifiers for determining what type of node a node in a Huffman Tree is */ 
#define BYTE_NODE 1

#define HEADER_BASE_SIZE 6 /* Size of the header with no encoding representations stored */

#define MAX_CODE_LEN 16 /* The longest any encoded representation is allowed to be */

#define MAX_INPUT_SET_SIZE 1 << 8 /* The input can contain at most 256 (1 << 8) unique bytes */ 
#define ENCODING_TABLE_LENGTH 1 << 8
#define DECODING_TABLE_LENGTH 1 << 16

/* Huffman Tree node */

typedef struct huffman_node_t {
	size_t freq;
	union {
		struct huffman_node_t * child[2];
		uint8_t c;
	};
} huffman_node_t;

/* Lookup table used for encoding and decoding */

typedef struct huffman_coding_table_t {
	union {
		uint16_t code;
		uint8_t symbol;
	};
	uint8_t length;
} huffman_coding_table_t;

/* Internal encoding functions */

static huffman_node_t * create_byte_node(const uint8_t c, const size_t freq)
{
	huffman_node_t * node;

	if(!(node = malloc(sizeof(huffman_node_t))))
		return NULL;

	node->freq = freq;
	node->child[0] = NULL;
	node->child[1] = NULL;
	node->c = c;

	return node;
}

static huffman_node_t * create_internal_node(huffman_node_t * first_child, huffman_node_t * second_child)
{
	huffman_node_t * node;

	if(!(node = malloc(sizeof(huffman_node_t))))
		return NULL;

	node->freq = first_child->freq + second_child->freq;
	node->child[0] = first_child;
	node->child[1] = second_child;

	return node;
}

static int node_compare(const void * first_node, const void * second_node)
{
	huffman_node_t * first	= *(huffman_node_t **)first_node;
	huffman_node_t * second	= *(huffman_node_t **)second_node;

	if(!(first->freq - second->freq)) {
		if(first->child[1] && !second->child[1]) {
			return 1;
		} else if(!first->child[1] && second->child[1]) {
			return -1;
		} else {
			return 0;
		}
	} else {
		return first->freq - second->freq;
	}
}

static int create_huffman_tree(const size_t * freq, huffman_node_t ** head_node)
{
	huffman_node_t * node_list[MAX_INPUT_SET_SIZE] = { NULL };
	huffman_node_t * internal_node;
	huffman_node_t ** node_list_p;
	size_t node_count = 0;

	for(size_t i = 0; i < MAX_INPUT_SET_SIZE; i++)
		if(freq[i] && !(node_list[node_count++] = create_byte_node((uint8_t)i, freq[i])))
			return MEM_ERROR;

	node_list_p = node_list;

	for(; node_count > 1; node_count--) {
		qsort(node_list_p, node_count, sizeof(huffman_node_t *), node_compare);

		if(!(internal_node = create_internal_node(node_list_p[0], node_list_p[1])))
			return MEM_ERROR;

		node_list_p[0] = NULL;
		node_list_p[1] = internal_node;

		node_list_p++;
	}

	*head_node = node_list_p[0];

	return EXIT_SUCCESS;
}

static void create_encoding_table(const huffman_node_t * node, huffman_coding_table_t encoding_table[MAX_INPUT_SET_SIZE], uint8_t bits_set)
{
	static size_t value = '\0';

	if(node->child[1]) {
		value &= ~(0x1 << bits_set);
		create_encoding_table(node->child[0], encoding_table, bits_set + 1);
		value |= 0x1 << bits_set;
		create_encoding_table(node->child[1], encoding_table, bits_set + 1);
	} else {
		encoding_table[node->c].code = value & ((1U << bits_set) - 1);
		encoding_table[node->c].length = bits_set;
	}
}

static void destroy_huffman_tree(huffman_node_t * node)
{
	if(node->child[1]) {
		destroy_huffman_tree(node->child[0]);
		destroy_huffman_tree(node->child[1]);
	}

	free(node);

	return;
}

static inline void write_k_bits(uint8_t * buffer, uint16_t value, size_t * bit_pos, const uint8_t bits)
{
	size_t byte_pos = *bit_pos >> 3;
	size_t bit_offset = *bit_pos & 7;
	size_t bits_to_first_byte = 8 - bit_offset;
	size_t extra_bytes_needed = ((bit_offset + bits) >> 3) - (bit_offset >> 3);

	buffer[byte_pos] &= ~0 >> bits_to_first_byte; /* Clear the top n bits of the first byte we're writing to */
	buffer[byte_pos] |= value << bit_offset; /* Shift `value` so that the largest relevant bit is in the MSB position and write as many bits as we can to the first byte of the buffer */

	if(extra_bytes_needed > 0) {
		value >>= bits_to_first_byte; /* Shift `value` such that the relevant bits can be written to the buffer */
		buffer[byte_pos + 1] &= 0; /* Clear the next byte */
		buffer[byte_pos + 1] |= value; /* Write the next 8 bits of `value` to the buffer */

		if(extra_bytes_needed > 1) {
			value >>= 8;
			buffer[byte_pos + 2] &= 0;
			buffer[byte_pos + 2] |= value; /* Write the remainder of `value` to the buffer */
		}
	}

	*bit_pos += bits;
}

/* Internal decoding functions */

static inline uint16_t peek_buffer(const uint8_t * input, const size_t bit_pos)
{
	size_t byte_pos = bit_pos >> 3;
	uint32_t concat = (input[byte_pos + 2] << 0x10) | (input[byte_pos + 1] << 0x8) | input[byte_pos];

	return concat >> (bit_pos & 7); /* Concatenate three successive bytes together and return a two bytes at the calculated bit offset */
}

/* Interface functions */

int huffman_encode(const uint8_t * input, uint8_t ** output, const uint32_t decompressed_length, uint32_t * compressed_length)
{
	size_t freq[MAX_INPUT_SET_SIZE] = { 0 };
	size_t encoded_bytes = 0;

	/* Frequency analysis */

	if(decompressed_length > MAX_INPUT_SET_SIZE) {
		for(size_t i = 0; i < decompressed_length; i++) {
			freq[input[i]]++;
		}
		
		for(size_t i = 0; i < MAX_INPUT_SET_SIZE; i++) {
			if(freq[i]) {
				encoded_bytes++;
			}
		}
	} else {
		for(size_t i = 0; i < decompressed_length; i++) {
			if(!freq[input[i]]) {
				encoded_bytes++;
			}

			freq[input[i]]++;
		}
	}

	/* Handle strings with either one unique byte or zero bytes */

	if(!encoded_bytes)
		return INPUT_ERROR;

	if(encoded_bytes == 1) {
		for(size_t i = 0; i < MAX_INPUT_SET_SIZE; i++) {
			if(freq[i]) {
				freq[i > 0 ? i - 1 : i + 1]++;
			}
		}
	}

	/* Construct a Huffman tree from the frequency analysis */

	huffman_node_t * head_node = NULL;

	if(create_huffman_tree(freq, &head_node) != EXIT_SUCCESS)
		return MEM_ERROR;

	huffman_coding_table_t encoding_table[ENCODING_TABLE_LENGTH] = {{ .code = 0, .length = 0 }};

	/* Convert the tree to a lookup table */

	create_encoding_table(head_node, encoding_table, 0);
	destroy_huffman_tree(head_node);

	uint16_t header_bit_length = 0;

	/* Use the generated encoding table to calculate the byte length of the output */

	for(size_t i = 0; i < ENCODING_TABLE_LENGTH; i++)
		if(encoding_table[i].length)
			header_bit_length += 16 + encoding_table[i].length;

	size_t encoded_bit_length = 0;

	for(size_t i = 0; i < decompressed_length; i++)
		encoded_bit_length += encoding_table[input[i]].length;

	size_t total_length = HEADER_BASE_SIZE + ((encoded_bit_length + header_bit_length + 7) >> 3) + 1; /* Fast division by 8, add one if there's a remainder */

	if(!(*output = calloc(total_length, sizeof(uint8_t))))
		return MEM_ERROR;

	if(compressed_length != NULL)
		*compressed_length = total_length;

	/* Write header information */

	((uint32_t *)(*output))[0] = decompressed_length;
	((uint16_t *)(*output))[2] = header_bit_length;

	size_t bit_pos = HEADER_BASE_SIZE << 3;

	/* Store the encoding information */

	for(size_t i = 0; i < ENCODING_TABLE_LENGTH; i++) {
		if(encoding_table[i].length) {
			write_k_bits(*output, i, &bit_pos, 8);
			write_k_bits(*output, encoding_table[i].length, &bit_pos, 8);
			write_k_bits(*output, encoding_table[i].code, &bit_pos, encoding_table[i].length);
		}
	}

	/* Encode output stream */

	for(size_t i = 0; i < decompressed_length; i++)
		write_k_bits(*output, encoding_table[input[i]].code, &bit_pos, encoding_table[input[i]].length);

	return EXIT_SUCCESS;
}

int huffman_decode(const uint8_t * input, uint8_t ** output)
{
	size_t bit_pos = HEADER_BASE_SIZE << 3;
	huffman_coding_table_t decoding_table[DECODING_TABLE_LENGTH] = {{ .symbol = 0, .length = 0 }};

	/* Extract header information */

	uint32_t decompressed_length = * (uint32_t *) &input[0];
	uint16_t header_bit_length = * (uint16_t *) &input[4] + (HEADER_BASE_SIZE << 3);

	/* Build decoding lookup table */

	while(bit_pos < header_bit_length) {
		uint8_t decoded_byte = peek_buffer(input, bit_pos);

		bit_pos += 8;

		uint8_t encoded_length = peek_buffer(input, bit_pos) & 15;

		if(!encoded_length)
			encoded_length = 16;

		bit_pos += 8;

		uint8_t pad_length = MAX_CODE_LEN - encoded_length;
		uint16_t encoded_byte = peek_buffer(input, bit_pos) & ((1U << encoded_length) - 1); /* Trim all leading bits */

		bit_pos += encoded_length;

		uint16_t padmask = (1U << pad_length) - 1;

		for(uint16_t padding = 0; padding <= padmask; padding++)
			decoding_table[encoded_byte | (padding << encoded_length)] = (huffman_coding_table_t) { .symbol = decoded_byte, .length = encoded_length };
	}

	if(!(*output = calloc(decompressed_length + 1, sizeof(uint8_t))))
		return MEM_ERROR;

	/* Decode input stream */

	for(uint32_t byte_count = 0; byte_count < decompressed_length; byte_count++) {
		uint16_t buffer = peek_buffer(input, bit_pos);

		(*output)[byte_count] = decoding_table[buffer].symbol;
		bit_pos += decoding_table[buffer].length;
	}

	(*output)[decompressed_length] = '\0';

	return EXIT_SUCCESS;
}