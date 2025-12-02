# CMake generated Testfile for 
# Source directory: C:/Logic interactive Dropbox/tommy _/dev/haxe/hlffi/tests
# Build directory: C:/Logic interactive Dropbox/tommy _/dev/haxe/hlffi/build-hlc/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(test_hlc "C:/Logic interactive Dropbox/tommy _/dev/haxe/hlffi/build-hlc/bin/Debug/test_hlc.exe")
  set_tests_properties(test_hlc PROPERTIES  _BACKTRACE_TRIPLES "C:/Logic interactive Dropbox/tommy _/dev/haxe/hlffi/tests/CMakeLists.txt;44;add_test;C:/Logic interactive Dropbox/tommy _/dev/haxe/hlffi/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(test_hlc "C:/Logic interactive Dropbox/tommy _/dev/haxe/hlffi/build-hlc/bin/Release/test_hlc.exe")
  set_tests_properties(test_hlc PROPERTIES  _BACKTRACE_TRIPLES "C:/Logic interactive Dropbox/tommy _/dev/haxe/hlffi/tests/CMakeLists.txt;44;add_test;C:/Logic interactive Dropbox/tommy _/dev/haxe/hlffi/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(test_hlc "C:/Logic interactive Dropbox/tommy _/dev/haxe/hlffi/build-hlc/bin/MinSizeRel/test_hlc.exe")
  set_tests_properties(test_hlc PROPERTIES  _BACKTRACE_TRIPLES "C:/Logic interactive Dropbox/tommy _/dev/haxe/hlffi/tests/CMakeLists.txt;44;add_test;C:/Logic interactive Dropbox/tommy _/dev/haxe/hlffi/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(test_hlc "C:/Logic interactive Dropbox/tommy _/dev/haxe/hlffi/build-hlc/bin/RelWithDebInfo/test_hlc.exe")
  set_tests_properties(test_hlc PROPERTIES  _BACKTRACE_TRIPLES "C:/Logic interactive Dropbox/tommy _/dev/haxe/hlffi/tests/CMakeLists.txt;44;add_test;C:/Logic interactive Dropbox/tommy _/dev/haxe/hlffi/tests/CMakeLists.txt;0;")
else()
  add_test(test_hlc NOT_AVAILABLE)
endif()
