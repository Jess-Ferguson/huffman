#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "huffman.h"

#define BASE_INPUT_LEN 1024
#define BUFFER_MAX_LEN 65355

typedef struct result {
	int32_t test_num;
	size_t decompressed_length;
	size_t compressed_length;
} result_t;

int compare(uint8_t * first, uint8_t * second, size_t len)
{
	for(size_t i = 0; i < len; i++) {
		if(first[i] < second[i]) {
			return -1;
		} else if(first[i] > second[i]) {
			return 1;
		}
	}

	return 0;
}

void usage(const char * progname)
{
	fprintf(stderr, "%s: [file name]\n", progname);
}

void show_progress(float progress)
{
	size_t width = 70;
	size_t pos = width * progress;

	printf("[+] Test completion: [");

	for(size_t i = 0; i < width; i++) {
		if(i < pos) {
			putchar('=');
		} else if(i == pos) {
			putchar('>');
		} else {
			putchar(' ');
		}
	}

	printf("] %.1f%%\r", progress * 100.0);
	fflush(stdout);
}

void print_result(result_t result)
{
	if(result.test_num >= 0)
		printf("Test number: %d\n", result.test_num);

	printf("Decompressed length: %zu\nCompressed length: %zu\nCompression ratio: %.2lf\n",
		result.decompressed_length,
		result.compressed_length,
		((double)result.compressed_length / result.decompressed_length)
	);
}

