#include "CAFAna/Core/SpectrumLoader.h"
#include "CAFAna/Core/Spectrum.h"
#include "CAFAna/Core/Loaders.h"
#include "CAFAna/Prediction/PredictionGenerator.h"
#include "CAFAna/Prediction/PredictionInterp.h"
#include "CAFAna/Prediction/PredictionNoExtrap.h"
#include "CAFAna/Prediction/PredictionOffExtrap.h"
#include "CAFAna/Analysis/Calcs.h"
#include "CAFAna/Analysis/Plots.h"
#include "CAFAna/Analysis/Style.h"
#include "CAFAna/Analysis/Surface.h"
#include "CAFAna/Analysis/Fit.h"
#include "CAFAna/Vars/FitVars.h"
#include "CAFAna/Experiment/SingleSampleExperiment.h"
#include "CAFAna/Experiment/MultiExperiment.h"
#include "CAFAna/Cuts/TruthCuts.h"
#include "CAFAna/Systs/DUNEXSecSysts.h"
#include "CAFAna/Systs/DUNEFluxSysts.h"
#include "CAFAna/Prediction/LinComb.h"

using namespace ana;

#include "StandardRecord/StandardRecord.h"
#include "OscLib/func/IOscCalculator.h"

#include "Utilities/rootlogon.C"

#include "TCanvas.h"
#include "TGraph.h"
#include "TH2.h"
#include "TLegend.h"
#include "TPad.h"

void Legend()
{
  TLegend* leg = new TLegend(.6, .6, .9, .85);
  leg->SetFillStyle(0);

  TH1* dummy = new TH1F("", "", 1, 0, 1);
  dummy->SetMarkerStyle(kFullCircle);
  leg->AddEntry(dummy->Clone(), "Fake Data", "lep");
  dummy->SetLineColor(kTotalMCColor);
  leg->AddEntry(dummy->Clone(), "Total MC", "l");
  dummy->SetLineColor(kNCBackgroundColor);
  leg->AddEntry(dummy->Clone(), "NC", "l");
  dummy->SetLineColor(kNumuBackgroundColor);
  leg->AddEntry(dummy->Clone(), "#nu_{#mu} CC", "l");
  dummy->SetLineColor(kBeamNueBackgroundColor);
  leg->AddEntry(dummy->Clone(), "Beam #nu_{e} CC", "l");

  leg->Draw();
}

