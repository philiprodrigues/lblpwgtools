#include "CAFAna/Prediction/PredictionInterp.h"
#include "CAFAna/Core/HistCache.h"
#include "CAFAna/Core/LoadFromFile.h"
#include "CAFAna/Core/Ratio.h"
#include "CAFAna/Core/SystRegistry.h"
#include "CAFAna/Core/Utilities.h"

#include "TDirectory.h"
#include "TH2.h"
#include "TMatrixD.h"
#include "TObjString.h"
#include "TVectorD.h"

// For debug plots
#include "TGraph.h"
#include "TCanvas.h"

#include "CAFAna/Core/Loaders.h"

#include <algorithm>
#include <malloc.h>

#ifdef USE_PREDINTERP_OMP
#include <omp.h>
#endif

namespace ana
{
  //----------------------------------------------------------------------
  PredictionInterp::PredictionInterp(std::vector<const ISyst*> systs,
                                     osc::IOscCalculator* osc,
                                     const IPredictionGenerator& predGen,
                                     Loaders& loaders,
                                     const SystShifts& shiftMC,
                                     EMode_t mode)
    : fOscOrigin(osc ? osc->Copy() : 0),
      fBinning(0, {}, {}, 0, 0),
      fSplitBySign(mode == kSplitBySign)
  {

    if(getenv("CAFANA_PRED_MINMCSTATS")){
      fMinMCStats = atoi(getenv("CAFANA_PRED_MINMCSTATS"));
    } else {
      fMinMCStats = 50;
    }


    for(const ISyst* syst: systs){
      ShiftedPreds sp;
      sp.systName = syst->ShortName();

      // Use whichever of these gives the most restrictive range
      const int x0 = std::max(-syst->PredInterpMaxNSigma(), int(trunc(syst->Min())));
      const int x1 = std::min(+syst->PredInterpMaxNSigma(), int(trunc(syst->Max())));

      for(int x = x0; x <= x1; ++x) sp.shifts.push_back(x);

      for(int sigma: sp.shifts){
        SystShifts shiftHere = shiftMC;
        shiftHere.SetShift(syst, sigma);
        sp.preds.emplace_back(predGen.Generate(loaders, shiftHere));
      }

      fPreds.emplace_back(syst, std::move(sp));
    } // end for syst

    fPredNom = predGen.Generate(loaders, shiftMC);
  }

  //----------------------------------------------------------------------
  PredictionInterp::~PredictionInterp(){}

