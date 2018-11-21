#!/bin/bash

if [[ ! -e DUNEPrismTools ]]; then
  git clone https://github.com/luketpickering/DUNEPrismTools.git DUNEPrismTools
fi

if [[ ! -e "Prob3++.20121225" ]]; then
wget http://webhome.phy.duke.edu/~raw22/public/Prob3++/Prob3++.20121225.tar.gz
tar -zxvf Prob3++.20121225.tar.gz
cd Prob3++.20121225
make
cd ../
fi

if [[ ! -e eigen ]]; then
git clone https://github.com/eigenteam/eigen-git-mirror.git eigen
cd eigen
git checkout 3.3.5
cd ../
fi

g++ -O3 -std=c++11 DUNEPrismTools/app/flux_tools/FluxLinearSolver_Standalone.cxx -o FluxLinearSolver_Standalone.exe -I DUNEPrismTools/src/flux_tools/ -I eigen -I Prob3++.20121225 Prob3++.20121225/libThreeProb_2.10.a $(root-config --cflags --libs)
