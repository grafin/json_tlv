#include <jp_util.h>
#include <jp_tlv_data.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>

#include <json.h>

enum jp_mode {
	DEFAULT = 0,
	ENCODE,
	DECODE,
};

_Noreturn static void
print_help(const int status, const char *progname)
{
	printf("Usage: %s [OPTIONS]\n", progname);
	printf("Options:\n");
	printf("  -h, --help\t\tPrint this help message\n");
	printf("  -i, --input\t\tInput file\n");
	printf("  -o, --output\t\tOutput file (defaults to stdout)\n");
	printf("  -d, --decode\t\tDecode input file\n");
	printf("  -e, --encode\t\tEncode input file\n");
	exit(status);
}

/* @TODO This should be in a separate file */
static void
encode(json_object *input, FILE *output)
{
	struct json_object *key_map = json_object_new_object();
	/* @TODO check that 32 is enough for any uint64 */
	char buf[32];

	uint64_t key_idx = 1;
	json_object_object_foreach(input, key, val) {
		enum json_type type = json_object_get_type(val);
		struct jp_tlv_data *tlv = NULL;
		switch (type) {
		case json_type_null: {
			tlv = jp_tlv_data_new(NUL, 0, NULL);
			break;
		}
		case json_type_boolean: {
			int b = json_object_get_boolean(val);
			tlv = jp_tlv_data_new(
				b ? BOOL_TRUE : BOOL_FALSE, 0, NULL);
			break;
		}
		case json_type_double: {
			double d = json_object_get_double(val);
			tlv = jp_tlv_data_new(FLOATING, sizeof(d), &d);
			break;
		}
		case json_type_int: {
			int64_t i = json_object_get_int64(val);
			tlv = jp_tlv_data_new(INTEGER, sizeof(i), &i);
			break;
		}
		case json_type_string: {
			const char *s = json_object_get_string(val);
			tlv = jp_tlv_data_new(STRING,
					      json_object_get_string_len(val),
					      s);
			break;
		}
		default:
			jp_panic("Unknown type: %d", type);
			break;
		}

		snprintf(buf, sizeof(buf), "%" PRIu64, key_idx++);
		json_object_object_add(key_map, buf,
				       json_object_new_string(key));

		jp_tlv_data_write(tlv, output);
		jp_tlv_data_free(tlv);
	}
	json_object_object_foreach(key_map, i, k) {
		struct jp_tlv_data *tlv = jp_tlv_data_new(
			KEY,
			json_object_get_string_len(k),
			json_object_get_string(k));
		jp_tlv_data_write(tlv, output);
		jp_tlv_data_free(tlv);

	}
	json_object_put(key_map);
}

/* @TODO This should be reworked: instead of reading the whole file into
 * memory, we should find where keys start and process key and value in the
 * same time, writing it to output one object at a time. */
/* @TODO This should be in a separate file. */
/* @TODO JSON objects should be parsed in separate function inside
 * src/jp_tlv_data.c. */
static void
decode(FILE *input, FILE *output)
{
	struct json_object *json_obj = json_object_new_object();
	struct json_object *key_map = json_object_new_object();
	/* @TODO check that 32 is enough for any uint64 */
	char buf[32];
	uint64_t idx = 1;
	uint64_t key_idx = 1;
	char c;
	struct jp_tlv_data tlv;

	while (fread(&c, sizeof(char), 1, input) == 1) {
		memset(&tlv, 0, sizeof(tlv));

		tlv.type = (enum jp_tlv_data_type)c;
		if (tlv.type == KEY) {
			snprintf(buf, sizeof(buf), "%" PRIu64, key_idx++);
		} else {
			snprintf(buf, sizeof(buf), "%" PRIu64, idx++);
		}

		switch (tlv.type) {
		case NUL:
			json_object_object_add(json_obj, buf,
					       json_object_new_null());
			break;
		case BOOL_FALSE:
			json_object_object_add(json_obj, buf,
					       json_object_new_boolean(0));
			break;
		case BOOL_TRUE:
			json_object_object_add(json_obj, buf,
					       json_object_new_boolean(1));
			break;
		case KEY:
		case STRING:
			if (fread(&tlv.length,
				  sizeof(tlv.length), 1, input) != 1) {
				jp_panic("Failed to read length");
			}

			tlv.value.string = calloc(tlv.length, sizeof(char));
			if (tlv.value.string == NULL) {
				jp_panic("calloc failed");
			}
			if (fread(tlv.value.string,
				  tlv.length, 1, input) != 1) {
				jp_panic("Failed to read string");
			}

			if (tlv.type == KEY) {
				json_object_object_add(
					key_map,
					buf,
					json_object_new_string(
						tlv.value.string));
			} else {
				json_object_object_add(
					json_obj,
					buf,
					json_object_new_string(
						tlv.value.string));
			}
			free(tlv.value.string);
			break;
		case INTEGER:
			if (fread(&tlv.value.integer,
				  sizeof(tlv.value.integer), 1, input) != 1) {
				jp_panic("Failed to read integer");
			}
			json_object_object_add(json_obj, buf,
					       json_object_new_int64(
						       tlv.value.integer));
			break;
		case FLOATING:
			if (fread(&tlv.value.floating,
				  sizeof(tlv.value.floating), 1, input) != 1) {
				jp_panic("Failed to read floating");
			}
			json_object_object_add(json_obj, buf,
					       json_object_new_double(
						       tlv.value.floating));
			break;
		default:
			jp_panic("Unknown type: %d", tlv.type);
		}
	}

	fprintf(output, "{");
	int comma = 0;
	json_object_object_foreach(json_obj, key, val) {
		struct json_object *k = json_object_object_get(key_map, key);
		if (k == NULL) {
			jp_panic("Failed to find key for %s", key);
		}

		fprintf(output, "%s\n%s, %s",
			comma ? "," : "",
			json_object_to_json_string(k),
			json_object_to_json_string(val));
		comma = 1;
	}
	fprintf(output, "\n}\n");

	json_object_put(json_obj);
	json_object_put(key_map);
}