  //----------------------------------------------------------------------
  std::vector<std::vector<PredictionInterp::Coeffs>> PredictionInterp::
  FitRatios(const std::vector<double>& shifts,
            const std::vector<std::unique_ptr<TH1>>& ratios) const
  {
    assert(shifts.size() == ratios.size());

    std::vector<std::vector<Coeffs>> ret;

    const int binMax = ratios[0]->GetNbinsX();

    for(int binIdx = 0; binIdx < binMax+2; ++binIdx){
      ret.push_back({});

      // This is cubic interpolation. For each adjacent set of four points we
      // determine coefficients for a cubic which will be the curve between the
      // center two. We constrain the function to match the two center points
      // and to have the right mean gradient at them. This causes this patch to
      // match smoothly with the next one along. The resulting function is
      // continuous and first and second differentiable. At the ends of the
      // range we fit a quadratic instead with only one constraint on the
      // slope. The coordinate conventions are that point y1 sits at x=0 and y2
      // at x=1. The matrices are simply the inverses of writing out the
      // constraints expressed above.

      // Special-case for linear interpolation
      if(ratios.size() == 2){
        const double y0 = ratios[0]->GetBinContent(binIdx);
        const double y1 = ratios[1]->GetBinContent(binIdx);

        ret.back().emplace_back(0, 0, y1-y0, y0);
        continue;
      }

      {
        const double y1 = ratios[0]->GetBinContent(binIdx);
        const double y2 = ratios[1]->GetBinContent(binIdx);
        const double y3 = ratios[2]->GetBinContent(binIdx);
        const double v[3] = {y1, y2, (y3-y1)/2};
        const double m[9] = { 1, -1,  1,
                             -2,  2, -1,
                              1,  0,  0};
        const TVectorD res = TMatrixD(3, 3, m) * TVectorD(3, v);
        ret.back().emplace_back(0, res(0), res(1), res(2));
      }

      // We're assuming here that the shifts are separated by exactly 1 sigma.
      for(unsigned int shiftIdx = 1; shiftIdx < ratios.size()-2; ++shiftIdx){
        const double y0 = ratios[shiftIdx-1]->GetBinContent(binIdx);
        const double y1 = ratios[shiftIdx  ]->GetBinContent(binIdx);
        const double y2 = ratios[shiftIdx+1]->GetBinContent(binIdx);
        const double y3 = ratios[shiftIdx+2]->GetBinContent(binIdx);

        const double v[4] = {y1, y2, (y2-y0)/2, (y3-y1)/2};
        const double m[16] = { 2, -2,  1,  1,
                              -3,  3, -2, -1,
                               0,  0,  1,  0,
                               1,  0,  0,  0};
        const TVectorD res = TMatrixD(4, 4, m) * TVectorD(4, v);
        ret.back().emplace_back(res(0), res(1), res(2), res(3));
      } // end for shiftIdx

      {
        const int N = ratios.size()-3;
        const double y0 = ratios[N  ]->GetBinContent(binIdx);
        const double y1 = ratios[N+1]->GetBinContent(binIdx);
        const double y2 = ratios[N+2]->GetBinContent(binIdx);
        const double v[3] = {y1, y2, (y2-y0)/2};
        const double m[9] = {-1,  1, -1,
                              0,  0,  1,
                              1,  0,  0};
        const TVectorD res = TMatrixD(3, 3, m) * TVectorD(3, v);
        ret.back().emplace_back(0, res(0), res(1), res(2));
      }
    } // end for binIdx

    double stride = -1;
    for(unsigned int i = 0; i < shifts.size()-1; ++i){
      const double newStride = shifts[i+1]-shifts[i];
      assert((stride < 0 || fabs(stride-newStride) < 1e-3) &&
             "Variably-spaced syst templates are unsupported");
      stride = newStride;
    }

    // If the stride is actually not 1, need to rescale all the coefficients
    for(std::vector<Coeffs>& cs: ret)
      for(Coeffs& c: cs){
        c = Coeffs(c.a/util::cube(stride),
                   c.b/util::sqr(stride),
                   c.c/stride,
                   c.d);}
    return ret;
  }

  //----------------------------------------------------------------------
  std::vector<std::vector<PredictionInterp::Coeffs>> PredictionInterp::
  FitComponent(const std::vector<double>& shifts,
               const std::vector<std::unique_ptr<IPrediction>>& preds,
               Flavors::Flavors_t flav,
               Current::Current_t curr,
               Sign::Sign_t sign,
               const std::string& systName) const
  {
    IPrediction const *pNom = nullptr;
    for(unsigned int i = 0; i < shifts.size(); ++i){
      if(shifts[i] == 0) {pNom = preds[i].get(); };
    }
    assert(pNom);

    // Do it this way rather than via fPredNom so that systematics evaluated
    // relative to some alternate nominal (eg Birks C where the appropriate
    // nominal is no-rock) can work.
    const Spectrum nom = pNom->PredictComponent(fOscOrigin,
						flav, curr, sign);

    std::vector<std::unique_ptr<TH1>> ratios;
    for(auto& p: preds){
      ratios.emplace_back(Ratio(p->PredictComponent(fOscOrigin,
                                                    flav, curr, sign),
                                nom).ToTH1());
      
      // Check none of the ratio values is crazy
      std::unique_ptr<TH1>& r = ratios.back();
      for(int i = 0; i < r->GetNbinsX()+2; ++i){
        const double y = r->GetBinContent(i);
        if(y > 2){
          // std::cout << "PredictionInterp: WARNING, ratio in bin "
          // 	    << i << " for " << shifts[&p-&preds.front()]
          //           << " sigma shift of " << systName << " is " << y
          //           << " which exceeds limit of 2. Capping." << std::endl;
          r->SetBinContent(i, 2);
        }
      }
    }
    
    return FitRatios(shifts, ratios);
  }

