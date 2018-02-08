SYSTEM     = x86-64_linux
LIBFORMAT  = static_pic

#------------------------------------------------------------
#
# When you adapt this makefile to compile your CPLEX programs
# please copy this makefile and set CPLEXDIR and CONCERTDIR to
# the directories where CPLEX and CONCERT are installed.
#
#------------------------------------------------------------

#CPLEXDIR      = ../../..
CPLEXDIR      = /home/zyecheng/Artifact/CPLEX/CPLEXDir/cplex
#CONCERTDIR    = ../../../../concert
CONCERTDIR    = /home/zyecheng/Artifact/CPLEX/CPLEXDir/concert

# ---------------------------------------------------------------------
# Compiler selection 
# ---------------------------------------------------------------------

CCC = g++ -O3
CC  = gcc -O0
JAVAC = javac 

# ---------------------------------------------------------------------
# Compiler options 
# ---------------------------------------------------------------------

CCOPT = -m64 -O -fPIC -fno-strict-aliasing -fexceptions -DNDEBUG -DIL_STD
COPT  = -m64 -fPIC -fno-strict-aliasing
JOPT  = -classpath $(CPLEXDIR)/lib/cplex.jar -O

# ---------------------------------------------------------------------
# Link options and libraries
# ---------------------------------------------------------------------

CPLEXBINDIR   = $(CPLEXDIR)/bin/$(BINDIST)
CPLEXJARDIR   = $(CPLEXDIR)/lib/cplex.jar
CPLEXLIBDIR   = $(CPLEXDIR)/lib/$(SYSTEM)/$(LIBFORMAT)
CONCERTLIBDIR = $(CONCERTDIR)/lib/$(SYSTEM)/$(LIBFORMAT)

CCLNDIRS  = -L$(CPLEXLIBDIR) -L$(CONCERTLIBDIR)
CLNDIRS   = -L$(CPLEXLIBDIR)
CCLNFLAGS = -lconcert -lilocplex -lcplex -lm -lpthread
CLNFLAGS  = -lcplex -lm -lpthread
JAVA      = java  -d64 -Djava.library.path=$(CPLEXDIR)/bin/x86-64_linux -classpath $(CPLEXJARDIR):
CFLAGS	  = -I./ -w -std=c++11 -I$(CPLEXDIR)/include -I$(CONCERTDIR)/include -DIL_STD -fpermissive -D MCMILPPACT -D LINUX -D NOFSM

obj = MainFunctionLinux.o ResponseTimeCalculator.o StatisticSet.o TaskSet.o Util.o Timer.o Task.o main.o EventDrivenPACaseStudy.o NoCFlowFocusRefinement.o NOCCommunication.o

#all:	
#	$(CCC) $(CFLAGS) $(CCLNDIRS) *.cpp  -o MCMILPPact.exe $(CCLNFLAGS)

MainFunctionLinux.o: MainFunctionLinux.cpp MainFunction.h
	 $(CCC) $(CFLAGS) $(CCLNDIRS) -c  MainFunctionLinux.cpp 

ResponseTimeCalculator.o: ResponseTimeCalculator.cpp ResponseTimeCalculator.h
	$(CCC) $(CFLAGS) $(CCLNDIRS) -c   ResponseTimeCalculator.cpp

StatisticSet.o: StatisticSet.cpp StatisticSet.h
	$(CCC) $(CFLAGS) $(CCLNDIRS) -c  StatisticSet.cpp

Task.o: Task.cpp Task.h
	$(CCC) $(CFLAGS) $(CCLNDIRS) -c  Task.cpp

TaskSet.o: TaskSet.cpp TaskSet.h  Task.h
	$(CCC) $(CFLAGS) $(CCLNDIRS) -c  TaskSet.cpp

Util.o: Util.cpp Util.h
	$(CCC) $(CFLAGS) $(CCLNDIRS) -c Util.cpp

Timer.o: Timer.cpp Timer.h
	$(CCC) $(CFLAGS) $(CCLNDIRS) -c Timer.cpp

EventDrivenPACaseStudy.o: EventDrivenPACaseStudy.cpp EventDrivenPACaseStudy.h
	$(CCC) $(CFLAGS) $(CCLNDIRS) -c EventDrivenPACaseStudy.cpp

NoCFlowFocusRefinement.o: NoCFlowFocusRefinement.cpp NoCFlowFocusRefinement.h
	$(CCC) $(CFLAGS) $(CCLNDIRS) -c NoCFlowFocusRefinement.cpp

NOCCommunication.o: NOCCommunication.cpp NoCCommunication.h
	$(CCC) $(CFLAGS) $(CCLNDIRS) -c NOCCommunication.cpp

main.o: main.cpp
	$(CCC) $(CFLAGS) $(CCLNDIRS) -c main.cpp


all: $(obj)
	$(CCC) $(CFLAGS) $(CCLNDIRS) -o MUTER.exe $(obj) $(CCLNFLAGS)


clean :
	rm $(obj) MUTER.exe
	

# ------------------------------------------------------------
#
# The examples
#
indefqpex1: indefqpex1.o
	$(CC) $(CFLAGS) $(CLNDIRS) -o indefqpex1 indefqpex1.o $(CLNFLAGS)
indefqpex1.o: $(EXSRCC)/indefqpex1.c
	$(CC) -c $(CFLAGS) $(EXSRCC)/indefqpex1.c -o indefqpex1.o
