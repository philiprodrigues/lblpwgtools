#include "CAFAna/Prediction/PredictionExtrap.h"
#include "FluxLinearSolver_Standalone.hxx"
#include "EvRateLinearSolver_Standalone.hxx"

#include "CAFAna/Extrap/IExtrap.h"
#include "CAFAna/Core/LoadFromFile.h"
#include "CAFAna/Core/Binning.h"
#include "CAFAna/Core/HistCache.h"
#include "CAFAna/Core/OscCurve.h"
#include "CAFAna/Core/Ratio.h"
#include "CAFAna/Core/Utilities.h"

#include "TDirectory.h"
#include "TObjString.h"
#include "TH1D.h"
#include "TSpline.h"

namespace ana
{
  //----------------------------------------------------------------------
  PredictionExtrap::PredictionExtrap(IExtrap* extrap, int offLocation, std::map <float, std::map <float, std::map<float, float>>> map)
    : fExtrap(extrap), pOff(offLocation), pMap(map)
  {
    std::cout<<"in  PredictionExtrap::PredictionExtrap(IExtrap* extrap, int offLocation, std::map <float, std::map <float, std::map<float, float>>> map)"<<std::endl;
  }

  //----------------------------------------------------------------------

  PredictionExtrap::PredictionExtrap(IExtrap* extrap, int offLocation)
    : fExtrap(extrap), pOff(offLocation)
  {
    std::cout<<"in  PredictionExtrap::PredictionExtrap(IExtrap* extrap, int offLocation)"<<std::endl;
  }
  //----------------------------------------------------------------------
  PredictionExtrap::~PredictionExtrap()
  {
    //    delete fExtrap;
  }

  //----------------------------------------------------------------------
  Spectrum PredictionExtrap::Predict(osc::IOscCalculator* calc) const
  {
    return PredictComponent(calc,
                            Flavors::kAll,
                            Current::kBoth,
                            Sign::kBoth);
  }

  //----------------------------------------------------------------------
/*
  Spectrum PredictionExtrap::PredictComponent(osc::IOscCalculator* calc,
                                              Flavors::Flavors_t flav,
                                              Current::Current_t curr,
                                              Sign::Sign_t sign) const
  {
    std::cout<<"in PredictionExtrap::PredictionComponent with off location value of  "<<pOff<<std::endl;
    //std::cout<<"testing fHist in PredictionExtrap::PredictionComponent"<<fHist->GetBinContent(2,2)<<" "<<fHist->GetBinContent(3,3)<<" "<<fHist->Integral()<<std::endl;
    Spectrum ret = fExtrap->NCComponent(); // Get binning
    ret.Clear();

    float OscScale = this -> GetOscScale(calc, pOff, pMap );
    OscScale = OscScale;

    if(curr & Current::kCC){
      if(flav & Flavors::kNuEToNuE    && sign & Sign::kNu)     {
	ret += fExtrap->NueSurvComponent().    Oscillated(calc, +12, +12, pOff);
      }
      if(flav & Flavors::kNuEToNuE    && sign & Sign::kAntiNu) ret += fExtrap->AntiNueSurvComponent().Oscillated(calc, -12, -12, pOff);

      if(flav & Flavors::kNuEToNuMu   && sign & Sign::kNu)     ret += fExtrap->NumuAppComponent().    Oscillated(calc, +12, +14, pOff);
      if(flav & Flavors::kNuEToNuMu   && sign & Sign::kAntiNu) ret += fExtrap->AntiNumuAppComponent().Oscillated(calc, -12, -14, pOff);

      if(flav & Flavors::kNuEToNuTau  && sign & Sign::kNu)     ret += fExtrap->TauFromEComponent().    Oscillated(calc, +12, +16, pOff);
      if(flav & Flavors::kNuEToNuTau  && sign & Sign::kAntiNu) ret += fExtrap->AntiTauFromEComponent().Oscillated(calc, -12, -16, pOff);

      if(flav & Flavors::kNuMuToNuE   && sign & Sign::kNu)     ret += fExtrap->NueAppComponent().    Oscillated(calc, +14, +12, pOff);
      if(flav & Flavors::kNuMuToNuE   && sign & Sign::kAntiNu) ret += fExtrap->AntiNueAppComponent().Oscillated(calc, -14, -12, pOff);

      if(flav & Flavors::kNuMuToNuMu  && sign & Sign::kNu)     ret += fExtrap->NumuSurvComponent().    Oscillated(calc, +14, +14, pOff);
      if(flav & Flavors::kNuMuToNuMu  && sign & Sign::kAntiNu) ret += fExtrap->AntiNumuSurvComponent().Oscillated(calc, -14, -14, pOff);

      if(flav & Flavors::kNuMuToNuTau && sign & Sign::kNu)     ret += fExtrap->TauFromMuComponent().    Oscillated(calc, +14, +16, pOff);
      if(flav & Flavors::kNuMuToNuTau && sign & Sign::kAntiNu) ret += fExtrap->AntiTauFromMuComponent().Oscillated(calc, -14, -16, pOff);
    }
    if(curr & Current::kNC){
      assert(flav == Flavors::kAll); // Don't know how to calculate anything else
      assert(sign == Sign::kBoth);   // Why would you want to split NCs out by sign?

      ret += fExtrap->NCComponent();
    }

    return ret;
  }
*/
  
