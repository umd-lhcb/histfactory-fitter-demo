
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

#include "cmd.h"
#include "fit_samples/mc.h"
#include "loader.h"
#include "plot.h"

#define UNBLIND

using namespace std;
using namespace RooFit;
using namespace RooStats;
using namespace HistFactory;

/////////////
// Helpers //
/////////////

unique_ptr<TDatime> get_date() { return make_unique<TDatime>(); }

void HistFactDstTauDemo(TString inputFile, TString outputDir, ArgProxy params) {
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

  TStopwatch sw, sw2, sw3;

  // Many many flags for steering
  /* STEERING OPTIONS */
  const bool constrainDstst       = true;
  const bool useMinos             = true;
  const bool useMuShapeUncerts    = true;
  const bool useTauShapeUncerts   = true;
  const bool useDststShapeUncerts = true;
  const bool fixshapes            = false;
  const bool fixshapesdstst       = false;
  const bool dofit                = true;
  const bool fitfirst             = false;
  const bool BBon3d =
      true;  // flag to enable Barlow-Beeston procedure for all histograms.
  // Should allow easy comparison of fit errors with and
  // without the technique. 3d or not is legacy from an old
  //(3+1)d fit configuration

  // Set the prefix that will appear before
  // all output for this measurement
  RooStats::HistFactory::Measurement meas("DstDemo", "DstDemo");
  meas.SetOutputFilePrefix(static_cast<string>(outputDir + "/fit_output/fit_"));
  meas.SetExportOnly(kTRUE);  // Tells histfactory to not run the fit and
                              // display results using its own

  meas.SetPOI("RawRDst");

  // set the lumi for the measurement.
  // only matters for the data-driven pdfs the way I've set it up.
  // in invfb variable rellumi gives the relative luminosity between the
  // data used to generate the pdfs and the sample we are fitting

  // actually, now this is only used for the misID
  meas.SetLumi(1.0);
  meas.SetLumiRelErr(0.05);
  /******* Fit starting constants ***********/

  // ISOLATED FULL RANGE NONN
  //*
  const double expTau = 0.252 * 0.1742 * 0.781 / 0.85;
  double       expMu  = 50e3;

  double RelLumi = 1.00;

  RooStats::HistFactory::Channel chan("Dstmu_kinematic");
  chan.SetStatErrorConfig(1e-5, "Poisson");

  // tell histfactory what data to use
  chan.SetData("h_data", inputFile.Data());

  // Load fit templates
  // clang-format off
  vector<function<void(const char*, Channel&, ArgProxy, Config)> > templates {
    // Data
    // MC
    addMcNorm, addMcSig, addMcD1
  };
  // clang-format on

  for (auto &t : templates) {
    t(inputFile.Data(), chan, params, addParams);
  }

  /*********************** MisID BKG (FROM DATA)*******************************/

  RooStats::HistFactory::Sample misID("h_misID", "h_misID", inputFile.Data());
  // if(BBon3d) misID.ActivateStatError();

  misID.SetNormalizeByTheory(kTRUE);
  misID.AddNormFactor("NmisID", RelLumi, 1e-6, 1e5);
  chan.AddSample(misID);

  /****** END SAMPLE CHANNELS *******/

  sw3.Reset();
  sw3.Start();
  meas.AddChannel(chan);

  meas.CollectHistograms();

  ////

  RooWorkspace *w;
  w = RooStats::HistFactory::MakeModelAndMeasurementFast(meas);

  ModelConfig *mc = (ModelConfig *)w->obj("ModelConfig");  // Get model manually
  RooSimultaneous *model = (RooSimultaneous *)mc->GetPdf();

  PiecewiseInterpolation *theIW =
      (PiecewiseInterpolation *)w->obj("h_D1_Dstmu_kinematic_Hist_alpha");
  // theIW->disableCache(kTRUE);
  theIW->Print("V");

  RooRealVar *poi =
      (RooRealVar *)mc->GetParametersOfInterest()->createIterator()->Next();
  std::cout << "Param of Interest: " << poi->GetName() << std::endl;

  // Lets tell roofit the right names for our histogram variables //
  RooArgSet * obs = (RooArgSet *)mc->GetObservables();
  RooRealVar *x   = (RooRealVar *)obs->find("obs_x_Dstmu_kinematic");
  RooRealVar *y   = (RooRealVar *)obs->find("obs_y_Dstmu_kinematic");
  RooRealVar *z   = (RooRealVar *)obs->find("obs_z_Dstmu_kinematic");
  x->SetTitle("m^{2}_{miss}");
  x->setUnit("GeV^{2}");
  y->SetTitle("E_{#mu}");
  y->setUnit("MeV");
  z->SetTitle("q^{2}");
  z->setUnit("MeV^{2}");

  // For simultaneous fits, this is the category histfactory uses to sort the
  // channels

  RooCategory *idx  = (RooCategory *)obs->find("channelCat");
  RooAbsData * data = (RooAbsData *)w->data("obsData");

  /* FIX SOME MODEL PARAMS */
  for (int i = 0; i < 3; i++) {
    if (((RooRealVar *)(mc->GetNuisanceParameters()->find(
            "mcNorm_" + mchistos[i]))) != NULL) {
      ((RooRealVar *)(mc->GetNuisanceParameters()->find("mcNorm_" +
                                                        mchistos[i])))
          ->setConstant(kTRUE);
      cout << "mcNorm_" + mchistos[i] + " = "
           << ((RooRealVar *)(mc->GetNuisanceParameters()->find("mcNorm_" +
                                                                mchistos[i])))
                  ->getVal()
           << endl;
    }
  }

  ((RooRealVar *)(mc->GetNuisanceParameters()->find("NDstst0")))->setVal(0.102);
  ((RooRealVar *)(mc->GetNuisanceParameters()->find("NDstst0")))
      ->setConstant(kTRUE);
  ((RooRealVar *)(mc->GetNuisanceParameters()->find("fD1")))
      ->setConstant(kTRUE);
  ((RooRealVar *)(mc->GetNuisanceParameters()->find("NmisID")))
      ->setConstant(kTRUE);

  if (useDststShapeUncerts)
    ((RooRealVar *)(mc->GetNuisanceParameters()->find("alpha_IW")))
        ->setRange(-3.0, 3.0);
  if (useMuShapeUncerts)
    ((RooRealVar *)(mc->GetNuisanceParameters()->find("alpha_v1mu")))
        ->setRange(-8, 8);
  if (useMuShapeUncerts)
    ((RooRealVar *)(mc->GetNuisanceParameters()->find("alpha_v2mu")))
        ->setRange(-8, 8);
  if (useMuShapeUncerts)
    ((RooRealVar *)(mc->GetNuisanceParameters()->find("alpha_v3mu")))
        ->setRange(-8, 8);
  ((RooRealVar *)(mc->GetNuisanceParameters()->find("alpha_BFD1")))
      ->setRange(-3, 3);

  if (fixshapes) {
    ((RooRealVar *)(mc->GetNuisanceParameters()->find("alpha_v1mu")))
        ->setVal(1.06);
    ((RooRealVar *)(mc->GetNuisanceParameters()->find("alpha_v1mu")))
        ->setConstant(kTRUE);
    ((RooRealVar *)(mc->GetNuisanceParameters()->find("alpha_v2mu")))
        ->setVal(-0.159);
    ((RooRealVar *)(mc->GetNuisanceParameters()->find("alpha_v2mu")))
        ->setConstant(kTRUE);
    ((RooRealVar *)(mc->GetNuisanceParameters()->find("alpha_v3mu")))
        ->setVal(-1.75);
    ((RooRealVar *)(mc->GetNuisanceParameters()->find("alpha_v3mu")))
        ->setConstant(kTRUE);
    ((RooRealVar *)(mc->GetNuisanceParameters()->find("alpha_v4tau")))
        ->setVal(0.0002);
    ((RooRealVar *)(mc->GetNuisanceParameters()->find("alpha_v4tau")))
        ->setConstant(kTRUE);
  }
  if (fixshapesdstst) {
    ((RooRealVar *)(mc->GetNuisanceParameters()->find("alpha_IW")))
        ->setVal(-0.005);  //-2.187);
    ((RooRealVar *)(mc->GetNuisanceParameters()->find("alpha_IW")))
        ->setConstant(kTRUE);
  }

  // This switches the model to a class written to handle analytic
  // Barlow-Beeston lite. Otherwise, every bin gets a minuit variable to
  // minimize over! This class, on the other hand, allows a likelihood where the
  // bin parameters are analytically minimized at each step
  HistFactorySimultaneous *model_hf = new HistFactorySimultaneous(*model);

  RooFitResult *toyresult;
  RooAbsReal *  nll_hf;

  RooFitResult *result, *result2;

  cerr << "Saving PDF snapshot" << endl;
  RooArgSet *allpars;
  allpars = (RooArgSet *)((RooArgSet *)mc->GetNuisanceParameters())->Clone();
  allpars->add(*poi);
  RooArgSet *constraints;
  constraints = (RooArgSet *)mc->GetConstraintParameters();
  if (constraints != NULL) allpars->add(*constraints);
  w->saveSnapshot("TMCPARS", *allpars, kTRUE);
  RooRealVar poierror("poierror", "poierror", 0.00001, 0.010);
  TIterator *iter = allpars->createIterator();
  RooAbsArg *tempvar;
  RooArgSet *theVars = (RooArgSet *)allpars->Clone();
  theVars->add(poierror);

  if (dofit) {  // return;
    nll_hf = model_hf->createNLL(*data, Offset(kTRUE));

    RooMinuit *minuit_hf = new RooMinuit(*nll_hf);
    // minuit_hf->setVerbose(kTRUE);
    RooArgSet *temp = new RooArgSet();
    nll_hf->getParameters(temp)->Print("V");
    cout << "******************************************************************"
            "****"
         << endl;
    minuit_hf->setErrorLevel(0.5);
#ifndef UNBLIND
    minuit_hf->setPrintLevel(-1);
#endif

    std::cout << "Minimizing the Minuit (Migrad)" << std::endl;

    w->saveSnapshot("TMCPARS", *allpars, kTRUE);

    sw3.Stop();
    sw.Reset();
    sw.Start();
    minuit_hf->setStrategy(2);
    minuit_hf->fit("smh");
    RooFitResult *tempResult = minuit_hf->save("TempResult", "TempResult");

    cout << tempResult->edm() << endl;
    if (useMinos) minuit_hf->minos(RooArgSet(*poi));
    sw.Stop();
    result = minuit_hf->save("Result", "Result");
  }

  if (result != NULL) {
    printf("Fit ran with status %d\n", result->status());

    printf("Stat error on R(D*) is %f\n", poi->getError());

    printf("EDM at end was %f\n", result->edm());

    result->floatParsInit().Print();

    cout << "CURRENT NUISANCE PARAMETERS:" << endl;
    // TIterator *paramiter = mc->GetNuisanceParameters()->createIterator();
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

    if (dofit)
      printf("Stopwatch: fit ran in %f seconds with %f seconds in prep\n",
             sw.RealTime(), sw3.RealTime());
  }

  ///////////
  // Plots //
  ///////////

  setGlobalPlotStyle();

  // Plot fit variables
  cout << "Plot fit variables..." << endl;
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
  // Parser ////////////////////////////////////////////////////////////////////
  cxxopts::Options argOpts("HistFactDstTauDemo",
                           "a demo R(D*) HistFactory fitter.");

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
    ("doFit", "perform a fit")
    ////
    ("expTau", "?", cxxopts::value<double>()
     ->default_value(to_string(0.252 * 0.1742 * 0.781 / 0.85)))
    ("expMu", "?", cxxopts::value<double>()
     ->default_value("50e3"))
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
    {"bbOn3D", true},
    {"doFit", true},
  });
  // clang-format on

  if (parsedArgs.count("help")) {
    cout << argOpts.help() << endl;
    exit(0);
  }

  auto inputFile = TString(parsedArgs["inputFile"].as<string>());
  auto outputDir = TString(parsedArgs["outputDir"].as<string>());

  HistFactDstTauDemo(inputFile, outputDir, parsedArgsProxy);

  return 0;
}
