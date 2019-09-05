mkdir build/
cd build/
 cmake -DCMAKE_BUILD_TYPE=Debug -G "CodeBlocks - Unix Makefiles" ../.
cmake --build . --target scala_lex
cd ..
./build/scala_lex ../test-files/test1-lex.scala
echo ""