  //-----------------------------------------------------------------------

  Spectrum PredictionExtrap::PredictComponent(osc::IOscCalculator* calc,
                                              Flavors::Flavors_t flav,
                                              Current::Current_t curr,
                                              Sign::Sign_t sign) const
  {
    std::cout<<"in PredictionExtrap::PredictionComponent with off location value of  "<<pOff<<std::endl;
    Spectrum ret = fvExtrap[0]->NCComponent(); // Get binning
    Spectrum target = fvExtrap[0]->NCComponent(); 
    ret.Clear();
    target.Clear();

    //float OscScale = this -> GetOscScale(calc, pOff, pMap );
    //OscScale = OscScale;
    std::cout<<"calculating current oscillation curve in Prediction Extra::PredictComponent()"<<std::endl;
    const OscCurve curveNumuSurv(calc, +14, +14);
    TSpline5* CAFProb = new TSpline5(curveNumuSurv.ToTH1());

    //TFile* outfile = new TFile("test_flux.root","RECREATE");

    Eigen::VectorXd cvals(fvExtrap.size());

    if(curr & Current::kCC){
      if(flav & Flavors::kNuMuToNuMu  && sign & Sign::kNu)  {
        target = fvExtrap[0]->NumuSurvComponent().Oscillated(calc, +14, +14);
        
        TH1* _target(target.ToTH1(1.e21));
        //_target->Write("target");
        //outfile->Write();
        //outfile->WriteTObject(_target);

        Eigen::VectorXd eigenTarget = Eigen::VectorXd::Zero(_target->GetNbinsX());
        Eigen::MatrixXd eigenFFMatrix = Eigen::MatrixXd::Zero(_target->GetNbinsX(),fvExtrap.size());

        for(Int_t i=0;i<_target->GetNbinsX();i++){
	  eigenTarget(i) = _target->GetBinContent(i+1);
          //std::cout<<"vector check "<<std::endl;
	  //std::cout<<eigenTarget(i)<<std::endl;
	}

        TH2F* test2 = new TH2F("","",_target->GetNbinsX(),0,_target->GetNbinsX(), fvExtrap.size(),0,fvExtrap.size());
        TH1* FFMatrix[fvExtrap.size()];
        for(Int_t cloop=0;cloop<fvExtrap.size();cloop++){
          FFMatrix[cloop] = fvExtrap[cloop]->NumuSurvComponent().Oscillated(calc, +14, +14, pOff).ToTH1(1.e21);
          for(Int_t j=0;j<FFMatrix[cloop]->GetNbinsX();j++){
            eigenFFMatrix(j, cloop) = FFMatrix[cloop]->GetBinContent(j+1);
            test2->SetBinContent(j,cloop, eigenFFMatrix(j, cloop) );
            //std::cout<<"matrix check "<<std::endl;
            //std::cout<<eigenFFMatrix(j, cloop)<<std::endl;
	  }
        }

        //test2->Write("test2");
	//outfile->Write();
	//outfile->WriteTObject(test2);

	//kQR , kInverse
        cvals = SolveEvRate(eigenFFMatrix,eigenTarget, kQR, 0); 

        //for(Int_t iii=0;iii<fvExtrap.size() ;iii++){
        //  std::cout<<iii<<" "<<cvals(iii)<<std::endl;
        //}

        Eigen::VectorXd fit_flux = eigenFFMatrix * cvals;
        TH1 * fit_flux_hist = static_cast<TH1 *>(_target->Clone("fit_flux_hist"));
        fit_flux_hist->Reset();
        FillHistFromEigenVector(fit_flux_hist, fit_flux);
        //fit_flux_hist->Write("test");
        //outfile->Write();
        //outfile->WriteTObject(fit_flux_hist);
      }
    }

 
    // ./FluxLinearSolver_Standalone -N /dune/data/users/gyang/FluxFit/ND_numode_OptimizedEngineeredNov2017Review_fit_binning_wppfx_0_45m.root,LBNF_numu_flux_Nom -F /dune/data/users/gyang/FluxFit/FD_numode_OptimizedEngineeredNov2017Review_w_PPFX_fit_binning.root,LBNF_numu_flux_Nom
    /*
    std::cout<<"initializing flux fit tool in Prediction Extra::PredictComponent()"<<std::endl;
    double RegFactor = 1.E-8;
    FluxLinearSolver fls;
    FluxLinearSolver::Params p = FluxLinearSolver::GetDefaultParams();
    //
    //Goes from -0.25 m, from first to second with step of third
    p.OffAxisRangesDescriptor = "0_34:0.5";

    fls.Initialize(p, {"/dune/data/users/gyang/FluxFit/ND_numode_OptimizedEngineeredNov2017Review_fit_binning_wppfx_0_45m.root", "LBNF_numu_flux_Nom"}, 
                      {"/dune/data/users/gyang/FluxFit/FD_numode_OptimizedEngineeredNov2017Review_w_PPFX_fit_binning.root", "LBNF_numu_flux_Nom"});

    std::cout<<"starting flux fit tool in Prediction Extra::PredictComponent()"<<std::endl;
    // Per fit step
    TH1 * unosc_flux = fls.GetFDFluxToOsc();
    //Oscillate unosc_flux -- you don't own it, don't delete it, copy it, do anything other than directly oscillate it once. If you need new oscillation parameters, call fls.GetFDFluxToOsc() again to get a fresh copy of the unoscillated flux.
    for(Int_t bin = 1; bin <= unosc_flux->GetNbinsX(); bin++){
      // e.g.
      //std::cout<<"testing CAFProb with energy and prob. : "<<unosc_flux->GetXaxis()->GetBinCenter(bin)<<" "<<CAFProb->Eval(unosc_flux->GetXaxis()->GetBinCenter(bin))<<std::endl;
      if(CAFProb->Eval(unosc_flux->GetXaxis()->GetBinCenter(bin))>0 && CAFProb->Eval(unosc_flux->GetXaxis()->GetBinCenter(bin))<1 ) 
	unosc_flux->SetBinContent(bin, unosc_flux->GetBinContent(bin) * CAFProb->Eval(unosc_flux->GetXaxis()->GetBinCenter(bin)));
       
      //std::cout<<"testing unosc_flux "<<unosc_flux->GetBinContent(bin)<<std::endl;
    }
    //std::array<double, 6> OscParameters{0.297,   0.0214,   0.534,
    //                                    7.37E-5, 2.539E-3, 0};
    //std::pair<int, int> OscChannel{14, 14};
    //fls.OscillateFDFlux(OscParameters, OscChannel); 
  
    fls.BuildTargetFlux(); //  This finds osc peaks and sets fit region and gaussian fall-off at low energy
    //fls.Solve(RegFactor);
    //Eigen::VectorXd cvals = fls.Solve(RegFactor);
    //std::cout<<"Sanity check for flux fit : "<<cvals(0)<<" "<<cvals(1)<<" "<<cvals(2)<<" "<<cvals(3)<<" "<<cvals(4)<<" "<<cvals(5)<<" "<<cvals(11)<<" "<<cvals(12)<<" "<<cvals(13)<<" "<<cvals(14)<<" "<<cvals(15)<<std::endl;
    std::cout<<"Sanity check for flux fit :"<<std::endl;
    for(Int_t iii=0;iii<fvExtrap.size() ;iii++){
      std::cout<<iii<<" "<<cvals(iii)<<std::endl;
    }

    //TFile* outfile = new TFile("test_flux.root","RECREATE");
    
    Eigen::VectorXd fit_flux = fls.FluxMatrix_Full * cvals;
    TH1 * fit_flux_hist = static_cast<TH1 *>(unosc_flux->Clone("fit_flux_hist"));
    fit_flux_hist->Reset();
    FillHistFromEigenVector(fit_flux_hist, fit_flux);
    fit_flux_hist->Write("test");
    outfile->Write();
    outfile->WriteTObject(fit_flux_hist);
    */

    if(curr & Current::kCC){
      if(flav & Flavors::kNuEToNuE    && sign & Sign::kNu)     {
        //ret += fvExtrap[0]->NueSurvComponent().    Oscillated(calc, +12, +12, pOff);
      }
      //if(flav & Flavors::kNuEToNuE    && sign & Sign::kAntiNu) ret += fvExtrap[0]->AntiNueSurvComponent().Oscillated(calc, -12, -12, pOff);

      //if(flav & Flavors::kNuEToNuMu   && sign & Sign::kNu)     ret += fvExtrap[0]->NumuAppComponent().    Oscillated(calc, +12, +14, pOff);
      //if(flav & Flavors::kNuEToNuMu   && sign & Sign::kAntiNu) ret += fvExtrap[0]->AntiNumuAppComponent().Oscillated(calc, -12, -14, pOff);

      //if(flav & Flavors::kNuEToNuTau  && sign & Sign::kNu)     ret += fvExtrap[0]->TauFromEComponent().    Oscillated(calc, +12, +16, pOff);
      //if(flav & Flavors::kNuEToNuTau  && sign & Sign::kAntiNu) ret += fvExtrap[0]->AntiTauFromEComponent().Oscillated(calc, -12, -16, pOff);

      //if(flav & Flavors::kNuMuToNuE   && sign & Sign::kNu)     ret += fvExtrap[0]->NueAppComponent().    Oscillated(calc, +14, +12, pOff);
      //if(flav & Flavors::kNuMuToNuE   && sign & Sign::kAntiNu) ret += fvExtrap[0]->AntiNueAppComponent().Oscillated(calc, -14, -12, pOff);

      //if(flav & Flavors::kNuMuToNuMu  && sign & Sign::kNu)     ret += fvExtrap[0]->NumuSurvComponent().    Oscillated(calc, +14, +14, pOff);
      //if(flav & Flavors::kNuMuToNuMu  && sign & Sign::kAntiNu) ret += fvExtrap[0]->AntiNumuSurvComponent().Oscillated(calc, -14, -14, pOff);

      if(flav & Flavors::kNuMuToNuMu  && sign & Sign::kNu){
        for(Int_t cloop=0;cloop<fvExtrap.size();cloop++){
	  //std::cout<<"Doing linear combination of ith sample with cval: "<<cloop<<" "<<cvals(cloop)<<std::endl;
          Spectrum tempSpec = fvExtrap[cloop]->NumuSurvComponent().Oscillated(calc, +14, +14, pOff);
	  tempSpec.Scale(cvals(cloop));
	  ret += tempSpec; //fvExtrap[0]->NumuSurvComponent().Oscillated(calc, +14, +14, pOff) .Scale(cvals(cloop));
        }
      }

      //if(flav & Flavors::kNuMuToNuTau && sign & Sign::kNu)     ret += fvExtrap[0]->TauFromMuComponent().    Oscillated(calc, +14, +16, pOff);
      //if(flav & Flavors::kNuMuToNuTau && sign & Sign::kAntiNu) ret += fvExtrap[0]->AntiTauFromMuComponent().Oscillated(calc, -14, -16, pOff);
    }
    if(curr & Current::kNC){
      assert(flav == Flavors::kAll); // Don't know how to calculate anything else
      assert(sign == Sign::kBoth);   // Why would you want to split NCs out by sign?

      //ret += fvExtrap[0]->NCComponent();
    }

    //TH1* testTH1 = ret.ToTH1(1.e21);
    //for(Int_t bin = 1; bin <= testTH1->GetNbinsX(); bin++){
    //  std::cout<<"testing ret at the end of predictComponent() "<<testTH1->GetBinContent(bin)<<std::endl;
    //}    

    return ret;
  }

