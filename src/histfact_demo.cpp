
#include "TDatime.h"
#include "TH3.h"
#include "TIterator.h"
#include "TRandom3.h"
#include "TStopwatch.h"

#include "RooAbsData.h"
#include "RooCategory.h"
#include "RooChi2Var.h"
#include "RooDataHist.h"
#include "RooDataSet.h"
#include "RooExtendPdf.h"
#include "RooFitResult.h"
#include "RooFormulaVar.h"
#include "RooGaussian.h"
#include "RooHist.h"
#include "RooHistPdf.h"
#include "RooMCStudy.h"
#include "RooMinuit.h"
#include "RooMsgService.h"
#include "RooParamHistFunc.h"
#include "RooPoisson.h"
#include "RooRandom.h"
#include "RooRealSumPdf.h"
#include "RooRealVar.h"
#include "RooSimultaneous.h"

#include "RooStats/MinNLLTestStat.h"
#include "RooStats/ModelConfig.h"
#include "RooStats/ToyMCSampler.h"

#include "RooStats/HistFactory/Channel.h"
#include "RooStats/HistFactory/FlexibleInterpVar.h"
#include "RooStats/HistFactory/HistFactoryModelUtils.h"
#include "RooStats/HistFactory/HistFactorySimultaneous.h"
#include "RooStats/HistFactory/MakeModelAndMeasurementsFast.h"
#include "RooStats/HistFactory/Measurement.h"
#include "RooStats/HistFactory/ParamHistFunc.h"
#include "RooStats/HistFactory/PiecewiseInterpolation.h"
#include "RooStats/HistFactory/RooBarlowBeestonLL.h"
////

#include <any>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <stdio.h>

// Third-party headers
#include <cxxopts.hpp>

// Basic ROOT headers
#include <TString.h>

// HistFactory headers

// Project headers
#include "cmd.h"
#include "loader.h"
#include "plot.h"

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

typedef map<TString, double>         NuParamKeyVal;
typedef map<TString, vector<double>> NuParamKeyRange;

void setNuisanceParamConst(ModelConfig *mc, vector<TString> params, bool verbose=false) {
  for (const auto &p : params) {
    auto nuParam =
        static_cast<RooRealVar *>(mc->GetNuisanceParameters()->find(p));
    nuParam->setConstant(kTRUE);

    if (verbose) cout << p << " = " << nuParam->getVal() << endl;
  }
}

void setNuisanceParamVal(ModelConfig *mc, NuParamKeyVal keyVal) {
  for (const auto &kv : keyVal) {
    auto nuParam =
        static_cast<RooRealVar *>(mc->GetNuisanceParameters()->find(kv.first));
    nuParam->setVal(kv.second);
    nuParam->setConstant(kTRUE);
  }
}

void setNuisanceParamRange(ModelConfig *mc, NuParamKeyRange keyRange) {
  for (const auto &kv : keyRange) {
    auto nuParam =
        static_cast<RooRealVar *>(mc->GetNuisanceParameters()->find(kv.first));
    nuParam->setRange(kv.second[0], kv.second[1]);
  }
}