  //----------------------------------------------------------------------
  void PredictionInterp::InitFitsHelper(ShiftedPreds& sp,
                                        std::vector<std::vector<std::vector<Coeffs>>>& fits,
                                        Sign::Sign_t sign) const
  {
    fits.resize(kNCoeffTypes);

    fits[kNueApp]   = FitComponent(sp.shifts, sp.preds, Flavors::kNuMuToNuE,  Current::kCC, sign, sp.systName);
    fits[kNueSurv]  = FitComponent(sp.shifts, sp.preds, Flavors::kNuEToNuE,   Current::kCC, sign, sp.systName);
    fits[kNumuSurv] = FitComponent(sp.shifts, sp.preds, Flavors::kNuMuToNuMu, Current::kCC, sign, sp.systName);

    fits[kNC]       = FitComponent(sp.shifts, sp.preds, Flavors::kAll, Current::kNC, sign, sp.systName);

    fits[kOther] = FitComponent(sp.shifts, sp.preds, Flavors::kNuEToNuMu | Flavors::kAllNuTau, Current::kCC, sign, sp.systName);
  }

  //----------------------------------------------------------------------
  void PredictionInterp::InitFits() const
  {
    // No systs
    if(fPreds.empty()){
      if(fBinning.POT() > 0 || fBinning.Livetime() > 0) return;
    }
    // Already initialized
    else if(!fPreds.empty() && !fPreds.begin()->second.fits.empty()) return;

    for(auto& it: fPreds){
      ShiftedPreds& sp = it.second;

      if(fSplitBySign){
        InitFitsHelper(sp, sp.fits, Sign::kNu);
        InitFitsHelper(sp, sp.fitsNubar, Sign::kAntiNu);
      }
      else{
        InitFitsHelper(sp, sp.fits, Sign::kBoth);
      }
      sp.nCoeffs = sp.fits[0][0].size();

      // Copy the outputs into the remapped indexing order. TODO this is very
      // ugly. Best would be to generate things in this order natively.

      sp.fitsRemap.resize(sp.fits.size());
      for(auto& it: sp.fitsRemap){
        it.resize(sp.fits[0][0].size());
        for(auto& it2: it) it2.resize(sp.fits[0].size(), Coeffs(0, 0, 0, 0));
      }

      for(unsigned int i = 0; i < sp.fitsRemap.size(); ++i){
        for(unsigned int j = 0; j < sp.fitsRemap[i].size(); ++j){
          for(unsigned int k = 0; k < sp.fitsRemap[i][j].size(); ++k){
            sp.fitsRemap[i][j][k] = sp.fits[i][k][j];
          }
        }
      }

      sp.fitsRemapAVX2.resize(sp.fits.size());
      for(auto& it: sp.fitsRemapAVX2){
        it.resize(sp.fits[0][0].size());
        for(auto& it2: it) it2.resize(sp.fits[0].size()/4+1, CoeffsAVX2(_mm256_setzero_pd(),
                                                                        _mm256_setzero_pd(),
                                                                        _mm256_setzero_pd(),
                                                                        _mm256_setzero_pd()));
      }

      for(unsigned int i = 0; i < sp.fitsRemapAVX2.size(); ++i){
        for(unsigned int j = 0; j < sp.fitsRemapAVX2[i].size(); ++j){
          for(unsigned int k = 0; k < sp.fitsRemapAVX2[i][j].size(); ++k){
#define GETCOEFF(offset, coeff) sp.fits[i].size()>(4*k+(offset)) ? sp.fits[i][4*k+(offset)][j].coeff : 0
            sp.fitsRemapAVX2[i][j][k].a = _mm256_setr_pd(GETCOEFF(0, a),
                                                        GETCOEFF(1, a),
                                                        GETCOEFF(2, a),
                                                        GETCOEFF(3, a));

            sp.fitsRemapAVX2[i][j][k].b = _mm256_setr_pd(GETCOEFF(0, b),
                                                        GETCOEFF(1, b),
                                                        GETCOEFF(2, b),
                                                        GETCOEFF(3, b));

            sp.fitsRemapAVX2[i][j][k].c = _mm256_setr_pd(GETCOEFF(0, c),
                                                        GETCOEFF(1, c),
                                                        GETCOEFF(2, c),
                                                        GETCOEFF(3, c));

            sp.fitsRemapAVX2[i][j][k].d = _mm256_setr_pd(GETCOEFF(0, d),
                                                        GETCOEFF(1, d),
                                                        GETCOEFF(2, d),
                                                        GETCOEFF(3, d));

          }
        }
      }

      sp.fitsNubarRemap.resize(sp.fitsNubar.size());
      for(auto& it: sp.fitsNubarRemap){
        it.resize(sp.fitsNubar[0][0].size());
        for(auto& it2: it) it2.resize(sp.fitsNubar[0].size(), Coeffs(0, 0, 0, 0));
      }

      for(unsigned int i = 0; i < sp.fitsNubarRemap.size(); ++i){
        for(unsigned int j = 0; j < sp.fitsNubarRemap[i].size(); ++j){
          for(unsigned int k = 0; k < sp.fitsNubarRemap[i][j].size(); ++k){
            sp.fitsNubarRemap[i][j][k] = sp.fitsNubar[i][k][j];
          }
        }
      }

      sp.fitsNubarRemapAVX2.resize(sp.fitsNubar.size());
      for(auto& it: sp.fitsNubarRemapAVX2){
        it.resize(sp.fitsNubar[0][0].size());
        for(auto& it2: it) it2.resize(sp.fitsNubar[0].size()/4+1, CoeffsAVX2(_mm256_setzero_pd(),
                                                                             _mm256_setzero_pd(),
                                                                             _mm256_setzero_pd(),
                                                                             _mm256_setzero_pd()));
      }

      for(unsigned int i = 0; i < sp.fitsNubarRemapAVX2.size(); ++i){
        for(unsigned int j = 0; j < sp.fitsNubarRemapAVX2[i].size(); ++j){
          for(unsigned int k = 0; k < sp.fitsNubarRemapAVX2[i][j].size(); ++k){
#define GETCOEFF2(offset, coeff) sp.fitsNubar[i].size()>(4*k+(offset)) ? sp.fitsNubar[i][4*k+(offset)][j].coeff : 0
            sp.fitsNubarRemapAVX2[i][j][k].a = _mm256_setr_pd(GETCOEFF2(0, a),
                                                             GETCOEFF2(1, a),
                                                             GETCOEFF2(2, a),
                                                             GETCOEFF2(3, a));
            
            sp.fitsNubarRemapAVX2[i][j][k].b = _mm256_setr_pd(GETCOEFF2(0, b),
                                                             GETCOEFF2(1, b),
                                                             GETCOEFF2(2, b),
                                                             GETCOEFF2(3, b));
            
            sp.fitsNubarRemapAVX2[i][j][k].c = _mm256_setr_pd(GETCOEFF2(0, c),
                                                             GETCOEFF2(1, c),
                                                             GETCOEFF2(2, c),
                                                             GETCOEFF2(3, c));

            sp.fitsNubarRemapAVX2[i][j][k].d = _mm256_setr_pd(GETCOEFF2(0, d),
                                                             GETCOEFF2(1, d),
                                                             GETCOEFF2(2, d),
                                                             GETCOEFF2(3, d));

          }
        }
      }


    } // end for(preds)


    // Predict something, anything, so that we can know what binning to use
    fBinning = fPredNom->Predict(fOscOrigin);
    fBinning.Clear();
  }

