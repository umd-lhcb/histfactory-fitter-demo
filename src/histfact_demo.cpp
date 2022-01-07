// Author: Phoebe Hamilton, Yipeng Sun
// License: BSD 2-clause
// Last Change: Fri Jan 07, 2022 at 04:32 PM +0100

#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <stdio.h>

// Third-party headers
#include <cxxopts.hpp>

// Basic ROOT headers
#include <TStopwatch.h>
#include <TString.h>

// HistFactory headers
#include <RooFitResult.h>
#include <RooMinuit.h>
#include <RooStats/HistFactory/MakeModelAndMeasurementsFast.h>
#include <RooStats/HistFactory/PiecewiseInterpolation.h>

// Project headers
#include "cmd.h"
#include "loader.h"
#include "plot.h"
#include "utils.h"

#include "fit_samples/data.h"
#include "fit_samples/mc.h"

#define UNBLIND

using namespace std;
using namespace RooFit;
using namespace RooStats;
using namespace HistFactory;

///////////////////////
// Fitter setup: Aux //
///////////////////////

void fixNuisanceParams(ModelConfig *mc) {
  vector<TString> mcHistos{"sigmu", "sigtau", "D1"};
  vector<TString> params{};
  for (auto h : mcHistos) {
    params.push_back("mcNorm_" + h);
  }

  setNuisanceParamConst(mc, params, true);
}

void configNuisanceParams(ModelConfig *mc) {
  setNuisanceParamVal(mc, NuParamKeyVal{{"NDstst0", 0.102}});
  setNuisanceParamRange(mc, NuParamKeyRange{{"alpha_BFD1", {-3.0, 3.0}}});
  setNuisanceParamConst(mc, {"fD1", "NmisID"});
}

void useDststShapeUncerts(ModelConfig *mc) {
  setNuisanceParamRange(mc, NuParamKeyRange{{"alpha_IW", {-3.0, 3.0}}});
}

void useMuShapeUncerts(ModelConfig *mc) {
  setNuisanceParamRange(mc, NuParamKeyRange{{"alpha_v1mu", {-8.0, 8.0}},
                                            {"alpha_v2mu", {-8.0, 8.0}},
                                            {"alpha_v3mu", {-8.0, 8.0}}});
}

void fixShapes(ModelConfig *mc) {
  setNuisanceParamVal(mc, {{"alpha_v1mu", 1.06},
                           {"alpha_v2mu", -0.159},
                           {"alpha_v3mu", -1.75},
                           {"alpha_v4tau", 0.0002}});
}

void fixShapesDstst(ModelConfig *mc) {
  setNuisanceParamVal(mc, {{"alpha_IW", 0.005 /* -2.187 */}});
}

////////////////////////
// Fitter setup: Main //
////////////////////////

