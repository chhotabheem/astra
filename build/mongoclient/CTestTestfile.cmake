# CMake generated Testfile for 
# Source directory: /app/astra/mongoclient
# Build directory: /app/astra/build/mongoclient
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(MongoClientTest "/app/astra/build/mongoclient/test_mongoclient")
set_tests_properties(MongoClientTest PROPERTIES  _BACKTRACE_TRIPLES "/app/astra/mongoclient/CMakeLists.txt;27;add_test;/app/astra/mongoclient/CMakeLists.txt;0;")
subdirs("mongodriver")