  //----------------------------------------------------------------------
  void PredictionInterp::SetOscSeed(osc::IOscCalculator* oscSeed){
    fOscOrigin = oscSeed->Copy();
    for(auto& it: fPreds) it.second.fits.clear();
    InitFits();
  }


  void PredictionInterp::DiscardSysts(std::vector<ISyst const *> const &systs) {

    size_t NPreds = fPreds.size();
    for (auto s : systs) {
      auto it = find_pred(s);
      if(it != fPreds.end()){
        fPreds.erase(it);
      }
    }
    HistCache::ClearCache();
  }

  std::vector<ISyst const *> PredictionInterp::GetAllSysts() const {
    std::vector<ISyst const *> allsysts;
    for (auto const &p : fPreds) {
      allsysts.push_back(p.first);
    }
    return allsysts;
  }


  //----------------------------------------------------------------------
  void PredictionInterp::
  ComponentDerivative(osc::IOscCalculator* calc,
                      Flavors::Flavors_t flav,
                      Current::Current_t curr,
                      Sign::Sign_t sign,
                      CoeffsType type,
                      const SystShifts& shift,
                      double pot,
                      std::unordered_map<const ISyst*, std::vector<double>>& dp) const
  {
    if(fSplitBySign && sign == Sign::kBoth){
      ComponentDerivative(calc, flav, curr, Sign::kNu,     type, shift, pot, dp);
      ComponentDerivative(calc, flav, curr, Sign::kAntiNu, type, shift, pot, dp);
      return;
    }

    const Spectrum snom = fPredNom->PredictComponent(calc, flav, curr, sign);
    TH1D* hnom = snom.ToTH1(snom.POT());
    const double* arr_nom = hnom->GetArray();

    const Spectrum base = PredictComponentSyst(calc, shift, flav, curr, sign);
    TH1D* h = base.ToTH1(pot);
    double* arr = h->GetArray();
    const unsigned int N = h->GetNbinsX()+2;


    // Should the interpolation use the nubar fits?
    const bool nubar = (fSplitBySign && sign == Sign::kAntiNu);

    for(auto& it: dp){
      const ISyst* syst = it.first;
      std::vector<double>& diff = it.second;
      if(diff.empty()) diff.resize(N);
      assert(diff.size() == N);
      assert(find_pred(syst) != fPreds.end());
      const ShiftedPreds& sp = get_pred(syst);

      double x = shift.GetShift(syst);

      int shiftBin = (x - sp.shifts[0])/sp.Stride();
      shiftBin = std::max(0, shiftBin);
      shiftBin = std::min(shiftBin, sp.nCoeffs-1);

      const Coeffs* fits = nubar ? &sp.fitsNubarRemap[type][shiftBin].front() : &sp.fitsRemap[type][shiftBin].front();

      x -= sp.shifts[shiftBin];

      const double x_cube = util::cube(x);
      const double x_sqr = util::sqr(x);

      for(unsigned int n = 0; n < N; ++n){
        // Wasn't corrected, so derivative is zero
        if(arr_nom[n] <= fMinMCStats) continue;

        // Uncomment to debug crashes in this function
        // assert(type < fits.size());
        // assert(n < sp.fits[type].size());
        // assert(shiftBin < int(sp.fits[type][n].size()));
        const Coeffs& f = fits[n];

        const double corr = f.a*x_cube + f.b*x_sqr + f.c*x + f.d;
        if(corr > 0){
          diff[n] += (3*f.a*x_sqr + 2*f.b*x + f.c)/corr*arr[n];
        }
      } // end for n
    } // end for syst

    HistCache::Delete(h);
    HistCache::Delete(hnom);
  }

