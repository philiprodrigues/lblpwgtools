#include "CAFAna/Prediction/PredictionGenerator.h"
#include "CAFAna/Prediction/PredictionNoExtrap.h"
#include "CAFAna/Prediction/PredictionOffExtrap.h"

namespace ana
{

  //--------------------------------------------------------------------------

  NoExtrapGenerator::NoExtrapGenerator(
    const HistAxis axis,
    //const Binning bins,
    const Cut cut,
    const Var wei
  ) : fAxis(axis), fCut(cut), fWei(wei) {}

  std::unique_ptr<IPrediction> NoExtrapGenerator::Generate(
							   Loaders& loaders,
							   const SystShifts& shiftMC
							   ) const {
    std::cout<<"in NoExtrapGenerator::Generate "<<std::endl;
    return std::unique_ptr<IPrediction>( new PredictionNoExtrap(
								loaders, fAxis, fCut, shiftMC, fWei ) );
  }


  OffExtrapGenerator::OffExtrapGenerator(
    const HistAxis axis,
    const Cut cut,
    std::map <float, std::map <float, std::map<float, float>>> map,
    int offLocation, 
    const Var wei
  ) : fAxis(axis), fCut(cut), fMap(map), fOffLocation(offLocation), fWei(wei) {}

  std::unique_ptr<IPrediction> OffExtrapGenerator::Generate(
                                                           Loaders& loaders,
                                                           const SystShifts& shiftMC
                                                           ) const {
    std::cout<<"in OffExtrapGenerator::Generate "<<std::endl;
    return std::unique_ptr<IPrediction>( new PredictionOffExtrap(
                                                                loaders, fAxis, fCut, shiftMC, fWei ) );

  }

  std::unique_ptr<IPrediction> OffExtrapGenerator::Generate(
                                                           SpectrumLoaderBase& loaderNonswap, SpectrumLoaderBase& loaderNue, SpectrumLoaderBase& loaderNuTau,
                                                           const SystShifts& shiftMC
                                                           ) const {
    std::cout<<"in OffExtrapGenerator::Generate "<<std::endl;
    return std::unique_ptr<IPrediction>( new PredictionOffExtrap(
                                                                loaderNonswap, loaderNue, loaderNuTau, fAxis, fCut, fMap, shiftMC, fOffLocation, fWei ) );
  
  }







}
