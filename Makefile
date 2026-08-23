APP=aslan
EXE=$(APP).exe
OUTDIR=build

SRCS=src\main.cpp src\registry_key.cpp src\installation.cpp src\parse.cpp src\launch.cpp src\dialog.cpp
RES=$(OUTDIR)\dialog.res

CXX=cl
RC=rc
CXXFLAGS=/nologo /std:c++17 /EHsc /O2 /DNDEBUG /Fo$(OUTDIR)\ /Fd$(OUTDIR)\vc.pdb
LDFLAGS=/nologo /SUBSYSTEM:CONSOLE Advapi32.lib User32.lib

all: $(OUTDIR)\$(EXE)

$(OUTDIR):
	@if not exist $(OUTDIR) mkdir $(OUTDIR)

$(RES): src\dialog.rc src\resource.h $(OUTDIR)
	$(RC) /nologo /fo$@ src\dialog.rc

$(OUTDIR)\$(EXE): $(SRCS) $(RES)
	$(CXX) $(CXXFLAGS) /Fe$@ $(SRCS) $(RES) /link $(LDFLAGS)

clean:
	@if exist $(OUTDIR) rmdir /s /q $(OUTDIR)