void test_prism()
{
  rootlogon(); // style

  Loaders dummyLoaders;

  // POT/yr * 3.5yrs * mass correction
  const double pot = 3.5 * 1.47e21 * 40/1.13;

  SystVector<DUNEXSecSyst> rett;
  for(int i = 0; i < 32; ++i) rett.push_back(GetDUNEXSecSyst(EVALORCategory(i)));

  const std::vector<const ISyst*> fluxSysts = {GetDUNEFluxSyst(0) ,GetDUNEFluxSyst(1), GetDUNEFluxSyst(2)};
  std::vector<const ISyst*> allSysts;
  allSysts.insert( allSysts.end(), rett.begin(), rett.end() );
  allSysts.insert( allSysts.end(), fluxSysts.begin(), fluxSysts.end() );

  std::vector<const ISyst*> useSysts;
  useSysts.insert( useSysts.end(), fluxSysts.begin(), fluxSysts.end() );

  std::string inFile1 = "/dune/data/users/marshalc/CAFs/mcc10_test/FarDetector/FHC_nonswap.root";
  std::string inFile2 = "/pnfs/dune/persistent/users/gyang/CAF/CAF/past_3rd/CAF_FHC.root";
  std::string inFile = inFile2;

  SpectrumLoader loaderNonswap(inFile);
  SpectrumLoader loaderNonswapOff1(inFile);
  SpectrumLoader loaderNonswapOff2(inFile);
  SpectrumLoader loaderNonswapOff3(inFile);

  SpectrumLoaderBase& loaderNue   = kNullLoader;
  SpectrumLoaderBase& loaderNuTau = kNullLoader;

  const Var Enu_recoNumu = SIMPLEVAR(dune.Ev_reco_numu);
  const Var Enu_recoNue = SIMPLEVAR(dune.Ev_reco_nue);
  const Var detLocation = SIMPLEVAR(dune.det_x);
  const Cut kSelNumu = SIMPLEVAR(dune.cvnnumu) > 0.7;
  const Cut kSelNue = SIMPLEVAR(dune.cvnnue) > 0.7;

  osc::IOscCalculatorAdjustable* calc0 = DefaultOscCalc();
  calc0->SetL(1300);
  calc0->SetdCP(TMath::Pi()*1.5);
  calc0->SetTh12(0.5857);
  calc0->SetTh13(0.148);
  calc0->SetTh23(0.726);
  calc0->SetDmsq21(0.000075);
  calc0->SetDmsq32(0.002524-0.000075);

  // pre-load a c value map, NOT BEING USED!
  LinComb linComb("","nashome/g/gyang/testArea/lblpwgtools/code/CAFAna/releases/development/CAFAna/Prediction");
  std::map <float, std::map <float, std::map<float, float>>> map = linComb.GetMap();

  PredictionOffExtrap predNumuPID(loaderNonswap, loaderNue, loaderNuTau, "PID", Binning::Simple(60, -.1, +1.1), SIMPLEVAR(dune.cvnnumu) ,1 , kNoCut,map, kNoShift);

  PredictionOffExtrap predNuePID(loaderNonswap, loaderNue, loaderNuTau, "PID", Binning::Simple(60, -.1, +1.1), SIMPLEVAR(dune.cvnnue) ,1 , kNoCut, map, kNoShift);

  //SpectrumLoaderBase& dummyLoaders = kNullLoader;

  // offLocation amd pre-loaded map are NOT USED!
  DUNEOffExtrapPredictionGenerator genFDNumuFHC(loaderNonswap,
                                                loaderNue,
                                                loaderNuTau,
                                                loaderNuTau,
                                                HistAxis("Neutrino Energy (GeV)", Binning::Simple(38, 0.5, 10), SIMPLEVAR(dune.Ev)),
                                                SIMPLEVAR(dune.nuPDG) == 14, map, 0, kUnweighted);

  PredictionInterp pred(useSysts, calc0, genFDNumuFHC, dummyLoaders);

  //PredictionOffExtrap pred(loaderNonswap, loaderNue, loaderNuTau, HistAxis( "Neutrino Energy (GeV)", Binning::Simple(40, 0, 10), SIMPLEVAR(dune.Ev)), SIMPLEVAR(dune.nuPDG) == 14 , map, kNoShift, 1);

  //PredictionOffExtrap pred(loaderNonswap, loaderNue, loaderNuTau, "Reconstructed E (GeV)", Binning::Simple(80, 0, 10), Enu_recoNumu, 1, kNoCut, map, kNoShift);

  //PredictionNoExtrap predOff1(loaderNonswapOff1, loaderNue, loaderNuTau, "Reconstructed E (GeV)", Binning::Simple(80, 0, 10), Enu_recoNumu, 1, kSelNumu, map, kNoShift, 1);

  //PredictionNoExtrap predOff2(loaderNonswapOff2, loaderNue, loaderNuTau, "Reconstructed E (GeV)", Binning::Simple(80, 0, 10), Enu_recoNumu, kSelNumu, map, kNoShift, 1);

  //PredictionNoExtrap predOff3(loaderNonswapOff3, loaderNue, loaderNuTau, "Reconstructed E (GeV)", Binning::Simple(80, 0, 10), Enu_recoNumu, kSelNumu, map, kNoShift, 1);

  PredictionOffExtrap predNue(loaderNonswap, loaderNue, loaderNuTau, "Reconstructed E (GeV)", Binning::Simple(24, 0, 6), Enu_recoNue, 1, kSelNue, map, kNoShift);

  //DUNENoExtrapPredictionGenerator genFDNumuFHC(*loaderNonswapOff1, axis,
  //                                               kSelNumu);

  /*
  // separate by true interaction category
  std::vector<Cut> truthcuts;
  for( int i = 0; i < 32; ++i ) {
    truthcuts.push_back( kVALORCategory == i );
  }
  const HistAxis axis("Reconstructed energy (GeV)",
                      Binning::Simple(40, 0, 10),
                      Enu_recoNumu);
  PredictionScaleComp predNumu(loaderNonswap, loaderNue, loaderNuTau, axis, kSelNumu, truthcuts);
  */

  loaderNonswap.Go();
  std::cout<<"finished a Go() "<<std::endl;

  loaderNue.Go();

  std::cout<<"Finished Go()"<<std::endl;

  SaveToFile(pred, "pred_numu.root", "pred");
  SaveToFile(predNue, "pred_nue.root", "pred");

  osc::IOscCalculatorAdjustable* calc = DefaultOscCalc();
  calc->SetL(1300);
  calc->SetdCP(TMath::Pi()*1.5);


  // Standard DUNE numbers from Matt Bass
  calc->SetTh12(0.5857);
  calc->SetTh13(0.148);
  calc->SetTh23(0.726);
  calc->SetDmsq21(0.000075);
  calc->SetDmsq32(0.002524-0.000075); // quoted value is 31

  // One sigma errors
  // (t12,t13,t23,dm21,dm32)=(0.023,0.018,0.058,0.0,0.024,0.016)

  osc::IOscCalculatorAdjustable* calcT = DefaultOscCalc();
  calcT->SetL(1300);
  calcT->SetdCP(0);
  calcT->SetTh12(0.5857);
  calcT->SetTh13(0.148);
  calcT->SetTh23(0.726); 
  calcT->SetDmsq21(0.000075);
  calcT->SetDmsq32(0.002524-0.000075); 

  Spectrum mock = pred.Predict(calcT).FakeData(pot);
  SingleSampleExperiment expt(&pred, mock);

  Spectrum mockNue = predNue.Predict(calc).FakeData(pot);
  SingleSampleExperiment exptNue(&predNue, mockNue);

  Spectrum mockNuePID = predNuePID.Predict(calc).FakeData(pot);
  Spectrum mockNumuPID = predNumuPID.Predict(calc).FakeData(pot);

  //Surface surf(&expt, calc, &kFitSinSqTheta23, 20, .4, .6, &kFitDmSq32Scaled, 20, 2.35, 2.5);

  MultiExperiment me({&expt, &exptNue});

  //Surface surfNue(&me, calc, &kFitDeltaInPiUnits, 20, 0, 2, &kFitSinSqTheta23, 20, .4, .6);

  std::map<const ISyst*,double> shiftmap;
  shiftmap.emplace(GetDUNEFluxSyst(0), +1);
  SystShifts shifts(shiftmap);
  
  std::map<const ISyst*,double> NOshiftmap;
  SystShifts NOshifts(NOshiftmap);

  const std::vector<const IFitVar*> oscFitVars = {&kFitSinSqTheta23,
                                                  &kFitDmSq32Scaled,
                                                  &kFitSinSq2Theta13, &kFitSinSq2Theta12, &kFitDmSq21 , &kFitDeltaInPiUnits};

  //SystShifts fakeDataShift(pred.GetSysts()[k_int_nu_MEC_dummy], +1);
  Spectrum fake = pred.PredictSyst(calcT, NOshifts).FakeData(1.47e21);

  fake.ToTH1(1.47e21, kRed)->Draw("hist");
  pred.Predict(calc).ToTH1(1.47e21)->Draw("hist same");

  SingleSampleExperiment mecexpt(&pred, fake);

  const std::vector<const ISyst*> systFitVars = useSysts;
  Fitter fit(&mecexpt, oscFitVars, systFitVars);

  osc::IOscCalculatorAdjustable* oscSeed = DefaultOscCalc();
  SystShifts systSeed = SystShifts::Nominal();
  fit.Fit(oscSeed, systSeed, Fitter::kQuiet);

  for (std::vector<const ISyst*>::iterator it = useSysts.begin() ; it != useSysts.end(); ++it){
    std::cout << "Syst. shift  " <<(*it)->ShortName().c_str() <<" "<< systSeed.GetShift(*it) << std::endl;
  }

  //SystShifts bfs(pred.GetSysts()[k_int_nu_MEC_dummy], seed.GetShift(pred.GetSysts()[k_int_nu_MEC_dummy]));
  SystShifts bfs;
  Spectrum bf = pred.PredictSyst(oscSeed, systSeed);
  //Spectrum bf2 = pred.PredictSyst(calc, systSeed);
  //bf.ToTH1(1.47e21, kBlue)->Draw("hist same");
  //bf2.ToTH1(1.47e21, kBlue, 7)->Draw("hist same");

  new TCanvas;
  fake.ToTH1(pot, kRed)->Draw("ep");
  pred.Predict(calc).ToTH1(pot, kBlack)->Draw("hist same");
  bf.ToTH1(pot, kBlue)->Draw("hist same");
  Legend();

  //gPad->Print("FD_mec_fit.eps");

/*
  new TCanvas;
  TH1* h3 = DataMCComparisonComponents(mock, &pred, calc);
  h3->SetTitle("#nu_{#mu} selection (CVNm>0.7) 3.5yrs #times 40kt");
  CenterTitles(h3);
  Legend();
  gPad->Print("components.eps");
  gPad->Print("components.C");

  new TCanvas;
  TH1* h4 = DataMCComparisonComponents(mockNue, &predNue, calc);
  h4->SetTitle("#nu_{e} selection (CVNe>0.7) 3.5yrs #times 40kt");
  CenterTitles(h4);
  Legend();
  gPad->Print("components_nue.eps");
  gPad->Print("components_nue.C");

  new TCanvas;
  TH1* h2 = DataMCComparisonComponents(mockNumuPID, &predNumuPID, calc);
  h2->SetTitle("#nu_{#mu} selection (CVNm>0.7) 3.5yrs #times 40kt");
  CenterTitles(h2);
  Legend();
  h2->GetYaxis()->SetRangeUser(1, 1e4);
  gPad->SetLogy();
  gPad->Print("components_pid.eps");
  gPad->Print("components_pid.C");

  new TCanvas;
  TH1* h = DataMCComparisonComponents(mockNuePID, &predNuePID, calc);
  h->SetTitle("#nu_{e} selection (CVNe>0.7) 3.5yrs #times 40kt");
  CenterTitles(h);
  Legend();
  h->GetYaxis()->SetRangeUser(0, 600);
  gPad->Print("components_nue_pid.eps");
  gPad->Print("components_nue_pid.C");

  new TCanvas;
  surf.DrawContour(Gaussian68Percent2D(surf), 7, kRed);
  surf.DrawContour(Gaussian90Percent2D(surf), kSolid, kRed);
  surf.DrawBestFit(kRed);
  gPad->Print("cont.eps");

  new TCanvas;
  surfNue.DrawContour(Gaussian68Percent2D(surfNue), 7, kRed);
  surfNue.DrawContour(Gaussian90Percent2D(surfNue), kSolid, kRed);
  surfNue.DrawBestFit(kRed);
  gPad->Print("cont_nue.eps");


  new TCanvas;

  // This is a very cheesy way to make the McD plot - would have to be very
  // different if we were varying any other parameters
  calc->SetdCP(0);
  Spectrum zeroNumu = pred.Predict(calc).FakeData(pot);
  Spectrum zeroNue = predNue.Predict(calc).FakeData(pot);
  calc->SetdCP(TMath::Pi());
  Spectrum oneNumu = pred.Predict(calc).FakeData(pot);
  Spectrum oneNue = predNue.Predict(calc).FakeData(pot);

  TGraph* g = new TGraph;

  for(int i = -100; i <= 200; ++i){
    calc->SetdCP(i/100.*TMath::Pi());

    Spectrum mockNumu = pred.Predict(calc).FakeData(pot);
    Spectrum mockNue = predNue.Predict(calc).FakeData(pot);

    const double llZero = //LogLikelihood(zeroNumu.ToTH1(pot), mockNumu.ToTH1(pot))+
      LogLikelihood(zeroNue.ToTH1(pot), mockNue.ToTH1(pot));

    const double llOne = //LogLikelihood(oneNumu.ToTH1(pot), mockNumu.ToTH1(pot))+
      LogLikelihood(oneNue.ToTH1(pot), mockNue.ToTH1(pot));

    const double ll = std::min(llZero, llOne);

    g->SetPoint(g->GetN(), i/100., sqrt(ll));
  }

  TH2* axes = new TH2F("", "3.5yrs #times 40kt, stats only;#delta / #pi;#sqrt{#chi^{2}}", 100, -1, +1, 100, 0, 6);
  CenterTitles(axes);
  axes->Draw();
  g->SetLineWidth(2);
  g->Draw("l same");

  gPad->SetGridx();
  gPad->SetGridy();

  gPad->Print("mcd.eps");
  gPad->Print("mcd.C");
*/

/*  
  SystShifts fakeDataShift(predNumu.GetSysts()[k_int_nu_MEC_dummy], +1);
  Spectrum fake = predNumu.PredictSyst(calc, fakeDataShift).FakeData(1.47e21);

  fake.ToTH1(1.47e21, kRed)->Draw("hist");
  predNumu.Predict(calc).ToTH1(1.47e21)->Draw("hist same");

  SingleSampleExperiment mecexpt(&predNumu, fake);

  Fitter fit(&mecexpt, {}, predNumu.GetSysts());
  SystShifts seed = SystShifts::Nominal();
  fit.Fit(seed);

  SystShifts bfs(predNumu.GetSysts()[k_int_nu_MEC_dummy], seed.GetShift(predNumu.GetSysts()[k_int_nu_MEC_dummy]));
  Spectrum bf = predNumu.PredictSyst(calc, bfs);
  Spectrum bf2 = predNumu.PredictSyst(calc, seed);
  bf.ToTH1(1.47e21, kBlue)->Draw("hist same");
  bf2.ToTH1(1.47e21, kBlue, 7)->Draw("hist same");

  gPad->Print("FD_mec_fit.eps");
  */
}
