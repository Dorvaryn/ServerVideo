CC=gcc
EDL=gcc
RM=rm
BUILDDIR=build/
SRCDIR=
CCFLAGS=
LDFLAGS=
RMFLAGS=-f
EXE=ServerVideo
LIBS=
EFFACE=clean
OBJ=$(BUILDDIR)requete.o $(BUILDDIR)utils.o $(BUILDDIR)cata.o

$(EXE) : $(OBJ) $(BUILDDIR)main.o
	$(EDL) $(LDFLAGS) -o $(EXE) $(OBJ) $(LIBS) $(BUILDDIR)main.o

$(BUILDDIR)%.o : $(SRCDIR)%.c $(SRCDIR)%.h
	$(CC) $(CCFLAGS) -c $< -o $@

$(BUILDDIR)main.o : $(SRCDIR)main.c
	$(CC) $(CCFLAGS) -c $< -o $@

$(EFFACE) :
	$(RM) $(RMFLAGS) $(BUILDDIR)*.o $(EXE) core
