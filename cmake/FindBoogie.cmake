if (BOOGIE_BIN)
  find_file(BOOGIE_EXE BoogieDriver.dll "${BOOGIE_BIN}")
else()
  find_file(BOOGIE_EXE BoogieDriver.dll)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BOOGIE DEFAULT_MSG BOOGIE_EXE)
