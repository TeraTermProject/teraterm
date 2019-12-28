
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *	•¶Žš—ñ‚Í‚·‚×‚ÄUTF-8
 */
typedef struct {
	int version_major;
	int version_minor;
	const char *version_text;
	const char *text;
	const char *url;
} version_one_t;

version_one_t *ParseJson(const char *json, size_t *info_count);
void ParseFree(version_one_t *ptr, size_t count);

#ifdef __cplusplus
}
#endif
