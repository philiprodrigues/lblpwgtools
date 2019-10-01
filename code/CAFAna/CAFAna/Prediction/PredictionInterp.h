#pragma once

#include "CAFAna/Prediction/IPrediction.h"
#include "CAFAna/Prediction/PredictionGenerator.h"
#include "CAFAna/Prediction/PredictionInterpKernel.h"

#include "CAFAna/Core/SpectrumLoader.h"
#include "CAFAna/Core/SystShifts.h"


#include "OscLib/func/IOscCalculator.h"
#include "Utilities/func/MathUtil.h"

#include <map>
#include <memory>

#include "TMD5.h"
#include "TH1D.h"

class TH1;

namespace ana
{
  class Loaders;

  /// Implements systematic errors by interpolation between shifted templates
  class PredictionInterp: public IPrediction
  {
  public:
    enum EMode_t{
      kCombineSigns, kSplitBySign
    };

    /// \param systs What systematics we should be capable of interpolating
    /// \param osc The oscillation point to expand around
    /// \param predGen Construct an IPrediction from the following
    ///                information.
    /// \param loaders The loaders to be passed on to the underlying prediction
    /// \param shiftMC Underlying shift. Use with care. Mostly for
    ///                PredictionNumuFAHadE. Should not contain any of of
    ///                \a systs
    /// \param mode    In FHC the wrong-sign has bad stats and probably the
    ///                fits can't be split out reasonably. For RHC it's
    ///                important not to conflate them.
    PredictionInterp(std::vector<const ISyst*> systs,
                     osc::IOscCalculator* osc,
                     const IPredictionGenerator& predGen,
                     Loaders& loaders,
                     const SystShifts& shiftMC = kNoShift,
                     EMode_t mode = kCombineSigns);

    virtual ~PredictionInterp();

    virtual void Derivative(osc::IOscCalculator* calc,
                            const SystShifts& shift,
                            double pot,
                            std::unordered_map<const ISyst*, std::vector<double>>& dp) const override;

    virtual void SaveTo(TDirectory* dir) const override;
    static std::unique_ptr<PredictionInterp> LoadFrom(TDirectory* dir);

    /// After calling this DebugPlots won't work fully and SaveTo won't work at
    /// all.
    void MinimizeMemory();

    void DebugPlot(const ISyst* syst,
                   osc::IOscCalculator* calc,
                   Flavors::Flavors_t flav = Flavors::kAll,
                   Current::Current_t curr = Current::kBoth,
                   Sign::Sign_t sign = Sign::kBoth) const;

    // If \a savePattern is not empty, print each pad. Must contain a "%s" to
    // contain the name of the systematic.
    void DebugPlots(osc::IOscCalculator* calc,
		    const std::string& savePattern = "",
		    Flavors::Flavors_t flav = Flavors::kAll,
		    Current::Current_t curr = Current::kBoth,
		    Sign::Sign_t sign = Sign::kBoth) const;

    void SetOscSeed(osc::IOscCalculator* oscSeed);

    void DebugPlotColz(const ISyst* syst,
                       osc::IOscCalculator* calc,
                       Flavors::Flavors_t flav = Flavors::kAll,
                       Current::Current_t curr = Current::kBoth,
                       Sign::Sign_t sign = Sign::kBoth) const;

    void DebugPlotsColz(osc::IOscCalculator* calc,
                        const std::string& savePattern = "",
                        Flavors::Flavors_t flav = Flavors::kAll,
                        Current::Current_t curr = Current::kBoth,
                        Sign::Sign_t sign = Sign::kBoth) const;

    bool SplitBySign() const {return fSplitBySign;}
    enum CoeffsType{
      kNueApp, kNueSurv, kNumuSurv, kNC,
      kOther, ///< Taus, numu appearance
      kNCoeffTypes
    };

    PredictionInterp() : fBinning(0, {}, {}, 0, 0) {
      if(getenv("CAFANA_PRED_MINMCSTATS")){
        fMinMCStats = atoi(getenv("CAFANA_PRED_MINMCSTATS"));
      } else {
        fMinMCStats = 50;
      }
    }

    static void LoadFromBody(TDirectory* dir, PredictionInterp* ret,
			     std::vector<const ISyst*> veto = {});

