#include "CAFAna/Prediction/PredictionOffExtrap.h"

#include "CAFAna/Extrap/IExtrap.h"
#include "CAFAna/Core/LoadFromFile.h"

#include "CAFAna/Core/Loaders.h"
#include "CAFAna/Extrap/TrivialExtrap.h"

#include "TDirectory.h"
#include "TObjString.h"


namespace ana
{
  //----------------------------------------------------------------------
  PredictionOffExtrap::PredictionOffExtrap(SpectrumLoaderBase& loaderNonswap,
                                         SpectrumLoaderBase& loaderNue,
                                         SpectrumLoaderBase& loaderNuTau,
                                         const std::string& label,
                                         const Binning& bins,
                                         const Var& var,
                                         const Cut& cut, 
					 std::map <float, std::map <float, std::map<float, float>>> map,
                                         const SystShifts& shift, int offLocation,
                                         const Var& wei)
    : PredictionExtrap(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau,
                                         label, bins, var, cut, shift, offLocation, wei), offLocation, map), fExtrapMap(map)
  {
    std::cout<<"in PredictionOffExtrap (off after)"<<std::endl; 

    //fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, label, bins, var, cut && SIMPLEVAR(dune.det_x) == 4 && SIMPLEVAR(dune.vtx_x)>-25 && SIMPLEVAR(dune.vtx_x)<25 , shift, offLocation, wei));
    fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, label, bins, var, cut && SIMPLEVAR(dune.det_x) == 4 && SIMPLEVAR(dune.vtx_x)>0 && SIMPLEVAR(dune.vtx_x)<50, shift, offLocation, wei));
    fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, label, bins, var, cut && SIMPLEVAR(dune.det_x) == 4 && SIMPLEVAR(dune.vtx_x)>50 && SIMPLEVAR(dune.vtx_x)<100, shift, offLocation, wei));
    fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, label, bins, var, cut && SIMPLEVAR(dune.det_x) == 4 && SIMPLEVAR(dune.vtx_x)>100 && SIMPLEVAR(dune.vtx_x)<150, shift, offLocation, wei));
    fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, label, bins, var, cut && SIMPLEVAR(dune.det_x) == 4 && SIMPLEVAR(dune.vtx_x)>150 && SIMPLEVAR(dune.vtx_x)<200, shift, offLocation, wei));


    for(Int_t i =1 ; i< 9; i++){
      fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, label, bins, var, cut && SIMPLEVAR(dune.det_x) == (i+1)*4 && SIMPLEVAR(dune.vtx_x)>-200 && SIMPLEVAR(dune.vtx_x)<-150, shift, offLocation, wei));
      //PredictionExtrap(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau,label, bins, var, cut && SIMPLEVAR(dune.det_x) == (i+1)*4 && SIMPLEVAR(dune.vtx_x)>0 && SIMPLEVAR(dune.vtx_x)<200, shift, offLocation, wei), offLocation, map);
      fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, label, bins, var, cut && SIMPLEVAR(dune.det_x) == (i+1)*4 && SIMPLEVAR(dune.vtx_x)>-150 && SIMPLEVAR(dune.vtx_x)<-100, shift, offLocation, wei));
      fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, label, bins, var, cut && SIMPLEVAR(dune.det_x) == (i+1)*4 && SIMPLEVAR(dune.vtx_x)>-100 && SIMPLEVAR(dune.vtx_x)<-50, shift, offLocation, wei));
      fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, label, bins, var, cut && SIMPLEVAR(dune.det_x) == (i+1)*4 && SIMPLEVAR(dune.vtx_x)>-50 && SIMPLEVAR(dune.vtx_x)<0, shift, offLocation, wei));
      fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, label, bins, var, cut && SIMPLEVAR(dune.det_x) == (i+1)*4 && SIMPLEVAR(dune.vtx_x)>0 && SIMPLEVAR(dune.vtx_x)<50, shift, offLocation, wei));
      fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, label, bins, var, cut && SIMPLEVAR(dune.det_x) == (i+1)*4 && SIMPLEVAR(dune.vtx_x)>50 && SIMPLEVAR(dune.vtx_x)<100, shift, offLocation, wei));
      fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, label, bins, var, cut && SIMPLEVAR(dune.det_x) == (i+1)*4 && SIMPLEVAR(dune.vtx_x)>100 && SIMPLEVAR(dune.vtx_x)<150, shift, offLocation, wei));
      fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, label, bins, var, cut && SIMPLEVAR(dune.det_x) == (i+1)*4 && SIMPLEVAR(dune.vtx_x)>150 && SIMPLEVAR(dune.vtx_x)<200, shift, offLocation, wei));

      //PredictionExtrap(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau,label, bins, var, cut && SIMPLEVAR(dune.det_x) == (i+2)*4 && SIMPLEVAR(dune.vtx_x)<0 && SIMPLEVAR(dune.vtx_x)>-200, shift, offLocation, wei), offLocation, map);
    }
    std::cout<<"wait, size of fvExtrp is "<<fvExtrap.size()<<std::endl;
        
  }

  //----------------------------------------------------------------------

  PredictionOffExtrap::PredictionOffExtrap(SpectrumLoaderBase& loaderNonswap,
                                         SpectrumLoaderBase& loaderNue,
                                         SpectrumLoaderBase& loaderNuTau,
                                         const std::string& label,
                                         const Binning& bins,
                                         const Var& var, int offLocation,
                                         const Cut& cut,
                                         std::map <float, std::map <float, std::map<float, float>>> map,
                                         const SystShifts& shift,
                                         const Var& wei)
    : PredictionExtrap(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau,
                                         label, bins, var,  cut, shift, wei), 100, map), fExtrapMap(map)
  {
    std::cout<<"in PredictionOffExtrap (off before)"<<std::endl;

    for(Int_t i =0 ; i< 10; i++){
      fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau,
                                           label, bins, var, cut /* SIMPLEVAR(dune.det_x) == 0 */ , shift, offLocation, wei));
    }

  }

  //----------------------------------------------------------------------
  PredictionOffExtrap::PredictionOffExtrap(SpectrumLoaderBase& loaderNonswap,
                                         SpectrumLoaderBase& loaderNue,
                                         SpectrumLoaderBase& loaderNuTau,
					 const HistAxis& axis,
                                         const Cut& cut,
                                         const SystShifts& shift,
                                         const Var& wei)
    : PredictionExtrap(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau,
                                         axis, cut, shift, 0, wei), 0)
  {
  }

  PredictionOffExtrap::PredictionOffExtrap(SpectrumLoaderBase& loaderNonswap,
                                         SpectrumLoaderBase& loaderNue,
                                         SpectrumLoaderBase& loaderNuTau,
                                         const HistAxis& axis,
                                         const Cut& cut,
                                         std::map <float, std::map <float, std::map<float, float>>> map,
                                         const SystShifts& shift, 
					 int offLocation,
                                         const Var& wei) 
    : PredictionExtrap(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau,
                                         axis, cut, shift, offLocation, wei), offLocation, map), fExtrapMap(map)
  {
    std::cout<<"in PredictionOffExtrap (off after)"<<std::endl;

    fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, axis, cut && SIMPLEVAR(dune.det_x) == 4 && SIMPLEVAR(dune.vtx_x)>0 && SIMPLEVAR(dune.vtx_x)<50, shift, offLocation, wei));
    fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, axis, cut && SIMPLEVAR(dune.det_x) == 4 && SIMPLEVAR(dune.vtx_x)>50 && SIMPLEVAR(dune.vtx_x)<100, shift, offLocation, wei));
    fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, axis, cut && SIMPLEVAR(dune.det_x) == 4 && SIMPLEVAR(dune.vtx_x)>100 && SIMPLEVAR(dune.vtx_x)<150, shift, offLocation, wei));
    fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, axis, cut && SIMPLEVAR(dune.det_x) == 4 && SIMPLEVAR(dune.vtx_x)>150 && SIMPLEVAR(dune.vtx_x)<200, shift, offLocation, wei));

    for(Int_t i =1 ; i< 9; i++){
      fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, axis, cut && SIMPLEVAR(dune.det_x) == (i+1)*4 && SIMPLEVAR(dune.vtx_x)>-200 && SIMPLEVAR(dune.vtx_x)<-150, shift, offLocation, wei));
      fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, axis, cut && SIMPLEVAR(dune.det_x) == (i+1)*4 && SIMPLEVAR(dune.vtx_x)>-150 && SIMPLEVAR(dune.vtx_x)<-100, shift, offLocation, wei));
      fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, axis, cut && SIMPLEVAR(dune.det_x) == (i+1)*4 && SIMPLEVAR(dune.vtx_x)>-100 && SIMPLEVAR(dune.vtx_x)<-50, shift, offLocation, wei));
      fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, axis, cut && SIMPLEVAR(dune.det_x) == (i+1)*4 && SIMPLEVAR(dune.vtx_x)>-50 && SIMPLEVAR(dune.vtx_x)<0, shift, offLocation, wei));
      fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, axis, cut && SIMPLEVAR(dune.det_x) == (i+1)*4 && SIMPLEVAR(dune.vtx_x)>0 && SIMPLEVAR(dune.vtx_x)<50, shift, offLocation, wei));
      fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, axis, cut && SIMPLEVAR(dune.det_x) == (i+1)*4 && SIMPLEVAR(dune.vtx_x)>50 && SIMPLEVAR(dune.vtx_x)<100, shift, offLocation, wei));
      fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, axis, cut && SIMPLEVAR(dune.det_x) == (i+1)*4 && SIMPLEVAR(dune.vtx_x)>100 && SIMPLEVAR(dune.vtx_x)<150, shift, offLocation, wei));
      fvExtrap.push_back(new TrivialExtrap(loaderNonswap, loaderNue, loaderNuTau, axis, cut && SIMPLEVAR(dune.det_x) == (i+1)*4 && SIMPLEVAR(dune.vtx_x)>150 && SIMPLEVAR(dune.vtx_x)<200, shift, offLocation, wei));

    }
    std::cout<<"wait, size of fvExtrp is "<<fvExtrap.size()<<std::endl;

  }



  //----------------------------------------------------------------------
  PredictionOffExtrap::PredictionOffExtrap(PredictionExtrap* pred) : PredictionExtrap(pred->GetExtrap() , 0)
  {
  }

  //----------------------------------------------------------------------
  PredictionOffExtrap::PredictionOffExtrap(Loaders& loaders,
                                         const std::string& label,
                                         const Binning& bins,
                                         const Var& var,
                                         const Cut& cut,
                                         const SystShifts& shift,
                                         const Var& wei)
    : PredictionOffExtrap(loaders, HistAxis(label, bins, var), cut, shift, wei)
  {
  }

  //----------------------------------------------------------------------
  PredictionOffExtrap::PredictionOffExtrap(Loaders& loaders,
                                         const HistAxis& axis,
                                         const Cut& cut,
                                         const SystShifts& shift,
                                         const Var& wei)
    : PredictionExtrap(new TrivialExtrap(loaders, axis, cut, shift, 0, wei), 0)
  {
  }

  //----------------------------------------------------------------------
  void PredictionOffExtrap::SaveTo(TDirectory* dir) const
  {
    TDirectory* tmp = gDirectory;

    dir->cd();

    TObjString("PredictionOffExtrap").Write("type");

    fExtrap->SaveTo(dir->mkdir("extrap"));

    tmp->cd();
  }


  //----------------------------------------------------------------------
  std::unique_ptr<PredictionOffExtrap> PredictionOffExtrap::LoadFrom(TDirectory* dir)
  {
    assert(dir->GetDirectory("extrap"));
    IExtrap* extrap = ana::LoadFrom<IExtrap>(dir->GetDirectory("extrap")).release();
    PredictionExtrap* pred = new PredictionExtrap(extrap,0);
    return std::unique_ptr<PredictionOffExtrap>(new PredictionOffExtrap(pred));
  }


  //----------------------------------------------------------------------
  PredictionOffExtrap::~PredictionOffExtrap()
  {
    // We created this in the constructor so it's our responsibility
    delete fExtrap;
  }
}
