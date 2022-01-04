// Author: Phoebe Hamilton, Yipeng Sun
// License: BSD 2-clause
// Last Change: Tue Jan 04, 2022 at 03:48 AM +0100

#ifndef _FIT_DEMO_CH_MC_H_
#define _FIT_DEMO_CH_MC_H_

#include <RooStats/HistFactory/Channel.h>
#include <RooStats/HistFactory/Measurement.h>
#include <RooStats/HistFactory/Sample.h>

#include "cmd.h"
#include "loader.h"

/////////////
// Samples //
/////////////
// MC samples to model the data

// B0 -> D*MuNu (normalization)
void addMcNorm(const char* ntp, RooStats::HistFactory::Channel& chan,
               ArgProxy params, Config addParams) {
  RooStats::HistFactory::Sample sample("h_sigmu", "h_sigmu", ntp);

  if (params.get<bool>("useMuShapeUncerts")) {
    sample.AddHistoSys("v1mu", "h_sigmu_v1m", ntp, "", "h_sigmu_v1p", ntp, "");
    sample.AddHistoSys("v2mu", "h_sigmu_v2m", ntp, "", "h_sigmu_v2p", ntp, "");
    sample.AddHistoSys("v3mu", "h_sigmu_v3m", ntp, "", "h_sigmu_v3p", ntp, "");
  }

  if (params.get<bool>("bbOn3D")) sample.ActivateStatError();

  sample.SetNormalizeByTheory(kFALSE);
  sample.AddNormFactor("Nmu", params.get<double>("expMu"), 1e-6, 1e6);
  sample.AddNormFactor("mcNorm_sigmu", addParams.get<double>("mcNorm_sigMu"),
                       1e-9, 1.);

  chan.AddSample(sample);
}

// B0 -> D*TauNu (signal)
void addMcSig(const char* ntp, RooStats::HistFactory::Channel& chan,
              ArgProxy params, Config addParams) {
  RooStats::HistFactory::Sample sample("h_sigtau", "h_sigtau", ntp);

  if (params.get<bool>("useTauShapeUncerts")) {
    sample.AddHistoSys("v1mu", "h_sigtau_v1m", ntp, "", "h_sigtau_v1p", ntp,
                       "");
    sample.AddHistoSys("v2mu", "h_sigtau_v2m", ntp, "", "h_sigtau_v2p", ntp,
                       "");
    sample.AddHistoSys("v3mu", "h_sigtau_v3m", ntp, "", "h_sigtau_v3p", ntp,
                       "");
  }

  if (params.get<bool>("bbOn3D")) sample.ActivateStatError();

  sample.SetNormalizeByTheory(kFALSE);
  sample.AddNormFactor("Nmu", params.get<double>("expMu"), 1e-6, 1e6);
  sample.AddNormFactor("RawRDst", params.get<double>("expTau"), 1e-6, 0.2);
  sample.AddNormFactor("mcNorm_sigtau", addParams.get<double>("mcNorm_sigTau"),
                       1e-9, 1.);

  chan.AddSample(sample);
}

// B0 -> D_1MuNu (D** bkg)
void addMcD1(const char* ntp, RooStats::HistFactory::Channel& chan,
             ArgProxy params, Config addParams) {
  RooStats::HistFactory::Sample sample("h_D1", "h_D1", ntp);

  if (params.get<bool>("useDststShapeUncerts"))
    sample.AddHistoSys("IW", "h_D1IWp", ntp, "", "h_D1IWm", ntp, "");

  if (params.get<bool>("bbOn3D")) sample.ActivateStatError();

  if (!params.get<bool>("constrainDstst"))
    sample.AddNormFactor("ND1", 1e2, 1e-6, 1e5);
  else {
    sample.AddNormFactor("NDstst0", 0.102, 1e-6, 1e0);
    sample.AddNormFactor("Nmu", params.get<double>("expMu"), 1e-6, 1e6);
    sample.AddNormFactor("fD1", 3.2, 3.2, 3.2);
    sample.AddOverallSys("BFD1", 0.9, 1.1);
  }

  sample.SetNormalizeByTheory(kFALSE);
  sample.AddNormFactor("mcNorm_D1", addParams.get<double>("mcNorm_D1"), 1e-9,
                       1.);

  chan.AddSample(sample);
}

#endif