    typedef ana::PredIntKern::Coeffs Coeffs;
    typedef ana::PredIntKern::CoeffsAVX2 CoeffsAVX2;

    /// Find coefficients describing this set of shifts
    std::vector<std::vector<Coeffs>>
    FitRatios(const std::vector<double>& shifts,
              const std::vector<std::unique_ptr<TH1>>& ratios) const;

    /// Find coefficients describing the ratios from this component
    std::vector<std::vector<Coeffs>>
    FitComponent(const std::vector<double>& shifts,
                 const std::vector<std::unique_ptr<IPrediction>>& preds,
                 Flavors::Flavors_t flav,
                 Current::Current_t curr,
                 Sign::Sign_t sign,
                 const std::string& systName) const;

  //----------------------------------------------------------------------
  virtual inline Spectrum Predict(osc::IOscCalculator* calc) const override
  {
    return fPredNom->Predict(calc);
  }

  //----------------------------------------------------------------------
  virtual inline Spectrum PredictComponent(osc::IOscCalculator* calc,
                                              Flavors::Flavors_t flav,
                                              Current::Current_t curr,
                                              Sign::Sign_t sign) const override
  {
    return fPredNom->PredictComponent(calc, flav, curr, sign);
  }

  //----------------------------------------------------------------------
  virtual inline Spectrum PredictSyst(osc::IOscCalculator* calc,
                                         const SystShifts& shift) const override
  {
    InitFits();

    return PredictComponentSyst(calc, shift,
                                Flavors::kAll,
                                Current::kBoth,
                                Sign::kBoth);
  }

    std::unique_ptr<IPrediction> fPredNom; ///< The nominal prediction

    struct ShiftedPreds
    {
      double Stride() const {return shifts.size() > 1 ? shifts[1]-shifts[0] : 1;}

      std::string systName; ///< What systematic we're interpolating
      std::vector<double> shifts; ///< Shift values sampled
      std::vector<std::unique_ptr<IPrediction>> preds;

      int nCoeffs; // Faster than calling size()

      /// Indices: [type][histogram bin][shift bin]
      std::vector<std::vector<std::vector<Coeffs>>> fits;
      /// Will be filled if signs are separated, otherwise not
      std::vector<std::vector<std::vector<Coeffs>>> fitsNubar;

      // Same info as above but with more-easily-iterable index order
      // [type][shift bin][histogram bin]. TODO this is ugly
      std::vector<std::vector<std::vector<Coeffs>>> fitsRemap;
      std::vector<std::vector<std::vector<Coeffs>>> fitsNubarRemap;

      // Same info as above but with CoeffsAVX2. Index order
      // [type][shift bin][histogram bin]. TODO this is even uglier
      std::vector<std::vector<std::vector<CoeffsAVX2>>> fitsRemapAVX2;
      std::vector<std::vector<std::vector<CoeffsAVX2>>> fitsNubarRemapAVX2;

      ShiftedPreds() {}
      ShiftedPreds(ShiftedPreds &&other)
          : systName(std::move(other.systName)),
            shifts(std::move(other.shifts)), preds(std::move(other.preds)),
            nCoeffs(other.nCoeffs), fits(std::move(other.fits)),
            fitsNubar(std::move(other.fitsNubar)),
            fitsRemap(std::move(other.fitsRemap)),
            fitsNubarRemap(std::move(other.fitsNubarRemap)) {}

      ShiftedPreds &operator=(ShiftedPreds &&other) {
        systName = std::move(other.systName);
        shifts = std::move(other.shifts);
        preds = std::move(other.preds);
        nCoeffs = other.nCoeffs;
        fits = std::move(other.fits);
        fitsNubar = std::move(other.fitsNubar);
        fitsRemap = std::move(other.fitsRemap);
        fitsNubarRemap = std::move(other.fitsNubarRemap);
        return *this;
      }

      void Dump(){
        std::cout << "[INFO]: " << systName << ", with " << preds.size() << " preds." << std::endl;
      }
    };

    //Memory saving feature, if you know you wont need any systs that were loaded in, can discard them.
    void DiscardSysts(std::vector<ISyst const *>const &);
    //Get all known about systs
    std::vector<ISyst const *> GetAllSysts() const;

