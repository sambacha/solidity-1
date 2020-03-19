# Try to find the boogie binary itself
if (BOOGIE_BIN)
  find_file(BOOGIE_EXE boogie "${BOOGIE_BIN}" DOC "Boogie executable to use")
else()
  find_file(BOOGIE_EXE boogie HINTS ENV PATH DOC "Boogie executable to use")
endif()

# If we couldn't find the boogie binary, try to find the DLL to run with dotnet
if (NOT BOOGIE_EXE)
  if (BOOGIE_BIN)
    find_file(BOOGIE_DLL BoogieDriver.dll "${BOOGIE_BIN}" DOC "Boogie DLL to use")
  else()
    find_file(BOOGIE_DLL boogie DOC "Boogie DLL to use")
  endif()
  # If found, prepend 'dotnet' and add to BOOGIE_EXE
  if (BOOGIE_DLL)
    set(BOOGIE_EXE "dotnet ${BOOGIE_DLL}" CACHE STRING "Boogie executable to use" FORCE)
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BOOGIE DEFAULT_MSG BOOGIE_EXE)
