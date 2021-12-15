
set(SETUP_URL "https://cygwin.com/setup-x86_64.exe")
set(SETUP_HASH_SHA256 "b9219acd1241ffa4d38e19587f1ccc2854f951e451f3858efc9d2e1fe19d375c")

file(DOWNLOAD
  ${SETUP_URL}
  "c:/cygwin64/setup-x86_64.exe"
  EXPECTED_HASH SHA256=${SETUP_HASH_SHA256}
  SHOW_PROGRESS
  )

execute_process(
  COMMAND c:/cygwin64/setup-x86_64.exe --quiet-mode --upgrade-also
  )