    //----------------------------------------------------------------------
    virtual inline Spectrum PredictComponentSyst(osc::IOscCalculator* calc,
                                                    const SystShifts& shift,
                                                    Flavors::Flavors_t flav,
                                                    Current::Current_t curr,
                                                    Sign::Sign_t sign) const override
      {
        InitFits();
        Spectrum& ret = fBinning;
        ret.Clear();
        fShiftBins.clear();

        if(ret.POT()==0) ret.OverridePOT(1e24);

        // Check that we're able to handle all the systs we were passed
        for(const ISyst* syst: shift.ActiveSysts()){
          if(find_pred(syst) == fPreds.end()){
            std::cerr << "This PredictionInterp is not set up to handle the requested systematic: " << syst->ShortName() << std::endl;
            abort();
          }
        } // end for syst


        const TMD5* hash = calc ? calc->GetParamsHash() : 0;

        // Should the interpolation use the nubar fits?
        const bool nubar = (fSplitBySign && sign == Sign::kAntiNu);

        if(curr & Current::kCC){
          // if(flav & Flavors::kNuMuToNuE)   PrefetchCoeffs(kNueApp,   nubar, shift);

          if(flav & Flavors::kNuEToNuE)    ret += ShiftedComponent(calc, hash, shift, Flavors::kNuEToNuE,    Current::kCC, sign, kNueSurv);

          // if(flav & Flavors::kNuMuToNuMu)  PrefetchCoeffs(kNumuSurv, nubar, shift);

          if(flav & Flavors::kNuEToNuMu)   ret += ShiftedComponent(calc, hash, shift, Flavors::kNuEToNuMu,   Current::kCC, sign, kOther  );

          // if(flav & Flavors::kNuMuToNuTau) PrefetchCoeffs(kOther,    nubar, shift);

          if(flav & Flavors::kNuEToNuTau)  ret += ShiftedComponent(calc, hash, shift, Flavors::kNuEToNuTau,  Current::kCC, sign, kOther  );

          if(flav & Flavors::kNuMuToNuE)   ret += ShiftedComponent(calc, hash, shift, Flavors::kNuMuToNuE,   Current::kCC, sign, kNueApp  );
          if(flav & Flavors::kNuMuToNuMu)  ret += ShiftedComponent(calc, hash, shift, Flavors::kNuMuToNuMu,  Current::kCC, sign, kNumuSurv);
          if(flav & Flavors::kNuMuToNuTau) ret += ShiftedComponent(calc, hash, shift, Flavors::kNuMuToNuTau, Current::kCC, sign, kOther   );
        }
        if(curr & Current::kNC){
          assert(flav == Flavors::kAll); // Don't know how to calculate anything else

          ret += ShiftedComponent(calc, hash, shift, Flavors::kAll, Current::kNC, sign, kNC);
        }

        delete hash;

        return ret;
      }

  //----------------------------------------------------------------------
  virtual inline Spectrum 
  ShiftedComponent(osc::IOscCalculator* calc,
                   const TMD5* hash,
                   const SystShifts& shift,
                   Flavors::Flavors_t flav,
                   Current::Current_t curr,
                   Sign::Sign_t sign,
                   CoeffsType type) const __attribute__((always_inline))
  {
    if(fSplitBySign && sign == Sign::kBoth){
      return (ShiftedComponent(calc, hash, shift, flav, curr, Sign::kAntiNu, type)+
              ShiftedComponent(calc, hash, shift, flav, curr, Sign::kNu,     type));
    }

    // Must be the base case of the recursion to use the cache. Otherwise we
    // can cache systematically shifted versions of our children, which is
    // wrong. Also, some calculators won't hash themselves.
    const bool canCache = (hash != 0);

    const Key_t key = {flav, curr, sign};
    auto it = fNomCache.find(key);

    // Should the interpolation use the nubar fits?
    const bool nubar = (fSplitBySign && sign == Sign::kAntiNu);

    // We have the nominal for this exact combination of flav, curr, sign, calc
    // stored.  Shift it and return.
    if(canCache && it != fNomCache.end() && it->second.hash == *hash){
      return ShiftSpectrum(it->second.nom, type, nubar, shift);
    }

    // We need to compute the nominal again for whatever reason
    const Spectrum nom = fPredNom->PredictComponent(calc, flav, curr, sign);

    if(canCache){
      // Insert into the cache if not already there, or update if there but
      // with old oscillation parameters.
      if(it == fNomCache.end())
        fNomCache.emplace(key, Val_t({*hash, nom}));
      else
        it->second = {*hash, nom};
    }

    return ShiftSpectrum(nom, type, nubar, shift);
  }