int main(int argc, char *argv[])
{
	char *input_path = NULL;
	char *output_path = NULL;
	enum jp_mode mode = DEFAULT;

	static const struct option longopts[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "input", required_argument, NULL, 'i' },
		{ "output", required_argument, NULL, 'o' },
		{ "decode", no_argument, NULL, 'd' },
		{ "encode", no_argument, NULL, 'e' },
		{ NULL, 0, NULL, 0 }
	};

	int opt = 0;
	while ((opt = getopt_long(
			argc, argv, "hdei:o:", longopts, NULL)) != -1) {
		switch (opt) {
		case 'h':
			print_help(0, argv[0]);
			break;
		case 'i': {
			if (input_path != NULL || optarg == NULL) {
				print_help(-1, argv[0]);
			}

			int input_path_len = strlen(optarg);
			if (input_path_len == 0) {
				jp_panic("Invalid input path");
			}

			input_path = calloc(input_path_len + 1, sizeof(char));
			if (input_path == NULL) {
				jp_panic("calloc failed");
			}

			strncpy(input_path, optarg, input_path_len);
			break;
		}
		case 'o': {
			if (output_path != NULL || optarg == NULL) {
				print_help(-1, argv[0]);
			}

			int output_path_len = strlen(optarg);
			if (output_path_len == 0) {
				jp_panic("Invalid output path");
			}

			output_path = calloc(output_path_len + 1, sizeof(char));
			if (output_path == NULL) {
				jp_panic("calloc failed");
			}

			strncpy(output_path, optarg, output_path_len);
			break;
		}
		case 'd':
			if (mode != DEFAULT) {
				print_help(-1, argv[0]);
			}
			mode = DECODE;
			break;
		case 'e':
			if (mode != DEFAULT) {
				print_help(-1, argv[0]);
			}
			mode = ENCODE;
			break;
		default:
			jp_panic("Unknown option: %c", opt);
			print_help(-1, argv[0]);
		}
	}
	mode = mode == DEFAULT ? ENCODE : mode;

	switch (mode) {
	case ENCODE: {
		FILE *output = stdout;
		json_object *input = NULL;

		if (output_path != NULL) {
			output = fopen(output_path, "wb");
			if (output == NULL) {
				jp_panic("Failed to open output file: %s",
					 output_path);
			}
		}

		if (input_path != NULL) {
			input = json_object_from_file(input_path);
			if (input == NULL) {
				jp_panic("Failed to open input file: %s",
					 input_path);
			}
		} else {
			input = json_object_from_fd(STDIN_FILENO);
			if (input == NULL) {
				jp_panic("Failed to read from stdin");
			}
		}


		encode(input, output);
		json_object_put(input);
		if (output != stdout && output != NULL) {
			fclose(output);
		}
		break;
	}
	case DECODE: {
		FILE *input = stdin;
		FILE *output = stdout;

		if (input_path != NULL) {
			input = fopen(input_path, "rb");
			if (input == NULL) {
				jp_panic("Failed to open input file: %s",
					 input_path);
			}
		}

		if (output_path != NULL) {
			output = fopen(output_path, "w");
			if (output == NULL) {
				jp_panic("Failed to open output file: %s",
					 output_path);
			}
		}

		decode(input, output);

		if (input != stdin && input != NULL) {
			fclose(input);
		}
		if (output != stdout && output != NULL) {
			fclose(output);
		}
		break;
	}
	default:
		break;
	}

	if (input_path != NULL) {
		free(input_path);
	}
	if (output_path != NULL) {
		free(output_path);
	}
	return 0;
}
