#ifndef STB_XML_H
#define STB_XML_H

#include <stdint.h>

typedef struct 
{
	uint16_t start;
	uint16_t end;
	uint8_t type;
	uint8_t val_start;
} xml_token_t;

#define XML_TAG_OPEN  1
#define XML_TAG_CLOSE 2
#define XML_TEXT      3
#define XML_COMMENT   4
#define XML_CDATA     5
#define XML_ATTR      6

#ifdef __cplusplus
extern "C" {
#endif

static int32_t xml_parse(const char* xml, int32_t len, xml_token_t* tokens, int32_t max_tokens);

#ifdef __cplusplus
}
#endif

#endif

#ifdef STB_XML_IMPLEMENTATION

#define IS_WHITESPACE(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')
#define IS_NAME_CHAR(c) (((c) >= 'A' && (c) <= 'Z') || \
                         ((c) >= 'a' && (c) <= 'z') || \
                         ((c) >= '0' && (c) <= '9') || \
                         (c) == ':' || (c) == '_' || (c) == '-' || (c) == '.')

static int32_t xml_parse(const char* xml, int32_t len, xml_token_t* tokens, int32_t max_tokens)
{
	int32_t token_count = 0;
	int32_t pos = 0;
	int32_t state = 0;
	int32_t quote = 0;
	int32_t text_start = -1;
	int32_t last_tag_idx = -1;
	
	while (pos < len && token_count < max_tokens)
	{
		char c = xml[pos];
		
		if (state == 2) /* Comment */
		{
			if (c == '-' && pos + 2 < len && xml[pos + 1] == '-' && xml[pos + 2] == '>')
			{
				if (token_count > 0)
				{
					tokens[token_count - 1].end = pos;
				}
				pos += 3;
				state = 0;
				continue;
			}
			pos++;
			continue;
		}
		
		if (state == 3) /* CDATA */
		{
			if (c == ']' && pos + 2 < len && xml[pos + 1] == ']' && xml[pos + 2] == '>')
			{
				if (token_count > 0)
				{
					tokens[token_count - 1].end = pos;
				}
				pos += 3;
				state = 0;
				continue;
			}
			pos++;
			continue;
		}
		
		if (quote) /* Quoted string */
		{
			if (c == quote)
			{
				quote = 0;
			}
			pos++;
			continue;
		}
		
		if (c == '<')
		{
			if (text_start != -1 && text_start < pos)
			{
				int32_t i = text_start;
				while (i < pos && IS_WHITESPACE(xml[i]))
				{
					i++;
				}
				if (i < pos)
				{
					tokens[token_count].type = XML_TEXT;
					tokens[token_count].start = i;
					tokens[token_count].end = pos;
					tokens[token_count].val_start = 0;
					token_count++;
				}
				text_start = -1;
			}
			
			pos++;
			if (pos >= len) break;
			
			if (xml[pos] == '?') /* XML declaration */
			{
				pos++;
				while (pos + 1 < len && !(xml[pos] == '?' && xml[pos + 1] == '>'))
				{
					pos++;
				}
				if (pos + 1 < len) pos += 2;
				continue;
			}
			
			if (xml[pos] == '!')
			{
				pos++;
				if (pos + 2 < len && xml[pos] == '-' && xml[pos + 1] == '-')
				{
					pos += 2;
					if (token_count < max_tokens)
					{
						tokens[token_count].type = XML_COMMENT;
						tokens[token_count].start = pos;
						tokens[token_count].end = 0;
						tokens[token_count].val_start = 0;
						token_count++;
					}
					state = 2;
					continue;
				}
				else if (pos + 6 < len &&
				         xml[pos] == '[' && xml[pos + 1] == 'C' &&
				         xml[pos + 2] == 'D' && xml[pos + 3] == 'A' &&
				         xml[pos + 4] == 'T' && xml[pos + 5] == 'A' &&
				         xml[pos + 6] == '[')
				{
					pos += 7;
					if (token_count < max_tokens)
					{
						tokens[token_count].type = XML_CDATA;
						tokens[token_count].start = pos;
						tokens[token_count].end = 0;
						tokens[token_count].val_start = 0;
						token_count++;
					}
					state = 3;
					continue;
				}
			}
			
			if (xml[pos] == '/') /* Closing tag */
			{
				pos++;
				if (token_count < max_tokens)
				{
					tokens[token_count].type = XML_TAG_CLOSE;
					tokens[token_count].start = pos;
					tokens[token_count].val_start = 0;
					
					while (pos < len && IS_NAME_CHAR(xml[pos]))
					{
						pos++;
					}
					tokens[token_count].end = pos;
					token_count++;
				}
				
				while (pos < len && xml[pos] != '>')
				{
					pos++;
				}
				if (pos < len) pos++;
				state = 0;
				continue;
			}
			
			/* Opening tag */
			if (token_count < max_tokens)
			{
				tokens[token_count].type = XML_TAG_OPEN;
				tokens[token_count].start = pos;
				tokens[token_count].end = 0;
				tokens[token_count].val_start = 0;
				
				while (pos < len && IS_NAME_CHAR(xml[pos]))
				{
					pos++;
				}
				tokens[token_count].end = pos;
				
				last_tag_idx = token_count;
				token_count++;
			}
			state = 1;
			continue;
		}
		
		if (state == 1) /* Inside tag - parsing attributes */
		{
			if (c == '>')
			{
				state = 0;
				pos++;
				continue;
			}
			
			if (c == '/' && pos + 1 < len && xml[pos + 1] == '>')
			{
				if (last_tag_idx != -1)
				{
					tokens[last_tag_idx].val_start = 1;
				}
				pos += 2;
				state = 0;
				continue;
			}
			
			if (IS_WHITESPACE(c))
			{
				pos++;
				continue;
			}
			
			/* Attribute */
			if (token_count < max_tokens)
			{
				tokens[token_count].type = XML_ATTR;
				tokens[token_count].start = pos;
				tokens[token_count].end = 0;
				tokens[token_count].val_start = 0;
				
				while (pos < len && IS_NAME_CHAR(xml[pos]))
				{
					pos++;
				}
				tokens[token_count].end = pos;
				
				while (pos < len && xml[pos] != '=')
				{
					pos++;
				}
				if (pos < len) pos++;
				
				while (pos < len && IS_WHITESPACE(xml[pos]))
				{
					pos++;
				}
				if (pos < len && (xml[pos] == '"' || xml[pos] == '\''))
				{
					quote = xml[pos];
					pos++;
					tokens[token_count].val_start = pos;
					
					while (pos < len && xml[pos] != quote)
					{
						pos++;
					}
					if (pos < len)
					{
						pos++;
						quote = 0;
					}
				}
				
				token_count++;
				continue;
			}
		}
		
		if (state == 0 && text_start == -1 && !IS_WHITESPACE(c))
		{
			text_start = pos;
		}
		
		pos++;
	}
	
	if (text_start != -1 && text_start < pos)
	{
		int32_t i = text_start;
		while (i < pos && IS_WHITESPACE(xml[i]))
		{
			i++;
		}
		if (i < pos)
		{
			tokens[token_count].type = XML_TEXT;
			tokens[token_count].start = i;
			tokens[token_count].end = pos;
			tokens[token_count].val_start = 0;
			token_count++;
		}
	}
	
	return token_count;
}

#endif