    inline void PrefetchCoeffs(CoeffsType type,
                               bool nubar,
                               const SystShifts &shift) const
      {
        if(fShiftBins.empty()) return;

        const size_t NPreds = fPreds.size();

        for (size_t p_it = 0; p_it < NPreds; ++p_it) {
          // const ISyst *syst = fPreds[p_it].first;
          const ShiftedPreds &sp = fPreds[p_it].second;

          int shiftBin = fShiftBins[p_it];

          const Coeffs *fits = nubar ? &sp.fitsNubarRemap[type][shiftBin].front()
            : &sp.fitsRemap[type][shiftBin].front();
          for(unsigned int n=0; n<fNBins; ++n) __builtin_prefetch(fits+n);
        } // end for syst

      }

  //----------------------------------------------------------------------
  virtual inline Spectrum ShiftSpectrum(const Spectrum &s, CoeffsType type,
                                          bool nubar,
                                          const SystShifts &shift) const __attribute__((always_inline)) {

    static int ncall=0;
    ++ncall;

    if (nubar)
      assert(fSplitBySign);

    // TODO histogram operations could be too slow
    TH1D *h = s.ToTH1(s.POT());

    const unsigned int N = h->GetNbinsX() + 2;
    fNBins=N;

#ifdef USE_PREDINTERP_OMP
    double corr[4][N];
    for (unsigned int i = 0; i < 4; ++i) {
      for (unsigned int j = 0; j < N; ++j) {
        corr[i][j] = 1;
      };
    }
#else
    alignas(64) double corr[N+4];
    alignas(64) double corrAVX2[N+4];
    for (unsigned int i = 0; i < N; ++i) {
      corr[i] = 1;
      corrAVX2[i] = 1;
    }
#endif

    size_t NPreds = fPreds.size();

    if(fShiftBins.empty()){
      fShiftBins.resize(NPreds);
      for (size_t p_it = 0; p_it < NPreds; ++p_it) {
        const ISyst *syst = fPreds[p_it].first;
        const ShiftedPreds &sp = fPreds[p_it].second;
        
        double x = shift.GetShift(syst);
        
        if (x == 0)
          continue;
        
        int shiftBin = (x - sp.shifts[0]) / sp.Stride();
        shiftBin = std::max(0, shiftBin);
        shiftBin = std::min(shiftBin, sp.nCoeffs - 1);
        fShiftBins[p_it]=shiftBin;
      }
    }

    std::vector<CoeffsAVX2> default_coeffs(N/4+1, CoeffsAVX2(_mm256_setzero_pd(),
                                                             _mm256_setzero_pd(),
                                                             _mm256_setzero_pd(),
                                                             _mm256_set1_pd(1)));
    const CoeffsAVX2* fitss[NPreds];
    double xs[NPreds];

    #pragma omp parallel for
    for (size_t p_it = 0; p_it < NPreds; ++p_it) {
      const ISyst *syst = fPreds[p_it].first;
      const ShiftedPreds &sp = fPreds[p_it].second;

      double x = shift.GetShift(syst);

      if (x == 0){
        fitss[p_it]=&default_coeffs.front();
        xs[p_it]=0;
        continue;
      }

      int shiftBin = fShiftBins[p_it];

      const Coeffs *fits = nubar ? &sp.fitsNubarRemap[type][shiftBin].front()
                                 : &sp.fitsRemap[type][shiftBin].front();

      const CoeffsAVX2 *fitsAVX2 = nubar ? &sp.fitsNubarRemapAVX2[type][shiftBin].front()
                                     : &sp.fitsRemapAVX2[type][shiftBin].front();
      fitss[p_it]=fitsAVX2;
      x -= sp.shifts[shiftBin];

      xs[p_it]=x;
    } // end for syst

    const size_t PREFETCH_DISTANCE=2;

    for (size_t p_it = 0; p_it < NPreds; ++p_it) {
      // const double x=xs[p_it];
      // const double x_cube = util::cube(x);
      // const double x_sqr = util::sqr(x);
      
#ifdef USE_PREDINTERP_OMP
      ShiftSpectrumKernel(fits, N, x, x_sqr, x_cube,
                          corr[omp_get_thread_num()]);
#else
      // broadcast x3, x2, x into __m256d registers
      __m256d x=_mm256_set1_pd(xs[p_it]);
      __m256d x2=_mm256_mul_pd(x,x);
      __m256d x3=_mm256_mul_pd(x2,x);
      for(unsigned int n = 0; n < N/4+1; ++n){
        // out  = f.a*x3
        // out += f.b*x2
        // out += f.c*x
        // out += f.d
        // out *= corr[n]
        // store corr
        _mm_prefetch(fitss[p_it+PREFETCH_DISTANCE]+n, _MM_HINT_T1);
        const CoeffsAVX2& f = fitss[p_it][n];
        __m256d out=_mm256_mul_pd(f.a, x3);

        out=_mm256_add_pd(out, _mm256_mul_pd(f.b, x2));
        out=_mm256_add_pd(out, _mm256_mul_pd(f.c, x));
        out=_mm256_add_pd(out, f.d);

        out=_mm256_mul_pd(out, _mm256_loadu_pd(corrAVX2+4*n));
        _mm256_storeu_pd(corrAVX2+4*n, out);
        // corr[n] *= f.a*x3 + f.b*x2 + f.c*x + f.d;
      } // end for n

      // ShiftSpectrumKernelAVX2(fitss[p_it], N/4+1, x, x_sqr, x_cube, corrAVX2);
      
#endif
    } // end for syst

#ifdef USE_PREDINTERP_OMP
    for (unsigned int i = 1; i < 4; ++i) {
      for (unsigned int j = 0; j < N; ++j) {
        corr[0][j] *= corr[i][j];
      };
    }
#endif

    double *arr = h->GetArray();
    for (unsigned int n = 0; n < N; ++n) {
      if (arr[n] > fMinMCStats) {
#ifdef USE_PREDINTERP_OMP
        arr[n] *= std::max(corr[0][n], 0.);
#else
        arr[n] *= std::max(corrAVX2[n], 0.);
#endif
      }
    }

    return Spectrum(std::unique_ptr<TH1D>(h), s.GetLabels(), s.GetBinnings(),
                    s.POT(), s.Livetime());
  }

