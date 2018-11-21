#pragma once

#include <functional>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <map>

using namespace std;
namespace ana
{

  class LinComb
  {
  public:
    LinComb (const std::string  tableIn, const std::string  histogramIn);   	
    ~LinComb(); 
    std::map <float, std::map <float, std::map<float, float>>> GetMap();

  private:
    const std::string  fInputTxt;
    const std::string  fInputDir;
    std::map <float, std::map <float, std::map<float, float>>> fMap;
  };
}
