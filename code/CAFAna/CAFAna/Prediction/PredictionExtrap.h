#pragma once

#include "StandardRecord/StandardRecord.h"
#include "OscLib/func/IOscCalculator.h"

#include "CAFAna/Prediction/IPrediction.h"

namespace ana
{
  class IExtrap;

  /// Take the output of an extrapolation and oscillate it as required
  class PredictionExtrap: public IPrediction
  {
  public:
    /// Takes ownership of \a extrap
    PredictionExtrap(IExtrap* extrap, int offLocation);
    PredictionExtrap(IExtrap* extrap, int offLocation, std::map <float, std::map <float, std::map<float, float>>> map);
    virtual ~PredictionExtrap();

    virtual Spectrum Predict(osc::IOscCalculator* calc) const override;

    virtual Spectrum PredictComponent(osc::IOscCalculator* calc,
                                      Flavors::Flavors_t flav,
                                      Current::Current_t curr,
                                      Sign::Sign_t sign) const override;

    OscillatableSpectrum ComponentCC(int from, int to) const override;
    Spectrum ComponentNC() const override;

    virtual void SaveTo(TDirectory* dir) const override;
    static std::unique_ptr<PredictionExtrap> LoadFrom(TDirectory* dir);

    PredictionExtrap() = delete;

    float GetOscScale(osc::IOscCalculator* calc, int offLocation, std::map <float, std::map <float, std::map<float, float>>> map) const;

    IExtrap* GetExtrap() const {return fExtrap;}
    IExtrap* GetExtrap(int num) const {return fvExtrap[num];}

    //int pOff;
  protected:

    IExtrap* fExtrap;
    std::vector<IExtrap*> fvExtrap;

    int pOff;
    std::map <float, std::map <float, std::map<float, float>>> pMap;
  };
}