  //----------------------------------------------------------------------
  OscillatableSpectrum PredictionExtrap::ComponentCC(int from, int to) const
  {
    if(from == +12 && to == +12) return fExtrap->NueSurvComponent();
    if(from == -12 && to == -12) return fExtrap->AntiNueSurvComponent();

    if(from == +12 && to == +14) return fExtrap->NumuAppComponent();
    if(from == -12 && to == -14) return fExtrap->AntiNumuAppComponent();

    if(from == +12 && to == +16) return fExtrap->TauFromEComponent();
    if(from == -12 && to == -16) return fExtrap->AntiTauFromEComponent();

    if(from == +14 && to == +12) return fExtrap->NueAppComponent();
    if(from == -14 && to == -12) return fExtrap->AntiNueAppComponent();

    if(from == +14 && to == +14) return fExtrap->NumuSurvComponent();
    if(from == -14 && to == -14) return fExtrap->AntiNumuSurvComponent();

    if(from == +14 && to == +16) return fExtrap->TauFromMuComponent();
    if(from == -14 && to == -16) return fExtrap->AntiTauFromMuComponent();

    assert(0 && "Not reached");
  }

  //----------------------------------------------------------------------
  Spectrum PredictionExtrap::ComponentNC() const
  {
    return fExtrap->NCComponent();
  }