  //----------------------------------------------------------------------
  void PredictionInterp::
  Derivative(osc::IOscCalculator* calc,
             const SystShifts& shift,
             double pot,
             std::unordered_map<const ISyst*, std::vector<double>>& dp) const
  {
    InitFits();

    // Check that we're able to handle all the systs we were passed
    for(auto& it: dp){
      if(find_pred(it.first) == fPreds.end()){
        std::cerr << "This PredictionInterp is not set up to handle the requested systematic: " << it.first->ShortName() << std::endl;
        abort();
      }
      it.second.clear();
    } // end for syst

    ComponentDerivative(calc, Flavors::kNuEToNuE,    Current::kCC, Sign::kBoth, kNueSurv,  shift, pot, dp);
    ComponentDerivative(calc, Flavors::kNuEToNuMu,   Current::kCC, Sign::kBoth, kOther,    shift, pot, dp);
    ComponentDerivative(calc, Flavors::kNuEToNuTau,  Current::kCC, Sign::kBoth, kOther,    shift, pot, dp);

    ComponentDerivative(calc, Flavors::kNuMuToNuE,   Current::kCC, Sign::kBoth, kNueApp,   shift, pot, dp);
    ComponentDerivative(calc, Flavors::kNuMuToNuMu,  Current::kCC, Sign::kBoth, kNumuSurv, shift, pot, dp);
    ComponentDerivative(calc, Flavors::kNuMuToNuTau, Current::kCC, Sign::kBoth, kOther,    shift, pot, dp);

    ComponentDerivative(calc, Flavors::kAll, Current::kNC, Sign::kBoth, kNC, shift, pot, dp);

    // Simpler (much slower) implementation in terms of finite differences for
    // test purposes
    /*
    const Spectrum p0 = PredictSyst(calc, shift);
    TH1D* h0 = p0.ToTH1(pot);

    const double dx = 1e-9;
    for(auto& it: dp){
      const ISyst* s =  it.first;
      std::vector<double>& v = it.second;
      SystShifts s2 = shift;
      s2.SetShift(s, s2.GetShift(s)+dx);

      const Spectrum p1 = PredictSyst(calc, s2);

      TH1D* h1 = p1.ToTH1(pot);

      v.resize(h1->GetNbinsX()+2);
      for(int i = 0; i < h1->GetNbinsX()+2; ++i){
        v[i] = (h1->GetBinContent(i) - h0->GetBinContent(i))/dx;
      }

      HistCache::Delete(h1);
    }

    HistCache::Delete(h0);
    */
  }

