// Author: Phoebe Hamilton, Yipeng Sun
// License: BSD 2-clause
// Last Change: Thu Jan 06, 2022 at 05:03 AM +0100

#ifndef _FIT_DEMO_CH_DATA_H_
#define _FIT_DEMO_CH_DATA_H_

#include <RooStats/HistFactory/Channel.h>
#include <RooStats/HistFactory/Measurement.h>
#include <RooStats/HistFactory/Sample.h>

#include "cmd.h"
#include "loader.h"

/////////////
// Samples //
/////////////
// Data and data-driven samples

void addData(RooStats::HistFactory::Channel& chan, ArgProxy params,
             Config addParams) {
  chan.SetData(addParams.get<TH1*>("h_data"));
}

void addMisId(RooStats::HistFactory::Channel& chan, ArgProxy params,
              Config addParams) {
  RooStats::HistFactory::Sample sample("h_misID");
  sample.SetHisto(addParams.get<TH1*>("h_misID"));

  sample.SetNormalizeByTheory(kTRUE);
  sample.AddNormFactor("NmisID", params.get<double>("relLumi"), 1e-6, 1e5);

  chan.AddSample(sample);
}

#endif
