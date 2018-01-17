# - Try to find python jsonschema package
# Once done, this will define
#
#  jsonschema_FOUND - system has python jsonschema & repoze packages
#  jsonschema_INCLUDE_DIRS - the jsonschema include directories


#find python site-package installation path
execute_process ( COMMAND python -c "from distutils.sysconfig import get_python_lib; print get_python_lib()" OUTPUT_VARIABLE PYTHON_SITE_PACKAGES OUTPUT_STRIP_TRAILING_WHITESPACE)

find_path(jsonschema_INCLUDE_DIRS
  NAMES jsonschema 
  PATHS ${PYTHON_SITE_PACKAGES}
  $ENV{PYTHONPATH}
  DOC "packager runtime dependencies: jsonschema")

if(jsonschema_INCLUDE_DIRS)
  set(jsonschema_FOUND true)
else(jsonschema_INCLUDE_DIRS)
  message(WARNING "PACKAGER runtime depenency python jsonschema package is missing!!!")
endif(jsonschema_INCLUDE_DIRS)