  protected:
    using PredMappedType = std::pair<const ISyst *, ShiftedPreds>;
    mutable std::vector<PredMappedType> fPreds;
    std::vector<PredMappedType>::iterator find_pred(const ISyst *s) const {
      return std::find_if(
          fPreds.begin(), fPreds.end(),
          [=](PredMappedType const &p) { return bool(s == p.first); });
    }
    ShiftedPreds const &get_pred(const ISyst *s) const {
      return std::find_if(fPreds.begin(), fPreds.end(),
                       [=](PredMappedType const &p) {
                         return bool(s == p.first);
                       })
          ->second;
    }

    /// The oscillation values we assume when evaluating the coefficients
    osc::IOscCalculator* fOscOrigin;

    mutable Spectrum fBinning; ///< Dummy spectrum to provide binning

    struct Key_t
    {
      Flavors::Flavors_t flav;
      Current::Current_t curr;
      Sign::Sign_t sign;
      bool operator<(const Key_t& rhs) const
      {
        return (std::make_tuple(flav, curr, sign) <
                std::make_tuple(rhs.flav, rhs.curr, rhs.sign));
      }
    };
    struct Val_t
    {
      TMD5 hash;
      Spectrum nom;
    };
    mutable std::map<Key_t, Val_t> fNomCache;

    bool fSplitBySign;

    // Don't apply systs to bins with fewer than this many MC stats
    double fMinMCStats;

    mutable std::vector<int> fShiftBins;
    mutable unsigned int fNBins{0};

    void InitFits() const;

    void InitFitsHelper(ShiftedPreds& sp,
                        std::vector<std::vector<std::vector<Coeffs>>>& fits,
                        Sign::Sign_t sign) const;

     /// Helper for \ref Derivative
    void ComponentDerivative(osc::IOscCalculator* calc,
                             Flavors::Flavors_t flav,
                             Current::Current_t curr,
                             Sign::Sign_t sign,
                             CoeffsType type,
                             const SystShifts& shift,
                             double pot,
                             std::unordered_map<const ISyst*, std::vector<double>>& dp) const;
  };

}
