#include <jp_tlv_data.h>
#include <jp_util.h>

#include <stdlib.h>
#include <string.h>

struct jp_tlv_data *
jp_tlv_data_new(
	const enum jp_tlv_data_type type,
	const uint8_t length,
	const void *value)
{
	struct jp_tlv_data *data = malloc(sizeof(struct jp_tlv_data));

	data->type = type;

	switch(data->type) {
	case NUL:
		data->length = 0;
		break;
	case KEY:
	case STRING:
		data->length = length;
		data->value.string = calloc(length, sizeof(char));
		memcpy(data->value.string, value, length);
		break;
	case INTEGER:
		data->length = sizeof(int64_t);
		data->value.integer = *(int64_t *)value;
		break;
	case FLOATING:
		data->length = sizeof(double);
		data->value.floating = *(double *)value;
		break;
	case BOOL_FALSE:
		data->length = 0;
		data->value.boolean = 0;
		break;
	case BOOL_TRUE:
		data->length = 0;
		data->value.boolean = 1;
		break;
	default:
		jp_panic("Unknown JSON object type: %d", type);
	}

	return data;
}

void
jp_tlv_data_free(struct jp_tlv_data *data)
{
	if (data == NULL) {
		return;
	}

	if (data->type == STRING || data->type == KEY) {
		if (data->value.string != NULL) {
			free(data->value.string);
		}
	}

	data->value.string = NULL;
	data->length = 0;
	data->type = 0;

	free(data);
}

int
jp_tlv_data_write(const struct jp_tlv_data *data, FILE *stream)
{
	char *msg = NULL;
	char type = (char)data->type;

	if (fwrite(&type, sizeof(type), 1, stream) != 1) {
		msg = "Failed to write type";
		goto panic;
	}

	/* @TODO: add fixed-endian fwrite wrapper */
	switch (type) {
		case NUL:
		case BOOL_FALSE:
		case BOOL_TRUE:
			break;
		case KEY:
		case STRING:
			if (fwrite(&data->length,
				   sizeof(data->length), 1, stream) != 1) {
				msg = "Failed to write length";
				goto panic;
			}

			if (fwrite(data->value.string,
				   data->length, 1, stream) != 1) {
				msg = "Failed to write string";
				goto panic;
			}

			break;
		case INTEGER:
			if (fwrite(&data->value.integer,
				   sizeof(data->value.integer),
				   1, stream) != 1) {
				msg = "Failed to write integer";
				goto panic;
			}
			break;
		case FLOATING:
			if (fwrite(&data->value.floating,
				   sizeof(data->value.floating),
				   1, stream) != 1) {
				msg = "Failed to write floating";
				goto panic;
			}
			break;
		default:
			jp_panic("Unknown JSON object type: %d", data->type);
	}

	return 0;

panic:
	jp_panic("%s", msg);
}
