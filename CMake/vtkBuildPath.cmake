# Attempt to build up the path/ld_library_path/python path needed to run VTK.
# On Windows simply executing the .bat file should be enough, on Linux/Mac the
# file can be sourced in the shell. You can also copy and paste the relevant
# parts into other files if preferred.
#
# Note: Now only setting the path to the latest configuration used (for MSVC/Xcode)

if(WIN32)
  set(VTK_PATH_SHELL_SCRIPT "windows_path.bat")
  set(PATH_FORMAT "set \${path_var}=\${add_path};%\${path_var}%\r\n")
  set(PATH_VARIABLE "PATH")
  set(PATH_SEPARATOR ";")
elseif(UNIX)
  set(VTK_PATH_SHELL_SCRIPT "unix_path.sh")
  if(APPLE)
    set(DYLD "DYLD")
  else()
    set(DYLD "LD")
  endif()
  set(PATH_VARIABLE "${DYLD}_LIBRARY_PATH")
  set(PATH_SEPARATOR ":")
  set(PATH_FORMAT "export \${path_var}=\${add_path}:\${\${path_var}}\n")
endif()

# set the script file name
string(CONCAT PATH_FILENAME "${VTK_CURRENT_BINARY_DIR}/" VTK_PATH_SHELL_SCRIPT)

# FOR THE PATH-VARIABLE
# replace the path to the executables
string(REPLACE "\${add_path}" "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}" PATH_TEMP PATH_FORMAT)
# replace the name of the platform-specific path environment variable
string(REPLACE "\${path_var}" PATH_VARIABLE PATH_LINES PATH_TEMP)

if(VTK_WRAP_PYTHON)
# FOR THE PYTHONPATH VARIABLE, if PYTHON is wrapped
# replace the path to the python-specific files
  string(CONCAT PATH_TEMP "${VTK_CURRENT_BINARY_DIR}/Wrapping/Python" PATH_SEPARATOR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}")
  string(REPLACE "\${add_path}" PATH_TEMP PATH_TEMP PATH_FORMAT)
# replace pathvar by PYTHONPATH
  string(REPLACE "\${path_var}" "PYTHONPATH" PATH_TEMP PATH_TEMP)
# apped the line to the file
  string(CONCAT PATH_LINES PATH_TEMP)
endif()

# write to file
file(WRITE PATH_FILENAME PATH_LINES)
