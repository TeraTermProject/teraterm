# doxygen
(cd doxygen; doxygen Doxyfile)

# global
(rm -rf global; mkdir global; cd ..; gtags; htags -ans --tabs 4 -F; mv HTML/* doc_internal/global)
