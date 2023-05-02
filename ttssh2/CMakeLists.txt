add_subdirectory(ttxssh)
set_target_properties(ttxssh PROPERTIES FOLDER ttssh2)

add_subdirectory(matcher)
set_target_properties(matcher PROPERTIES FOLDER ttssh2)

add_subdirectory(argon2)
set_target_properties(argon2 PROPERTIES FOLDER ttssh2)

add_subdirectory(libsshagentc)
set_target_properties(libsshagentc PROPERTIES FOLDER ttssh2)
