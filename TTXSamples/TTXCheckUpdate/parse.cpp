#include <windows.h>
#include <wininet.h>
#include <assert.h>
#include <string.h>
#include <locale.h>
#include <conio.h>
#include <tchar.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#include "cJSON/cJSON.h"

#include "codeconv.h"

#include "parse.h"

static version_one_t *parse_json_one(cJSON *node, const char *lang, version_one_t *result)
{
	cJSON *node_version = cJSON_GetObjectItemCaseSensitive(node, "version");
	cJSON *node_version_major = cJSON_GetObjectItemCaseSensitive(node_version, "major");
	result->version_major = node_version_major->valueint;
	cJSON *node_version_minor = cJSON_GetObjectItemCaseSensitive(node_version, "minor");
	result->version_minor = node_version_minor->valueint;
	cJSON *node_version_text = cJSON_GetObjectItemCaseSensitive(node_version, "text");
	cJSON *node_version_text_lang = cJSON_GetObjectItemCaseSensitive(node_version_text, lang);
	if (cJSON_HasObjectItem(node_version_text, lang)) {
		node_version_text_lang = cJSON_GetObjectItemCaseSensitive(node_version_text, lang);
	}
	else {
		node_version_text_lang = cJSON_GetObjectItemCaseSensitive(node_version_text, "default");
	}
	result->version_text = _strdup(node_version_text_lang->valuestring);
	cJSON *node_text = cJSON_GetObjectItemCaseSensitive(node, "text");
	cJSON *node_text_lang;
	if (cJSON_HasObjectItem(node_text, lang)) {
		node_text_lang = cJSON_GetObjectItemCaseSensitive(node_text, lang);
	}
	else {
		node_text_lang = cJSON_GetObjectItemCaseSensitive(node_text, "default");
	}
	result->text = _strdup(node_text_lang->valuestring);
	cJSON *node_url = cJSON_GetObjectItemCaseSensitive(node, "url");
	if (node_url != NULL) {
		result->url = _strdup(node_url->valuestring);
	}
	else {
		result->url = NULL;
	}
	return result;
}

version_one_t *ParseJson(const char *json, size_t *info_count)
{
	*info_count = 0;

#if 0
	const wchar_t* agent = L"teraterm_updatechecker";
	const wchar_t* update_info_url = L"http://box.zzz.mydns.jp/json/teraterm_version.json";

	char *ptr;
	size_t size;
	if (!HttpRequest(agent, update_info_url, (void **)&ptr, &size)) {
		return NULL;
	}

	char *strU8 = (char *)malloc(size+1);
	memcpy(strU8, ptr, size);
	strU8[size] = '\0';

#if 0
	std::string resultU8(ptr,&ptr[size]);
	std::wstring resultW = ToWcharU8(resultU8.c_str());

	setlocale(LC_ALL, "Japanese");
	wprintf(L"%s\n", resultW.c_str());
#endif
#endif

	cJSON *json_root = cJSON_Parse(json);
	if (json == NULL) {
		// parse error
		return NULL;
	}

	cJSON *versions = cJSON_GetObjectItemCaseSensitive(json_root, "versions");
	if (versions == NULL) {
		// parse error
		return NULL;
	}
	int version_count = cJSON_GetArraySize(versions);
	if (version_count == 0) {
		// ƒtƒ@ƒCƒ‹‚ª‚¨‚©‚µ‚¢?
		return NULL;
	}
	version_one_t *results = (version_one_t *)calloc(sizeof(version_one_t), version_count);
	//const char *lang = "japanese";
	const char *lang = "jp";

	for (int i = 0; i < version_count; i++) {
		version_one_t *one = &results[i];
		cJSON *node_elem = cJSON_GetArrayItem(versions, i);
		parse_json_one(node_elem, lang, one);
#if 0
		printf("version detail '%s'\n", one->version_str);
		wchar_t *strW = ToWcharU8(one->text);
		printf("text '%ls'\n", strW);
#endif
	}

	cJSON_Delete(json_root);

	*info_count = version_count;

	return results;
}

void ParseFree(version_one_t *ptr, size_t count)
{
	for (int i = 0; i < count; i++) {
		version_one_t *v = &ptr[i];
		free((void *)v->version_text);
		free((void *)v->text);
		free((void *)v->url);
	}
	free(ptr);
}