void fit(ArgProxy params, Config addParams) {
  // avoid accidental unblinding!
  RooMsgService::instance().setGlobalKillBelow(RooFit::ERROR);

  ///////////////////////
  // Initialize fitter //
  ///////////////////////

  auto outputDir = TString(params.get<string>("outputDir"));

  TStopwatch swPrep;
  swPrep.Reset();
  swPrep.Start();

  // Set the prefix that will appear before all output for this measurement
  RooStats::HistFactory::Measurement meas("demo", "demo");
  meas.SetOutputFilePrefix(static_cast<string>(outputDir + "/fit_output/fit"));
  meas.SetExportOnly(kTRUE);  // Tells histfactory to not run the fit and
                              // display results using its own
  meas.SetPOI("RawRDst");

  // set the lumi for the measurement.
  // only matters for the data-driven pdfs the way I've set it up.
  //
  // actually, now this is only used for the misID
  meas.SetLumi(params.get<double>("relLumi"));
  meas.SetLumiRelErr(0.05);

  RooStats::HistFactory::Channel chan("Dstmu_kinematic");
  chan.SetStatErrorConfig(1e-5, "Poisson");

  ////////////////////////
  // Load fit templates //
  ////////////////////////
  // clang-format off
  vector<function<void(Channel&, ArgProxy, Config)>> templates{
    // Data
    addData, addMisId,
    // MC
    addMcNorm, addMcSig, addMcD1
  };
  // clang-format on
  for (auto &t : templates) t(chan, params, addParams);

  meas.AddChannel(chan);
  // This is not needed as we have loaded the histograms manually
  // meas.CollectHistograms();

  ///////////////////////////
  // Change fit parameters //
  ///////////////////////////

  auto ws = RooStats::HistFactory::MakeModelAndMeasurementFast(meas);

  // Get model manually
  auto mc    = static_cast<ModelConfig *>(ws->obj("ModelConfig"));
  auto model = static_cast<RooSimultaneous *>(mc->GetPdf());

  auto theIW = static_cast<PiecewiseInterpolation *>(
      ws->obj("h_D1_Dstmu_kinematic_Hist_alpha"));
  theIW->Print("V");

  // Lets tell roofit the right names for our histogram variables //
  auto obs = static_cast<const RooArgSet *>(mc->GetObservables());

  auto x = static_cast<RooRealVar *>(obs->find("obs_x_Dstmu_kinematic"));
  x->SetTitle("m^{2}_{miss}");
  x->setUnit("GeV^{2}");

  auto y = static_cast<RooRealVar *>(obs->find("obs_y_Dstmu_kinematic"));
  y->SetTitle("E_{#mu}");
  y->setUnit("MeV");

  auto z = static_cast<RooRealVar *>(obs->find("obs_z_Dstmu_kinematic"));
  z->SetTitle("q^{2}");
  z->setUnit("MeV^{2}");

  // Nuisance parameter config
  fixNuisanceParams(mc);
  configNuisanceParams(mc);

  // Looks like shape uncertainties and fix shapes should NOT be enabled at the
  // same time
  if (params.get<bool>("useDststShapeUncerts")) useDststShapeUncerts(mc);
  if (params.get<bool>("useMuShapeUncerts")) useMuShapeUncerts(mc);

  if (params.get<bool>("fixShapes")) fixShapes(mc);
  if (params.get<bool>("fixShapesDstst")) fixShapesDstst(mc);

  // This switches the model to a class written to handle analytic
  // Barlow-Beeston lite. Otherwise, every bin gets a minuit variable to
  // minimize over!  This class, on the other hand, allows a likelihood where
  // the bin parameters are analytically minimized at each step
  unique_ptr<HistFactorySimultaneous> modelHf(
      new HistFactorySimultaneous(*model));

  auto poi = static_cast<RooRealVar *>(
      mc->GetParametersOfInterest()->createIterator()->Next());
  cout << "Param of Interest: " << poi->GetName() << endl;

  cout << "Saving PDF snapshot" << endl;
  auto allPars = static_cast<RooArgSet *>(
      (static_cast<const RooArgSet *>(mc->GetNuisanceParameters()))->Clone());
  allPars->add(*poi);

  auto constraints =
      static_cast<const RooArgSet *>(mc->GetConstraintParameters());
  if (constraints != nullptr) allPars->add(*constraints);

  ws->saveSnapshot("TMCPARS", *allPars, kTRUE);

  RooRealVar poiErr("poierror", "poierror", 0.00001, 0.010);
  auto       theVars = static_cast<RooArgSet *>(allPars->Clone());
  theVars->add(poiErr);

  auto data  = static_cast<RooAbsData *>(ws->data("obsData"));
  auto nllHf = modelHf->createNLL(*data, Offset(kTRUE));

  unique_ptr<RooArgSet> temp(new RooArgSet());
  nllHf->getParameters(temp.get())->Print("V");

  unique_ptr<RooMinuit> minuitHf(new RooMinuit(*nllHf));
  minuitHf->setErrorLevel(0.5);

#ifndef UNBLIND
  minuitHf->setPrintLevel(-1);
#endif

  ws->saveSnapshot("TMCPARS", *allPars, kTRUE);
  swPrep.Stop();

  ////////////
  // Do fit //
  ////////////

  cout << "==============================" << endl;
  cout << "Minimizing the Minuit (Migrad)" << endl;

  TStopwatch swFit;
  swFit.Reset();
  swFit.Start();

  minuitHf->setStrategy(2);
  minuitHf->fit("smh");

  auto tempResult = minuitHf->save("TempResult", "TempResult");
  cout << tempResult->edm() << endl;

  if (params.get<bool>("useMinos")) minuitHf->minos(RooArgSet(*poi));
  auto result = minuitHf->save("Result", "Result");

  swFit.Stop();

  printf("Fit ran with status %d\n", result->status());
  printf("Stat error on R(D*) is %f\n", poi->getError());
  printf("EDM at end was %f\n", result->edm());
  result->floatParsInit().Print();

  cout << "CURRENT NUISANCE PARAMETERS:" << endl;
  auto       paramIter = result->floatParsFinal().createIterator();
  RooAbsArg *paramItem;
  int        finalParamCounter = 0;

  while ((paramItem = static_cast<RooAbsArg *>(paramIter->Next()))) {
    if (!paramItem->isConstant()) {
      if (!(TString(paramItem->GetName()).EqualTo(poi->GetName()))) {
        auto paramVal = static_cast<RooRealVar *>(
            result->floatParsFinal().find(paramItem->GetName()));
        cout << finalParamCounter << ": " << paramItem->GetName()
             << "\t\t\t = " << paramVal->getVal() << " +/- "
             << paramVal->getError() << endl;
      }
    }
    finalParamCounter++;
  }

  result->correlationMatrix().Print();
  printf("Stopwatch: fit ran in %f seconds with %f seconds in prep\n",
         swFit.RealTime(), swPrep.RealTime());

  ///////////
  // Plots //
  ///////////

  setGlobalPlotStyle();

  cout << "Plot fit variables..." << endl;
  // For simultaneous fits, this is the category histfactory uses to sort the
  // channels
  auto idx = static_cast<RooCategory *>(obs->find("channelCat"));
  auto fitVarFrames =
      plotC1(std::vector<RooRealVar *>{x, y, z},
             {"m^{2}_{miss}", "E_{#mu}", "q^{2}"}, data, modelHf.get(), idx);
  auto fitVarAnchors = vector<double>{8.7, 2250, 11.1e6};

  auto c1 = plotFitVars(fitVarFrames, fitVarAnchors, "c1", 1000, 300);
  c1->SaveAs(outputDir + "/" + "c1.pdf");
}

