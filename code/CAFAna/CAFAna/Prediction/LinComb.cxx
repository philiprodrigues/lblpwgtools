#include "CAFAna/Prediction/LinComb.h"

#include "TH1D.h"
#include "TFile.h"

#include <string>
#include <iostream>

using namespace std;
namespace ana
{
  //----------------------------------------------------------------------
  LinComb::LinComb(const std::string table, const std::string histogram)
  : fInputTxt(table), fInputDir(histogram)
  {
  }
  //----------------------------------------------------------------------
  LinComb::~LinComb()
  {
  }
  
  // A simple test version:
  // Set A: sin2 th23 = 0.5;  dm232 = 2.5e-3
  // Set B: sin2 th23 = 0.65; dm232 = 2.8e-3
  // Set C: sin2 th23 = 0.65; dm232 = 2.2e-3
  // Set D: sin2 th23 = 0.65; dm232 = 2.5e-3
  // Set E: sin2 th23 = 0.5;  dm232 = 2.8e-3
  // Set F: sin2 th23 = 0.5;  dm232 = 2.2e-3
  std::map <float, std::map <float, std::map<float, float>>> LinComb::GetMap()
  {
    std::cout<<"getting map from histograms in "<<fInputDir.c_str()<<std::endl;
    TFile f1(Form("/%s/nuprism_coef_oscSpectrum_setA.root",fInputDir.c_str()));
    TFile f2(Form("/%s/nuprism_coef_oscSpectrum_setB.root",fInputDir.c_str()));
    TFile f3(Form("/%s/nuprism_coef_oscSpectrum_setC.root",fInputDir.c_str()));
    TFile f4(Form("/%s/nuprism_coef_oscSpectrum_setD.root",fInputDir.c_str()));
    TFile f5(Form("/%s/nuprism_coef_oscSpectrum_setE.root",fInputDir.c_str()));
    TFile f6(Form("/%s/nuprism_coef_oscSpectrum_setF.root",fInputDir.c_str()));

    TH1D* h1 = (TH1D*) f1.Get("Coefficients");
    TH1D* h2 = (TH1D*) f2.Get("Coefficients");
    TH1D* h3 = (TH1D*) f3.Get("Coefficients");
    TH1D* h4 = (TH1D*) f4.Get("Coefficients");
    TH1D* h5 = (TH1D*) f5.Get("Coefficients");
    TH1D* h6 = (TH1D*) f6.Get("Coefficients");

    std::map<float, float> sMap1;
    std::map<float, float> sMap2;
    std::map<float, float> sMap3;
    std::map<float, float> sMap4;
    std::map<float, float> sMap5;
    std::map<float, float> sMap6;

    std::map<float, float> mMap;
    std::map<float, float> lMap;

    for(Int_t i=0;i<h1->GetNbinsX();i++){
      sMap1.insert (std::pair<float, float> (h1->GetBinCenter(i+1), h1->GetBinContent(i+1) ));
      sMap2.insert (std::pair<float, float> (h2->GetBinCenter(i+1), h2->GetBinContent(i+1) ));
      sMap3.insert (std::pair<float, float> (h3->GetBinCenter(i+1), h3->GetBinContent(i+1) ));
      sMap4.insert (std::pair<float, float> (h4->GetBinCenter(i+1), h4->GetBinContent(i+1) ));
      sMap5.insert (std::pair<float, float> (h5->GetBinCenter(i+1), h5->GetBinContent(i+1) ));
      sMap6.insert (std::pair<float, float> (h6->GetBinCenter(i+1), h6->GetBinContent(i+1) ));
    }
    std::map<float, std::map<float, float>> mMap1;
    mMap1.insert ( std::pair<float, std::map<float, float>> ( 2.5e-3, (std::map<float, float>)sMap1) );
    std::map<float, std::map<float, float>> mMap2;
    mMap2.insert ( std::pair<float, std::map<float, float>> ( 2.8e-3, (std::map<float, float>)sMap2) );
    std::map<float, std::map<float, float>> mMap3;
    mMap3.insert ( std::pair<float, std::map<float, float>> ( 2.2e-3, (std::map<float, float>)sMap3) );
    std::map<float, std::map<float, float>> mMap4;
    mMap4.insert ( std::pair<float, std::map<float, float>> ( 2.5e-3, (std::map<float, float>)sMap4) );
    std::map<float, std::map<float, float>> mMap5;
    mMap5.insert ( std::pair<float, std::map<float, float>> ( 2.8e-3, (std::map<float, float>)sMap5) );
    std::map<float, std::map<float, float>> mMap6;
    mMap6.insert ( std::pair<float, std::map<float, float>> ( 2.2e-3, (std::map<float, float>)sMap6) );

    std::map <float, std::map <float, std::map<float, float>>> lMap1;
    lMap1.insert ( std::pair<float, std::map <float, std::map<float, float>> > ( 0.5,  mMap1) );
    lMap1.insert ( std::pair<float, std::map <float, std::map<float, float>> > ( 0.65,  mMap1) );
    lMap1.insert ( std::pair<float, std::map <float, std::map<float, float>> > ( 0.65,  mMap1) );
    lMap1.insert ( std::pair<float, std::map <float, std::map<float, float>> > ( 0.65,  mMap1) );
    lMap1.insert ( std::pair<float, std::map <float, std::map<float, float>> > ( 0.5,  mMap1) );
    lMap1.insert ( std::pair<float, std::map <float, std::map<float, float>> > ( 0.5,  mMap1) );

    std::map <float, std::map <float, std::map<float, float>>> tempMap = lMap1;
/* 
    {
     {
      0.5,
      {
  	2.5e-3, {1,1}
      }
     },

     {
      0.65,
      {
	2.8e-3, {1,1}
      }
     }
      0.65,
      {
	2.2e-3, sMap
      },
      0.65,
      {
	2.5e-3, sMap
      },
      0.5,
      {
	2.8e-3, sMap
      },
      0.5,
      {
	2.2e-3, sMap
      } 
    };
*/
    return fMap;
  }
}