int main(int argc, char ** argv)
{
	uint8_t * encoded = NULL;
	uint8_t * decoded = NULL;
	FILE * test_strings_fp = NULL;
	char * test_strings = NULL; /* A series of horrible strings that try and break the compression */
	char ** test_strings_p = NULL; /* A special pointer for doing some magic on test_strings */
	char input_buffer[BUFFER_MAX_LEN];

	size_t failures = 0;
	size_t test_count = 0;
	size_t test_length = 0;
	int32_t compressed_length;
	long test_strings_file_len;

	result_t average = (result_t) {
		.test_num = -1,
		.compressed_length = 0,
		.decompressed_length = 0
	};

	result_t best = (result_t) {
		.test_num = -1,
		.compressed_length = SIZE_MAX,
		.decompressed_length = 0
	};

	result_t worst = (result_t) {
		.test_num = -1,
		.compressed_length = 0,
		.decompressed_length = 0
	};

	if(argc >= 2) {
		printf("[+] Loading tests from \"%s\"\n", argv[1]);

		if(!(test_strings_fp = fopen(argv[1], "r"))) {
			fprintf(stderr, "Error: Could not open file \"%s\"!\n", argv[1]);
			perror("fopen()");
			usage(argv[0]);
			return -1;
		}

		if(fseek(test_strings_fp, 0, SEEK_END) == -1) {
			perror("seek()");
			fclose(test_strings_fp);
			return -1;
		}

		if((test_strings_file_len = ftell(test_strings_fp)) == -1) {
			perror("ftell()");
			fclose(test_strings_fp);
			return -1;
		}

		if(test_strings_file_len == 0) {
			fprintf(stderr, "[-] No tests detected, terminating...\n");
			fclose(test_strings_fp);
			return 0;
		}

		rewind(test_strings_fp);

		if(!(test_strings = malloc((test_strings_file_len + 1) * sizeof(char)))) {  /* The extra byte is so we have somewhere to put a null byte in case the file doesn't end with a new line */
			fprintf(stderr, "Error: Could not allocate memory for test strings!\n");
			perror("malloc()");
			fclose(test_strings_fp);
			return -1;
		}

		test_strings[test_strings_file_len] = '\0';

		fread(test_strings, sizeof(char), test_strings_file_len, test_strings_fp);
		fclose(test_strings_fp);

		for(size_t i = 0; i < test_strings_file_len; i++) {
			if(test_strings[i] == '\n') {
				test_count++;
				test_strings[i] = '\0';
			}
		}

		if(test_strings[test_strings_file_len - 1] != '\n')
			test_count++;
	} else {
		size_t test_strings_length = BASE_INPUT_LEN;
		bool resize_test_strings = true;

		while(fgets(input_buffer, BUFFER_MAX_LEN, stdin)) {
			size_t input_length = strlen(input_buffer);

			while(test_strings_length < test_length + input_length + 1) {
				test_strings_length *= 2; /* Grow the size of test_strings exponentially to reduce overhead from successive reallocations */ 
				resize_test_strings = true;
			}

			if(resize_test_strings) {
				if(!(test_strings = realloc(test_strings, test_strings_length))) {
					fprintf(stderr, "Error: Could not allocate memory for test strings!\n");
					perror("realloc()");
					return -1;
				}

				resize_test_strings = false;
			}
			
			memcpy(test_strings + test_length, input_buffer, input_length + 1);

			test_length += input_length;

			if(input_buffer[input_length - 1] == '\n')
				test_count++;
		}

		if(test_length > 0 && test_strings[test_length - 1] != '\n')
			test_count++;
	}

	if(!test_count) {
		fprintf(stderr, "[-] No tests detected, terminating...\n");
		free(test_strings);
		return 0;
	}

	if(!(test_strings_p = malloc(test_count * sizeof(char *)))) {
		fprintf(stderr, "Error: Could not allocate memory for test strings!\n");
		perror("malloc()");
		free(test_strings);
		return -1;
	}

	test_strings_p[0] = &test_strings[0];

	if(argc >= 2) {
		for(size_t i = 0, cur_test = 1; i < test_strings_file_len && cur_test < test_count; i++) {
			if(test_strings[i] == '\0') {
				test_strings_p[cur_test] = &test_strings[i + 1];
				cur_test++;
			}
		}
	} else {
		for(size_t i = 0, cur_test = 1; i < test_length && cur_test < test_count; i++) {
			if(test_strings[i] == '\n') { /* Tests from stdin are deliniated with newlines */
				test_strings[i] = '\0';
				test_strings_p[cur_test] = &test_strings[i + 1];
				cur_test++;
			}
		}
	}

	printf("[+] Found %zu test strings\n", test_count);

	for(size_t i = 0; i < test_count; i++) {
		show_progress((float)i / test_count);

		test_length = strlen(test_strings_p[i]) + 1;

		if((compressed_length = huffman_encode((uint8_t *)test_strings_p[i], &encoded, test_length)) < 0) {
			fprintf(stderr, "\n[-] Error: Failed to encode test %zu/%zu!\n", i + 1, test_count);
			failures++;
			continue;
		}

		if(huffman_decode(encoded, &decoded) < 0) {
			fprintf(stderr, "\n[-] Error: Failed to decode test %zu/%zu!\n", i + 1, test_count);
			free(encoded);
			failures++;
			continue;
		}
		
		if(!compare((uint8_t *)test_strings_p[i], decoded, test_length)) {
			double compression_ratio = (double)compressed_length / test_length;

			if((best.decompressed_length == 0) || (compression_ratio < (double)best.compressed_length / best.decompressed_length)) {
				best.test_num = i;
				best.decompressed_length = test_length;
				best.compressed_length = compressed_length;
			}

			if((worst.decompressed_length == 0) || (compression_ratio > (double)worst.compressed_length / worst.decompressed_length)) {
				worst.test_num = i;
				worst.decompressed_length = test_length;
				worst.compressed_length = compressed_length;
			}

			average.compressed_length += compressed_length;
			average.decompressed_length += test_length;
		} else {
			fprintf(stderr, "\n[-] Error: Failed comparison on test %zu/%zu!\n", i + 1, test_count);
			failures++;
		}

		free(decoded);
		free(encoded);
	}

	printf("\n[+] Tests complete!\n\nResults:\n\nTests completed: %zu\nSuccessful tests: %zu (%.1f%%)\nFailed tests: %zu (%.1f%%)\n",
		test_count,
		test_count - failures,
		100 * (float) (test_count - failures) / test_count,
		failures,
		100 * (float) failures / test_count
	);

	average.compressed_length /= test_count;
	average.decompressed_length /= test_count;

	printf("\nBest case:\n\n");
	print_result(best);
	printf("\nWorst case:\n\n");
	print_result(worst);
	printf("\nAverage case:\n\n");
	print_result(average);

	free(test_strings_p);
	free(test_strings);

	return 0;
}
