mkdir win32
mkdir win32\include
mkdir win32\include\il
copy include\il\config.h win32\include\il
copy include\il\il.h win32\include\il
copy include\il\ilu.h win32\include\il
copy include\il\ilut.h win32\include\il
copy include\il\devil_cpp_wrapper.hpp win32\include\il
copy include\il\il2.h win32\include\il

mkdir win32\x86-libcmt
copy projects\msvc10\Release-libcmt\resil.dll win32\x86-libcmt
copy projects\msvc10\Release-libcmt\resil.lib win32\x86-libcmt
copy projects\msvc10\Release-libcmt\resil.map win32\x86-libcmt
copy projects\msvc10\Release-libcmt\ilu.dll win32\x86-libcmt
copy projects\msvc10\Release-libcmt\ilu.lib win32\x86-libcmt
copy projects\msvc10\Release-libcmt\ilu.map win32\x86-libcmt
copy projects\msvc10\Release-libcmt\ilut.dll win32\x86-libcmt
copy projects\msvc10\Release-libcmt\ilut.lib win32\x86-libcmt
copy projects\msvc10\Release-libcmt\ilut.map win32\x86-libcmt

mkdir win32\x86-msvcrt
copy projects\msvc10\Release-msvcrt\resil.dll win32\x86-msvcrt
copy projects\msvc10\Release-msvcrt\resil.lib win32\x86-msvcrt
copy projects\msvc10\Release-msvcrt\resil.map win32\x86-msvcrt
copy projects\msvc10\Release-msvcrt\ilu.dll win32\x86-msvcrt
copy projects\msvc10\Release-msvcrt\ilu.lib win32\x86-msvcrt
copy projects\msvc10\Release-msvcrt\ilu.map win32\x86-msvcrt
copy projects\msvc10\Release-msvcrt\ilut.dll win32\x86-msvcrt
copy projects\msvc10\Release-msvcrt\ilut.lib win32\x86-msvcrt
copy projects\msvc10\Release-msvcrt\ilut.map win32\x86-msvcrt

mkdir win32\x64-libcmt
copy projects\msvc10\x64\Release-libcmt\resil.dll win32\x64-libcmt
copy projects\msvc10\x64\Release-libcmt\resil.lib win32\x64-libcmt
copy projects\msvc10\x64\Release-libcmt\resil.map win32\x64-libcmt
copy projects\msvc10\x64\Release-libcmt\ilu.dll win32\x64-libcmt
copy projects\msvc10\x64\Release-libcmt\ilu.lib win32\x64-libcmt
copy projects\msvc10\x64\Release-libcmt\ilu.map win32\x64-libcmt
copy projects\msvc10\x64\Release-libcmt\ilut.dll win32\x64-libcmt
copy projects\msvc10\x64\Release-libcmt\ilut.lib win32\x64-libcmt
copy projects\msvc10\x64\Release-libcmt\ilut.map win32\x64-libcmt

mkdir win32\x64-msvcrt
copy projects\msvc10\x64\Release-msvcrt\resil.dll win32\x64-msvcrt
copy projects\msvc10\x64\Release-msvcrt\resil.lib win32\x64-msvcrt
copy projects\msvc10\x64\Release-msvcrt\resil.map win32\x64-msvcrt
copy projects\msvc10\x64\Release-msvcrt\ilu.dll win32\x64-msvcrt
copy projects\msvc10\x64\Release-msvcrt\ilu.lib win32\x64-msvcrt
copy projects\msvc10\x64\Release-msvcrt\ilu.map win32\x64-msvcrt
copy projects\msvc10\x64\Release-msvcrt\ilut.dll win32\x64-msvcrt
copy projects\msvc10\x64\Release-msvcrt\ilut.lib win32\x64-msvcrt
copy projects\msvc10\x64\Release-msvcrt\ilut.map win32\x64-msvcrt