  //----------------------------------------------------------------------
  void PredictionInterp::SaveTo(TDirectory* dir) const
  {
    InitFits();

    TDirectory* tmp = gDirectory;

    dir->cd();
    TObjString("PredictionInterp").Write("type");


    fPredNom->SaveTo(dir->mkdir("pred_nom"));


    for(auto &it: fPreds){
      const ShiftedPreds& sp = it.second;

      for(unsigned int i = 0; i < sp.shifts.size(); ++i){
        if(!sp.preds[i]){
          std::cout << "Can't save a PredictionInterp after MinimizeMemory()" << std::endl;
          abort();
        }
        sp.preds[i]->SaveTo(dir->mkdir(TString::Format("pred_%s_%+d",
                                                       sp.systName.c_str(),
                                                       int(sp.shifts[i])).Data()));
      } // end for i
    } // end for it

    ana::SaveTo(*fOscOrigin, dir->mkdir("osc_origin"));

    if(!fPreds.empty()){
      TH1F hSystNames("syst_names", ";Syst names", fPreds.size(), 0, fPreds.size());
      int binIdx = 1;
      for(auto &it: fPreds){
        hSystNames.GetXaxis()->SetBinLabel(binIdx++, it.second.systName.c_str());
      }
      dir->cd();
      hSystNames.Write("syst_names");
    }

    TObjString split_sign = fSplitBySign ? "yes" : "no";
    dir->cd();
    split_sign.Write("split_sign");

    tmp->cd();
  }

  //----------------------------------------------------------------------
  std::unique_ptr<PredictionInterp> PredictionInterp::LoadFrom(TDirectory* dir)
  {
    TObjString* tag = (TObjString*)dir->Get("type");
    assert(tag);
    assert(tag->GetString() == "PredictionInterp" ||
           tag->GetString() == "PredictionInterp2"); // backwards compatibility

    std::unique_ptr<PredictionInterp> ret(new PredictionInterp);

    LoadFromBody(dir, ret.get());

    TObjString* split_sign = (TObjString*)dir->Get("split_sign");
    // Can be missing from old files
    ret->fSplitBySign = (split_sign && split_sign->String() == "yes");

    return ret;
  }

