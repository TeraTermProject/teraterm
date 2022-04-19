#
# convert to HTML files for web server
#
TOHTML='perl 2sjis.pl'
REF_E=en/html/reference
REF_J=ja/html/reference

$TOHTML -i ../cygwin/cygterm/COPYING   -o $REF_E/CygTerm+-LICENSE.txt  -l unix
$TOHTML -i ../cygwin/cygterm/COPYING   -o $REF_J/CygTerm+-LICENSE.txt  -l unix

$TOHTML -i $REF_J/build_with_cmake.md -o $REF_J/build_with_cmake.html
$TOHTML -i $REF_E/build_with_cmake.md -o $REF_E/build_with_cmake.html
$TOHTML -i $REF_J/build_library_with_cmake.md -o $REF_J/build_library_with_cmake.html
$TOHTML -i $REF_E/build_library_with_cmake.md -o $REF_E/build_library_with_cmake.html
$TOHTML -i $REF_J/menu_id.md          -o $REF_J/menu_id.html
$TOHTML -i $REF_E/menu_id.md          -o $REF_E/menu_id.html

$TOHTML -i ja/html/setup/folder.md     -o ja/html/setup/folder.html
$TOHTML -i en/html/setup/folder.md     -o en/html/setup/folder.html

chmod 664 $REF_E/CygTerm+-LICENSE.txt
chmod 664 $REF_J/CygTerm+-LICENSE.txt

chmod 664 $REF_J/build_with_cmake.html
chmod 664 $REF_E/build_with_cmake.html
chmod 664 $REF_J/build_library_with_cmake.html
chmod 664 $REF_E/build_library_with_cmake.html
chmod 664 $REF_J/menu_id.html
chmod 664 $REF_E/menu_id.html

chmod 664 ja/html/setup/folder.html
chmod 664 en/html/setup/folder.html
