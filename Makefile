CFLAGS=/nologo /O2
RFLAGS=/nologo /n

hotcorner.exe: hotcorner.obj version.res
	$(CC) $(CFLAGS) /Fe:$(@F) $**

clean:
	del *.obj *.res hotcorner.exe

install: hotcorner.exe
	copy hotcorner.exe "%USERPROFILE%\Start Menu\Programs\Startup"

uninstall:
	del "%USERPROFILE%\Start Menu\Programs\Startup\hotcorner.exe"

# set -Name INCLUDE -Value "C:\Program Files (x86)\Windows Kits\10\Include\10.0.16299.0\um;C:\Program Files (x86)\Windows Kits\10\Include\10.0.16299.0\shared;C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\include"
