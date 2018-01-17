# - Try to find python repoze package
# Once done, this will define
#
#  repoze_FOUND - system has python repoze packages
#  repoze_INCLUDE_DIRS - the repoze include directories


#find python site-package installation path
execute_process ( COMMAND python -c "from distutils.sysconfig import get_python_lib; print get_python_lib()" OUTPUT_VARIABLE PYTHON_SITE_PACKAGES OUTPUT_STRIP_TRAILING_WHITESPACE)

find_path(repoze_INCLUDE_DIRS
  NAMES repoze 
  PATHS ${PYTHON_SITE_PACKAGES}
  $ENV{PYTHONPATH}
  DOC "packager runtime dependencies: repoze")

if(repoze_INCLUDE_DIRS)
  set(repoze_FOUND true)
else(repoze_INCLUDE_DIRS)
  message(WARNING "PACKAGER runtime depenency python repoze package is missing!!!")
endif(repoze_INCLUDE_DIRS)