//////////
// Main //
//////////

int main(int argc, char **argv) {
  cxxopts::Options argOpts("histfact_demo", "a demo R(D*) HistFactory fitter.");

  // clang-format off
  argOpts.add_options()
    ("h,help", "print usage")
    ("i,inputDir", "input fit templates", cxxopts::value<string>())
    ("o,outputDir", "output directory", cxxopts::value<string>())
    ("m,mode", "fitter mode", cxxopts::value<string>()
     ->default_value("fullFit"))
    ////
    ("constrainDstst", "constrain D** normalization")
    ("useMuShapeUncerts", "constrain normalization shape")
    ("useTauShapeUncerts", "constrain signal shape")
    ("useDststShapeUncerts", "constrain D** shape")
    ("fixShapes", "?")
    ("fixShapesDstst", "?")
    ////
    ("useMinos", "?")
    ("bbOn3D", "enable Barlow-Beeston procedure for all histograms (legacy)")
    ////
    // ISOLATED FULL RANGE NONN (huh?)
    ("expTau", "?", cxxopts::value<double>()
     ->default_value(to_string(0.252 * 0.1742 * 0.781 / 0.85)))
    ("expMu", "?", cxxopts::value<double>()
     ->default_value("50e3"))
    ("relLumi", "set relative luminosity between data used to generate pdf"
     " and the sample we are fitting, in fb^{-1}.", cxxopts::value<double>()
     ->default_value("1.0"))
    ;
  // clang-format on

  auto parsedArgs = argOpts.parse(argc, argv);
  auto mode       = parsedArgs["mode"].as<string>();

  // Define default values for modes
  auto parsedArgsProxy = ArgProxy(parsedArgs, mode);
  // clang-format off
  parsedArgsProxy.setDefault("fullFit", map<string, any>{
    {"constrainDstst", true},
    {"useMuShapeUncerts", true},
    {"useTauShapeUncerts", true},
    {"useDststShapeUncerts", true},
    {"fixShapes", false},
    {"fixShapesDstst", false},
    {"useMinos", true},
    {"bbOn3D", true}
  });
  // clang-format on

  if (parsedArgs.count("help")) {
    cout << argOpts.help() << endl;
    return 0;
  }

  // Load histograms and compute normalization now
  auto histoLoader = HistoLoader(parsedArgsProxy.get<string>("inputDir"), true);
  histoLoader.load();
  auto addParams = histoLoader.getConfig();

  fit(parsedArgsProxy, addParams);

  return 0;
}
