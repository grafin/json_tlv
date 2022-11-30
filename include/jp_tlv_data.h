#ifndef JP_TLV_DATA_H
#define JP_TLV_DATA_H

#include <stdint.h>
#include <stdio.h>

enum jp_tlv_data_type {
	NUL = 0,
	STRING,
	INTEGER,
	FLOATING,
	BOOL_TRUE,
	BOOL_FALSE,
	KEY,
	JP_TLV_DATA_TYPE_MAX
};

union jp_tlv_data_value {
	char *string;
	int64_t integer;
	double floating;
	uint8_t boolean;
};

struct jp_tlv_data {
	enum jp_tlv_data_type type;
	uint8_t length;
	union jp_tlv_data_value value;
};

struct jp_tlv_data *
jp_tlv_data_new(
	const enum jp_tlv_data_type type,
	const uint8_t length,
	const void *value);
void
jp_tlv_data_free(struct jp_tlv_data *data);
int
jp_tlv_data_write(const struct jp_tlv_data *data, FILE *stream);

#endif /* JP_TLV_DATA_H */
