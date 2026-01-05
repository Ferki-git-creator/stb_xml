#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#define STB_XML_IMPLEMENTATION
#include "stb_xml.h"

static void print_token(const char* xml, xml_token_t token, int idx)
{
	const char* types[] = {"", "TAG_OPEN", "TAG_CLOSE", "TEXT", "COMMENT", "CDATA", "ATTR"};
	
	printf("[%d] %s: ", idx, types[token.type]);
	
	if (token.type == XML_TAG_OPEN || token.type == XML_TAG_CLOSE || 
	    token.type == XML_TEXT || token.type == XML_COMMENT || token.type == XML_CDATA)
	{
		if (token.end > token.start)
		{
			printf("\"%.*s\"", token.end - token.start, xml + token.start);
		}
	}
	else if (token.type == XML_ATTR)
	{
		if (token.end > token.start)
		{
			printf("\"%.*s\"", token.end - token.start, xml + token.start);
		}
		if (token.val_start > 0)
		{
			int32_t val_end = token.val_start;
			while (val_end < (int32_t)strlen(xml) && xml[val_end] != '"' && xml[val_end] != '\'')
			{
				val_end++;
			}
			printf(" value=\"%.*s\"", val_end - token.val_start, xml + token.val_start);
		}
	}
	
	if (token.type == XML_TAG_OPEN)
	{
		printf(" (self-closing: %s)", token.val_start ? "yes" : "no");
	}
	
	printf("\n");
}

static void run_parser(const char* name, const char* xml, int32_t len, int32_t iterations)
{
	xml_token_t tokens[10000];
	clock_t start = clock();
	int32_t total_tokens = 0;
	
	for (int32_t i = 0; i < iterations; i++)
	{
		total_tokens += xml_parse(xml, len, tokens, 10000);
	}
	
	clock_t end = clock();
	double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
	double bytes_per_sec = (len * iterations) / elapsed;
	double tokens_per_sec = (total_tokens) / elapsed;
	
	printf("%s:\n", name);
	printf("  Time: %.3f sec (iterations: %d)\n", elapsed, iterations);
	printf("  Speed: %.0f MB/sec\n", bytes_per_sec / (1024*1024));
	printf("  Tokens/sec: %.0f\n", tokens_per_sec);
	printf("  Avg tokens per parse: %d\n\n", total_tokens / iterations);
}

static char* generate_large_xml(int32_t target_size, int32_t* out_len)
{
	const char* item_template = 
		"<item id=\"%d\">"
		"<name>Product %d</name>"
		"<price>%d.%02d</price>"
		"<quantity>%d</quantity>"
		"<category>Category %d</category>"
		"</item>";
	
	int32_t buffer_size = target_size + 10000;
	char* xml = (char*)malloc(buffer_size);
	if (!xml)
	{
		return NULL;
	}
	
	int32_t pos = 0;
	
	pos += snprintf(xml + pos, buffer_size - pos, "<products>\n");
	
	int32_t item_id = 0;
	while (pos < target_size && pos < buffer_size - 1000)
	{
		item_id++;
		pos += snprintf(xml + pos, buffer_size - pos, item_template,
		               item_id,
		               item_id,
		               item_id % 100, (item_id * 37) % 100,
		               item_id % 20,
		               item_id % 10);
		
		if (item_id % 10 == 0)
		{
			pos += snprintf(xml + pos, buffer_size - pos, "\n");
		}
	}
	
	pos += snprintf(xml + pos, buffer_size - pos, "\n</products>");
	
	*out_len = pos;
	return xml;
}

int main()
{
	printf("=== STB XML PARSER PERFORMANCE ===\n\n");
	
	printf("1. Students XML (functional test):\n");
	const char* xml1 = 
		"<students>\n"
		"  <student id=\"101\" group=\"A\">\n"
		"    <name>John Smith</name>\n"
		"    <grades>\n"
		"      <math>12</math>\n"
		"      <physics>11</physics>\n"
		"    </grades>\n"
		"  </student>\n"
		"</students>";
	
	int32_t len = (int32_t)strlen(xml1);
	xml_token_t tokens1[50];
	int32_t count = xml_parse(xml1, len, tokens1, 50);
	
	printf("Tokens: %d\n", count);
	
	for (int32_t i = 0; i < count; i++)
	{
		print_token(xml1, tokens1[i], i);
	}
	printf("\n");
	
	printf("2. Performance test (small XML, 72 bytes):\n");
	const char* small_xml = "<item id=\"1\"><name>Test</name><value>42</value></item>";
	int32_t small_len = (int32_t)strlen(small_xml);
	
	run_parser("Small XML", small_xml, small_len, 100000);
	
	printf("3. Performance test (medium XML, ~1KB):\n");
	const char* medium_xml = 
		"<data>\n"
		"  <entry><id>1</id><name>Alice Johnson</name><score>95</score><grade>A</grade></entry>\n"
		"  <entry><id>2</id><name>Bob Smith</name><score>87</score><grade>B</grade></entry>\n"
		"  <entry><id>3</id><name>Charlie Brown</name><score>92</score><grade>A-</grade></entry>\n"
		"  <entry><id>4</id><name>David Wilson</name><score>78</score><grade>C+</grade></entry>\n"
		"  <entry><id>5</id><name>Eve Davis</name><score>99</score><grade>A+</grade></entry>\n"
		"  <entry><id>6</id><name>Frank Miller</name><score>88</score><grade>B+</grade></entry>\n"
		"  <entry><id>7</id><name>Grace Lee</name><score>91</score><grade>A-</grade></entry>\n"
		"  <entry><id>8</id><name>Henry Taylor</name><score>84</score><grade>B</grade></entry>\n"
		"  <entry><id>9</id><name>Ivy Clark</name><score>79</score><grade>C+</grade></entry>\n"
		"  <entry><id>10</id><name>Jack Anderson</name><score>96</score><grade>A</grade></entry>\n"
		"</data>";
	
	int32_t medium_len = (int32_t)strlen(medium_xml);
	run_parser("Medium XML", medium_xml, medium_len, 20000);
	
	printf("4. Performance test (large XML, ~1MB):\n");
	int32_t large_len = 0;
	char* large_xml = generate_large_xml(1024*1024, &large_len);
	
	if (large_xml)
	{
		printf("Generated XML: %d bytes\n", large_len);
		run_parser("Large XML (1MB)", large_xml, large_len, 100);
		
		printf("5. Performance test (large XML, ~1MB, more iterations):\n");
		run_parser("Large XML (1MB, 500 iterations)", large_xml, large_len, 500);
		
		free(large_xml);
	}
	else
	{
		printf("Failed to allocate memory for large XML\n");
	}
	
	printf("\n=== PARSER FEATURES ===\n");
	printf("1. 6-byte tokens (vs libxml2: 40+ bytes per node)\n");
	printf("2. 4-state machine (vs libxml2: complex state machine)\n");
	printf("3. Bit-packed flags (reused fields)\n");
	printf("4. Zero-copy (no data duplication)\n");
	printf("5. Single pass (no backtracking)\n");
	printf("6. Minimal error checking\n");
	printf("7. ~1500 bytes total (vs libxml2: 500KB+)\n");
	printf("8. No dependencies, no allocations during parsing\n");
	
	printf("\n=== COMPARISON WITH LIBXML2 ===\n");
	printf("To run comparison:\n");
	printf("  gcc -o test_libxml test_libxml.c -lxml2 -O2 -I/usr/include/libxml2\n");
	printf("  ./test_libxml\n\n");
	
	return 0;
}