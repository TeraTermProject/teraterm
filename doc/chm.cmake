# - hhc.exe の exit code を無視する
#   -https://cmake.org/pipermail/cmake/2007-October/017111.html

set(HHP "teraterm.hhp")

message("${HHC} ${HHP}")

execute_process(
  COMMAND ${HHC} ${HHP}
)
