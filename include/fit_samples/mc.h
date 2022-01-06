// Author: Phoebe Hamilton, Yipeng Sun
// License: BSD 2-clause
// Last Change: Thu Jan 06, 2022 at 05:02 AM +0100

#ifndef _FIT_DEMO_CH_MC_H_
#define _FIT_DEMO_CH_MC_H_

#include <RooStats/HistFactory/Channel.h>
#include <RooStats/HistFactory/Measurement.h>
#include <RooStats/HistFactory/Sample.h>

#include "cmd.h"
#include "loader.h"
#include "utils.h"

/////////////
// Samples //
/////////////
// MC samples to model the data

// B0 -> D*MuNu (normalization)
void addMcNorm(RooStats::HistFactory::Channel& chan, ArgProxy params,
               Config addParams) {
  RooStats::HistFactory::Sample sample("h_sigmu");
  sample.SetHisto(addParams.get<TH1*>("h_sigmu"));

  sample.SetNormalizeByTheory(kFALSE);
  sample.AddNormFactor("Nmu", params.get<double>("expMu"), 1e-6, 1e6);
  sample.AddNormFactor("mcNorm_sigmu", addParams.get<double>("h_sigmu_NormFac"),
                       1e-9, 1.);

  if (params.get<bool>("useMuShapeUncerts")) {
    batchAddHistoSys(sample,
                     {{"v1mu", "h_sigmu_v1m", "h_sigmu_v1p"},
                      {"v2mu", "h_sigmu_v2m", "h_sigmu_v2p"},
                      {"v3mu", "h_sigmu_v3m", "h_sigmu_v3p"}},
                     addParams);
  }

  if (params.get<bool>("bbOn3D")) sample.ActivateStatError();

  chan.AddSample(sample);
}

// B0 -> D*TauNu (signal)
void addMcSig(RooStats::HistFactory::Channel& chan, ArgProxy params,
              Config addParams) {
  RooStats::HistFactory::Sample sample("h_sigtau");
  sample.SetHisto(addParams.get<TH1*>("h_sigtau"));

  sample.SetNormalizeByTheory(kFALSE);
  sample.AddNormFactor("Nmu", params.get<double>("expMu"), 1e-6, 1e6);
  sample.AddNormFactor("RawRDst", params.get<double>("expTau"), 1e-6, 0.2);
  sample.AddNormFactor("mcNorm_sigtau",
                       addParams.get<double>("h_sigtau_NormFac"), 1e-9, 1.);

  if (params.get<bool>("useTauShapeUncerts")) {
    batchAddHistoSys(sample,
                     {{"v1mu", "h_sigtau_v1m", "h_sigtau_v1p"},
                      {"v2mu", "h_sigtau_v2m", "h_sigtau_v2p"},
                      {"v3mu", "h_sigtau_v3m", "h_sigtau_v3p"},
                      {"v4tau", "h_sigtau_v4m", "h_sigtau_v4p"}},
                     addParams);
  }

  if (params.get<bool>("bbOn3D")) sample.ActivateStatError();

  chan.AddSample(sample);
}

// B0 -> D_1MuNu (D** bkg)
void addMcD1(RooStats::HistFactory::Channel& chan, ArgProxy params,
             Config addParams) {
  RooStats::HistFactory::Sample sample("h_D1");
  sample.SetHisto(addParams.get<TH1*>("h_D1"));

  sample.SetNormalizeByTheory(kFALSE);
  sample.AddNormFactor("mcNorm_D1", addParams.get<double>("h_D1_NormFac"), 1e-9,
                       1.);

  if (params.get<bool>("useDststShapeUncerts"))
    batchAddHistoSys(sample, {{"IW", "h_D1IWp", "h_D1IWm"}}, addParams);

  if (params.get<bool>("bbOn3D")) sample.ActivateStatError();

  if (!params.get<bool>("constrainDstst"))
    sample.AddNormFactor("ND1", 1e2, 1e-6, 1e5);
  else {
    sample.AddNormFactor("NDstst0", 0.102, 1e-6, 1e0);
    sample.AddNormFactor("Nmu", params.get<double>("expMu"), 1e-6, 1e6);
    sample.AddNormFactor("fD1", 3.2, 3.2, 3.2);
    sample.AddOverallSys("BFD1", 0.9, 1.1);
  }

  chan.AddSample(sample);
}

#endif
