#include "CAFAna/Prediction/PredictionExtrap.h"

#include "CAFAna/Prediction/PredictionGenerator.h"

namespace ana
{
  class Loaders;

  class DUNERunPOTSpectrumLoader;

  /// Prediction that just uses FD MC, with no extrapolation
  class PredictionOffExtrap: public PredictionExtrap
  {
  public:
    PredictionOffExtrap(PredictionExtrap* pred);

    // This is the DUNE constructor
    PredictionOffExtrap(SpectrumLoaderBase& loaderNonswap,
                       SpectrumLoaderBase& loaderNue,
                       SpectrumLoaderBase& loaderNuTau,
                       const std::string& label,
                       const Binning& bins,
                       const Var& var,
                       const Cut& cut,             
    		       std::map <float, std::map <float, std::map<float, float>>> map ,          
                       const SystShifts& shift = kNoShift,
		       int offLocation = 0, 
                       const Var& wei = kUnweighted);

    PredictionOffExtrap(SpectrumLoaderBase& loaderNonswap,
                       SpectrumLoaderBase& loaderNue,
                       SpectrumLoaderBase& loaderNuTau,
                       const HistAxis& axis,
                       const Cut& cut,
                       std::map <float, std::map <float, std::map<float, float>>> map ,
                       const SystShifts& shift = kNoShift,
                       int offLocation = 0,
                       const Var& wei = kUnweighted);

    PredictionOffExtrap(SpectrumLoaderBase& loaderNonswap,
                       SpectrumLoaderBase& loaderNue,
                       SpectrumLoaderBase& loaderNuTau,
                       const std::string& label,
                       const Binning& bins,
                       const Var& var, int offLocation,
                       const Cut& cut,
                       std::map <float, std::map <float, std::map<float, float>>> map ,
                       const SystShifts& shift = kNoShift,
                       const Var& wei = kUnweighted);

    PredictionOffExtrap(SpectrumLoaderBase& loaderNonswap,
                       SpectrumLoaderBase& loaderNue,
                       SpectrumLoaderBase& loaderNuTau,
		       const HistAxis& axis,
		       const Cut& cut,
                       const SystShifts& shift = kNoShift,
                       const Var& wei = kUnweighted);

    PredictionOffExtrap(Loaders& loaders,
                       const std::string& label,
                       const Binning& bins,
                       const Var& var,
                       const Cut& cut,
                       const SystShifts& shift = kNoShift,
                       const Var& wei = kUnweighted);

    PredictionOffExtrap(Loaders& loaders,
                       const HistAxis& axis,
                       const Cut& cut,
                       const SystShifts& shift = kNoShift,
                       const Var& wei = kUnweighted);

    virtual ~PredictionOffExtrap();

    static std::unique_ptr<PredictionOffExtrap> LoadFrom(TDirectory* dir);
    virtual void SaveTo(TDirectory* dir) const override;

    std::map <float, std::map <float, std::map<float, float>>> fExtrapMap;

  };

  class OffExtrapPredictionGenerator: public IPredictionGenerator
  {
  public:
    OffExtrapPredictionGenerator(HistAxis axis, Cut cut)
      : fAxis(axis), fCut(cut)
    {
    }

    virtual std::unique_ptr<IPrediction>
    Generate(Loaders& loaders, const SystShifts& shiftMC = kNoShift) const override
    {
      return std::unique_ptr<IPrediction>(
                                          new PredictionOffExtrap(loaders, fAxis, fCut, shiftMC));
    }

  protected:
    HistAxis fAxis;
    Cut fCut;
    //std::map <float, std::map <float, std::map<float, float>>> fExtrapMap;
  };

  class DUNEOffExtrapPredictionGenerator: public IPredictionGenerator
  {
  public:

    DUNEOffExtrapPredictionGenerator(SpectrumLoaderBase& loaderBeam,
                                    SpectrumLoaderBase& loaderNue,
                                    SpectrumLoaderBase& loaderNuTau,
                                    SpectrumLoaderBase& loaderNC,
                                    HistAxis axis,
                                    Cut cut,
				    std::map <float, std::map <float, std::map<float, float>>> map,
				    int offLocation, Var wei)
      : fLoaderBeam(loaderBeam), fLoaderNue(loaderNue),
        fLoaderNuTau(loaderNuTau), fLoaderNC(loaderNC),
        fAxis(axis), fCut(cut), fMap(map), fOffLocation(offLocation), fWei(wei)
    {
    }
    virtual std::unique_ptr<IPrediction>
    Generate(Loaders& loaders, const SystShifts& shiftMC = kNoShift) const override
    {
      return std::unique_ptr<IPrediction>(
        new PredictionOffExtrap(fLoaderBeam, fLoaderNue, fLoaderNuTau, 
                               fAxis, fCut, fMap, shiftMC, fOffLocation, fWei));
    }

  protected:
    SpectrumLoaderBase& fLoaderBeam;
    SpectrumLoaderBase& fLoaderNue;
    SpectrumLoaderBase& fLoaderNuTau;
    SpectrumLoaderBase& fLoaderNC;
    HistAxis fAxis;
    Cut fCut;
    std::map <float, std::map <float, std::map<float, float>>> fMap;    
    int fOffLocation;
    Var fWei;    
  };
}

