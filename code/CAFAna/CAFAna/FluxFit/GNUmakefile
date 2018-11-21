CC=g++
src=DUNEPrismTools/app/flux_tools/FluxLinearSolver_Standalone.cxx
obj= myprog.o

LDFLAGS=`root-config --cflags --glibs` 

myprog: $(OBJ)
	$(CC) -O3 -std=c++11 $(src) -o FluxLinearSolver_Standalone -I DUNEPrismTools/src/flux_tools/ -I eigen -I Prob3++.20121225 Prob3++.20121225/libThreeProb_2.10.a $(LDFLAGS)

.PHONY: clean
