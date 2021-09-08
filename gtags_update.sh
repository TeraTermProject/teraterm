/usr/bin/find teraterm TTProxy TTX* ttssh2 -type f -name "*.c" -o -name "*.cpp" -o -name "*.h" | grep -v Release | grep -v .vs | grep -v Debug | grep -v build > gtags.files
gtags -f gtags.files
# see doc_internal/readme.md