  //----------------------------------------------------------------------
  void PredictionInterp::LoadFromBody(TDirectory* dir, PredictionInterp* ret,
				      std::vector<const ISyst*> veto)
  {
    ret->fPredNom = ana::LoadFrom<IPrediction>(dir->GetDirectory("pred_nom"));

    TH1* hSystNames = (TH1*)dir->Get("syst_names");
    if(hSystNames){
      for(int systIdx = 0; systIdx < hSystNames->GetNbinsX(); ++systIdx){
        ShiftedPreds sp;
        sp.systName = hSystNames->GetXaxis()->GetBinLabel(systIdx+1);

        const ISyst* syst = SystRegistry::ShortNameToSyst(sp.systName);

        if(std::find(veto.begin(), veto.end(), syst) != veto.end()) continue;

        // Use whichever of these gives the most restrictive range
        const int x0 = std::max(-syst->PredInterpMaxNSigma(), int(trunc(syst->Min())));
        const int x1 = std::min(+syst->PredInterpMaxNSigma(), int(trunc(syst->Max())));

        for(int shift = x0; shift <= x1; ++shift){
          TDirectory* preddir = dir->GetDirectory(TString::Format("pred_%s_%+d", sp.systName.c_str(), shift).Data());
          if(!preddir){
            std::cout << "PredictionInterp: " << syst->ShortName() << " " << shift << " sigma " << " not found in " << dir->GetName() << std::endl;
            continue;
          }

          sp.shifts.push_back(shift);
          sp.preds.emplace_back(ana::LoadFrom<IPrediction>(preddir));
        } // end for shift

        ret->fPreds.emplace_back(syst, std::move(sp));
      } // end for systIdx
    } // end if hSystNames

    ret->fOscOrigin = ana::LoadFrom<osc::IOscCalculator>(dir->GetDirectory("osc_origin")).release();
  }

  //----------------------------------------------------------------------
  void PredictionInterp::MinimizeMemory()
  {
    DiscardSysts(GetAllSysts());
  }

  //----------------------------------------------------------------------
  void PredictionInterp::DebugPlot(const ISyst* syst,
                                   osc::IOscCalculator* calc,
                                   Flavors::Flavors_t flav,
                                   Current::Current_t curr,
                                   Sign::Sign_t sign) const
  {
    InitFits();

    auto it = find_pred(syst);
    if(it == fPreds.end()){
      std::cout << "PredictionInterp::DebugPlots(): "
                << syst->ShortName() << " not found" << std::endl;
      return;
    }

    std::unique_ptr<TH1> nom(fPredNom->PredictComponent(calc, flav, curr, sign).ToTH1(18e20));
    const int nbins = nom->GetNbinsX();

    TGraph* curves[nbins];
    TGraph* points[nbins];

    for(int i = 0; i <= 80; ++i){
      const double x = .1*i-4;
      const SystShifts ss(it->first, x);
      std::unique_ptr<TH1> h(PredictComponentSyst(calc, ss, flav, curr, sign).ToTH1(18e20));

      for(int bin = 0; bin < nbins; ++bin){
        if(i == 0){
          curves[bin] = new TGraph;
          points[bin] = new TGraph;
        }

        const double ratio = h->GetBinContent(bin+1)/nom->GetBinContent(bin+1);

        if(!std::isnan(ratio)) curves[bin]->SetPoint(curves[bin]->GetN(), x, ratio);
        else curves[bin]->SetPoint(curves[bin]->GetN(), x, 1);
      } // end for bin
    } // end for i (x)

    // As elswhere, to allow BirksC etc that need a different nominal to plot
    // right.
    IPrediction const* pNom = 0;
    for(unsigned int shiftIdx = 0; shiftIdx < it->second.shifts.size(); ++shiftIdx){
      if(it->second.shifts[shiftIdx] == 0) {pNom = it->second.preds[shiftIdx].get();}
    }
    assert(pNom);
    std::unique_ptr<TH1> hnom(pNom->PredictComponent(calc, flav, curr, sign).ToTH1(18e20));

    for(unsigned int shiftIdx = 0; shiftIdx < it->second.shifts.size(); ++shiftIdx){
      if(!it->second.preds[shiftIdx]) continue; // Probably MinimizeMemory()
      std::unique_ptr<TH1> h;
      h = std::move(std::unique_ptr<TH1>(it->second.preds[shiftIdx]->PredictComponent(calc, flav, curr, sign).ToTH1(18e20)));

      for(int bin = 0; bin < nbins; ++bin){
        const double ratio = h->GetBinContent(bin+1)/hnom->GetBinContent(bin+1);
        if(!std::isnan(ratio)) points[bin]->SetPoint(points[bin]->GetN(), it->second.shifts[shiftIdx], ratio);
        else points[bin]->SetPoint(points[bin]->GetN(), it->second.shifts[shiftIdx], 1);
      }
    } // end for shiftIdx


    int nx = int(sqrt(nbins));
    int ny = int(sqrt(nbins));
    if(nx*ny < nbins) ++nx;
    if(nx*ny < nbins) ++ny;

    TCanvas* c = new TCanvas;
    c->Divide(nx, ny);

    for(int bin = 0; bin < nbins; ++bin){
      c->cd(bin+1);
      (new TH2F("",
                TString::Format("%s %g < %s < %g;Shift;Ratio",
                                it->second.systName.c_str(),
                                nom->GetXaxis()->GetBinLowEdge(bin+1),
                                nom->GetXaxis()->GetTitle(),
                                nom->GetXaxis()->GetBinUpEdge(bin+1)),
                100, -4, +4, 100, .5, 1.5))->Draw();
      curves[bin]->Draw("l same");
      points[bin]->SetMarkerStyle(kFullDotMedium);
      points[bin]->Draw("p same");
    } // end for bin

    c->cd(0);
  }

