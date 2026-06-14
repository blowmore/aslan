APP=aslan
EXE=$(APP).exe
OUTDIR=build

SRCS=main.cpp

CXX=cl
CXXFLAGS=/nologo /std:c++17 /EHsc /O2 /DNDEBUG
LDFLAGS=/nologo /SUBSYSTEM:CONSOLE Advapi32.lib

all: $(OUTDIR)\$(EXE)

$(OUTDIR):
	@if not exist $(OUTDIR) mkdir $(OUTDIR)

$(OUTDIR)\$(EXE): $(SRCS) $(OUTDIR)
	$(CXX) $(CXXFLAGS) /Fe$@ $(SRCS) /link $(LDFLAGS)

clean:
	@if exist $(OUTDIR) rmdir /s /q $(OUTDIR)
