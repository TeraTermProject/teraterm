add_subdirectory(ttxssh)
set_target_properties(ttxssh PROPERTIES FOLDER ttssh2)

add_subdirectory(ttssh)
set_target_properties(ttssh PROPERTIES FOLDER ttssh2)

add_subdirectory(matcher)
set_target_properties(matcher PROPERTIES FOLDER ttssh2)

# config時に cmake で puttyversion.h を生成するようにした
#add_subdirectory(puttyrev)
#set_target_properties(puttyrev PROPERTIES FOLDER ttssh2)

add_subdirectory(putty)
set_target_properties(libputty PROPERTIES FOLDER ttssh2)

