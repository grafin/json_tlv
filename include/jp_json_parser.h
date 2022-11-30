#ifndef JP_JSON_PARSER_H
#define JP_JSON_PARSER_H

#include <jp_tlv_data.h>
#include <json.h>

struct jp_tlv_data *
jp_json_next_object(FILE *stream);

#endif /* JP_JSON_PARSER_H */