void fixNuisanceParams(ModelConfig *mc) {
  vector<TString> mcHistos{"sigmu", "sigtau", "D1"};
  vector<TString> params{};
  for (auto h : mcHistos) {
    params.push_back("mcNorm_"+h);
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

void fit(TString inputFile, TString outputDir, ArgProxy params) {
  // avoid accidental unblinding!
  RooMsgService::instance().setGlobalKillBelow(RooFit::ERROR);

  // Below: Read histogram file to generate normalization constants required to
  // make each histo normalized to unity. Not totally necessary here, but
  // convenient

  TFile q(inputFile);
  TH1 * htemp;

  TString mchistos[3] = {"sigmu", "sigtau", "D1"};
  double  mcN_sigmu, mcN_sigtau, mcN_D1;
  double *mcnorms[3] = {&mcN_sigmu, &mcN_sigtau, &mcN_D1};
  for (int i = 0; i < 3; i++) {
    q.GetObject("h_" + mchistos[i], htemp);
    assert(htemp != NULL);
    *(mcnorms[i]) = 1. / htemp->Integral();
    cout << "mcN_" + mchistos[i] + " = " << 1. / *(mcnorms[i]) << endl;
  }

  // Adding normalization factors
  Config addParams{};
  addParams.set("mcNorm_sigMu", mcN_sigmu);
  addParams.set("mcNorm_sigTau", mcN_sigtau);
  addParams.set("mcNorm_D1", mcN_D1);

  ///////////////////////////
  // Basic fitter settings //
  ///////////////////////////

  TStopwatch swLoadConfig;
  swLoadConfig.Reset();
  swLoadConfig.Start();

  const bool useMinos       = true;

  // Set the prefix that will appear before all output for this measurement
  RooStats::HistFactory::Measurement meas("demo", "demo");
  meas.SetOutputFilePrefix(static_cast<string>(outputDir + "/fit_output/fit"));
  meas.SetExportOnly(kTRUE);  // Tells histfactory to not run the fit and
                              // display results using its own
  meas.SetPOI("RawRDst");

  // set the lumi for the measurement.
  // only matters for the data-driven pdfs the way I've set it up.
  // in invfb variable rellumi gives the relative luminosity between the
  // data used to generate the pdfs and the sample we are fitting
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
  vector<function<void(const char*, Channel&, ArgProxy, Config)> > templates {
    // Data
    addData, addMisId,
    // MC
    addMcNorm, addMcSig, addMcD1
  };
  // clang-format on

  for (auto &t : templates) {
    t(inputFile.Data(), chan, params, addParams);
  }

  meas.AddChannel(chan);
  meas.CollectHistograms();

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

  auto poi = static_cast<RooRealVar *>(
      mc->GetParametersOfInterest()->createIterator()->Next());
  cout << "Param of Interest: " << poi->GetName() << endl;

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
  HistFactorySimultaneous *model_hf = new HistFactorySimultaneous(*model);
  RooAbsReal *             nll_hf;
  RooFitResult *           result;

  cerr << "Saving PDF snapshot" << endl;
  RooArgSet *allpars;
  allpars = (RooArgSet *)((RooArgSet *)mc->GetNuisanceParameters())->Clone();
  allpars->add(*poi);
  RooArgSet *constraints;
  constraints = (RooArgSet *)mc->GetConstraintParameters();
  if (constraints != NULL) allpars->add(*constraints);
  ws->saveSnapshot("TMCPARS", *allpars, kTRUE);
  RooRealVar poierror("poierror", "poierror", 0.00001, 0.010);
  TIterator *iter = allpars->createIterator();
  RooAbsArg *tempvar;
  RooArgSet *theVars = (RooArgSet *)allpars->Clone();
  theVars->add(poierror);

  auto data = static_cast<RooAbsData *>(ws->data("obsData"));

  nll_hf = model_hf->createNLL(*data, Offset(kTRUE));

  RooMinuit *minuit_hf = new RooMinuit(*nll_hf);
  RooArgSet *temp      = new RooArgSet();
  nll_hf->getParameters(temp)->Print("V");
  minuit_hf->setErrorLevel(0.5);
#ifndef UNBLIND
  minuit_hf->setPrintLevel(-1);
#endif

  ws->saveSnapshot("TMCPARS", *allpars, kTRUE);
  swLoadConfig.Stop();

  cout << "******************************************************************"
       << endl;

  ////////////
  // Do fit //
  ////////////

  cout << "Minimizing the Minuit (Migrad)" << endl;
  TStopwatch sw;
  sw.Reset();
  sw.Start();
  minuit_hf->setStrategy(2);
  minuit_hf->fit("smh");
  RooFitResult *tempResult = minuit_hf->save("TempResult", "TempResult");

  cout << tempResult->edm() << endl;
  if (useMinos) minuit_hf->minos(RooArgSet(*poi));
  sw.Stop();
  result = minuit_hf->save("Result", "Result");

  if (result != NULL) {
    printf("Fit ran with status %d\n", result->status());

    printf("Stat error on R(D*) is %f\n", poi->getError());

    printf("EDM at end was %f\n", result->edm());

    result->floatParsInit().Print();

    cout << "CURRENT NUISANCE PARAMETERS:" << endl;
    TIterator * paramiter         = result->floatParsFinal().createIterator();
    RooRealVar *__temp            = (RooRealVar *)paramiter->Next();
    int         final_par_counter = 0;
    while (__temp != NULL) {
      if (!__temp->isConstant()) {
        if (!(TString(__temp->GetName()).EqualTo(poi->GetName()))) {
          cout << final_par_counter << ": " << __temp->GetName() << "\t\t\t = "
               << ((RooRealVar *)result->floatParsFinal().find(
                       __temp->GetName()))
                      ->getVal()
               << " +/- "
               << ((RooRealVar *)result->floatParsFinal().find(
                       __temp->GetName()))
                      ->getError()
               << endl;
        }
      }
      final_par_counter++;
      __temp = (RooRealVar *)paramiter->Next();
    }

    result->correlationMatrix().Print();

    printf("Stopwatch: fit ran in %f seconds with %f seconds in prep\n",
           sw.RealTime(), swLoadConfig.RealTime());
  }

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
             {"m^{2}_{miss}", "E_{#mu}", "q^{2}"}, data, model_hf, idx);
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
    ("i,inputFile", "input fit templates", cxxopts::value<string>())
    ("o,outputDir", "output directory", cxxopts::value<string>())
    ("m,mode", "fitter mode", cxxopts::value<string>()
     ->default_value("fullFit"))
    ////
    ("constrainDstst", "constrain D** normalization")
    ("useMinos", "?")
    ("useMuShapeUncerts", "constrain normalization shape")
    ("useTauShapeUncerts", "constrain signal shape")
    ("useDststShapeUncerts", "constrain D** shape")
    ////
    ("fixShapes", "?")
    ("fixShapesDstst", "?")
    ("bbOn3D", "enable Barlow-Beeston procedure for all histograms")
    ////
    // ISOLATED FULL RANGE NONN (huh?)
    ("expTau", "?", cxxopts::value<double>()
     ->default_value(to_string(0.252 * 0.1742 * 0.781 / 0.85)))
    ("expMu", "?", cxxopts::value<double>()
     ->default_value("50e3"))
    ("relLumi", "real luminosity", cxxopts::value<double>()
     ->default_value("1.0"))
    ;
  // clang-format on

  auto parsedArgs = argOpts.parse(argc, argv);
  auto mode       = parsedArgs["mode"].as<string>();

  // Define default values for modes
  auto parsedArgsProxy = ArgProxy(parsedArgs, mode);
  // clang-format off
  parsedArgsProxy.set_default("fullFit", map<string, any>{
    {"constrainDstst", true},
    {"useMinos", true},
    {"useMuShapeUncerts", true},
    {"useTauShapeUncerts", true},
    {"useDststShapeUncerts", true},
    {"fixShapes", false},
    {"fixShapesDstst", false},
    {"bbOn3D", true}
  });
  // clang-format on

  if (parsedArgs.count("help")) {
    cout << argOpts.help() << endl;
    exit(0);
  }

  auto inputFile = TString(parsedArgs["inputFile"].as<string>());
  auto outputDir = TString(parsedArgs["outputDir"].as<string>());

  fit(inputFile, outputDir, parsedArgsProxy);

  return 0;
}