  //----------------------------------------------------------------------
  void PredictionInterp::DebugPlots(osc::IOscCalculator* calc,
				    const std::string& savePattern,
				    Flavors::Flavors_t flav,
				    Current::Current_t curr,
				    Sign::Sign_t sign) const
  {
    for(auto& it: fPreds){
      DebugPlot(it.first, calc, flav, curr, sign);

      if(!savePattern.empty()){
	assert(savePattern.find("%s") != std::string::npos);
	gPad->Print(TString::Format(savePattern.c_str(), it.second.systName.c_str()).Data());
      }
    } // end for it
  }

  //----------------------------------------------------------------------
  void PredictionInterp::DebugPlotColz(const ISyst* syst,
                                       osc::IOscCalculator* calc,
                                       Flavors::Flavors_t flav,
                                       Current::Current_t curr,
                                       Sign::Sign_t sign) const
  {
    InitFits();

    std::unique_ptr<TH1> nom(fPredNom->PredictComponent(calc, flav, curr, sign).ToTH1(18e20));
    const int nbins = nom->GetNbinsX();

    TH2* h2 = new TH2F("", (syst->LatexName()+";;#sigma").c_str(),
                       nbins, nom->GetXaxis()->GetXmin(), nom->GetXaxis()->GetXmax(),
                       80, -4, +4);
    h2->GetXaxis()->SetTitle(nom->GetXaxis()->GetTitle());

    for(int i = 1; i <= 80; ++i){
      const double y = h2->GetYaxis()->GetBinCenter(i);
      const SystShifts ss(syst, y);
      std::unique_ptr<TH1> h(PredictComponentSyst(calc, ss, flav, curr, sign).ToTH1(18e20));

      for(int bin = 0; bin < nbins; ++bin){
        const double ratio = h->GetBinContent(bin+1)/nom->GetBinContent(bin+1);

        if(!isnan(ratio) && !isinf(ratio))
          h2->Fill(h2->GetXaxis()->GetBinCenter(bin), y, ratio);
      } // end for bin
    } // end for i (x)

    h2->Draw("colz");
    h2->SetMinimum(0.5);
    h2->SetMaximum(1.5);
  }

  //----------------------------------------------------------------------
  void PredictionInterp::DebugPlotsColz(osc::IOscCalculator* calc,
                                        const std::string& savePattern,
                                        Flavors::Flavors_t flav,
                                        Current::Current_t curr,
                                        Sign::Sign_t sign) const
  {
    InitFits();

    for(auto &it: fPreds){
      new TCanvas;
      DebugPlotColz(it.first, calc, flav, curr, sign);

      if(!savePattern.empty()){
	assert(savePattern.find("%s") != std::string::npos);
	gPad->Print(TString::Format(savePattern.c_str(), it.second.systName.c_str()).Data());
      }
    } // end for it
  }

} // namespace
