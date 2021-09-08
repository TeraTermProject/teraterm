# doxygen
(cd doxygen; doxygen Doxyfile)

# global
(cd ..; ./gtags_update.sh; htags -ans --tabs 4 -F; mv HTML/* doc_internal/global)
