APP=aslan
EXE=$(APP).exe
OUTDIR=build

SRCS=src\main.cpp src\registry_key.cpp src\installation.cpp src\project_parser.cpp

CXX=cl
CXXFLAGS=/nologo /std:c++17 /EHsc /O2 /DNDEBUG /Fo$(OUTDIR)\ /Fd$(OUTDIR)\vc.pdb
LDFLAGS=/nologo /SUBSYSTEM:CONSOLE Advapi32.lib

all: $(OUTDIR)\$(EXE)

$(OUTDIR):
	@if not exist $(OUTDIR) mkdir $(OUTDIR)

$(OUTDIR)\$(EXE): $(SRCS) $(OUTDIR)
	$(CXX) $(CXXFLAGS) /Fe$@ $(SRCS) /link $(LDFLAGS)

clean:
	@if exist $(OUTDIR) rmdir /s /q $(OUTDIR)