  //----------------------------------------------------------------------

  float PredictionExtrap::GetOscScale(osc::IOscCalculator* calc, int fOff, std::map <float, std::map <float, std::map<float, float>>> fMap) const
  {
    //osc::IOscCalculatorAdjustable* Calc = calc->Copy();
    const OscCurve curve(calc, 14, 14);
    TH1* Curve = curve.ToTH1();
    std::cout<<"in GetOscScale, contents in Curve "<<Curve->GetBinContent(3)<<" "<<Curve->GetBinContent(4)<<std::endl;
    //std::cout<<"in GetOscScale with th23 and dm32 : "<<Calc->GetTh23()<<" "<<Calc->GetDmsq32()<<std::endl;     
    // Here adding flux fitting

    return 0 ;

  }

  //----------------------------------------------------------------------
  void PredictionExtrap::SaveTo(TDirectory* dir) const
  {
    TDirectory* tmp = gDirectory;

    dir->cd();

    TObjString("PredictionExtrap").Write("type");

    fExtrap->SaveTo(dir->mkdir("extrap"));

    tmp->cd();
  }

  //----------------------------------------------------------------------
  std::unique_ptr<PredictionExtrap> PredictionExtrap::LoadFrom(TDirectory* dir)
  {
    assert(dir->GetDirectory("extrap"));
    IExtrap* extrap = ana::LoadFrom<IExtrap>(dir->GetDirectory("extrap")).release();

    return std::unique_ptr<PredictionExtrap>(new PredictionExtrap(extrap , 0 ));
  }
}
