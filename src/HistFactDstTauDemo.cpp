#include <stdio.h>
#include <iostream>

#include "TCanvas.h"
#include "TDatime.h"
#include "TH3.h"
#include "TIterator.h"
#include "TLatex.h"
#include "TLegend.h"
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
#include <cxxopts.hpp>
#include <map>
#include <string>
#include <iostream>

// Basic ROOT headers
#include <TString.h>

// HistFactory headers

#include "cmd.h"
#include "plot.h"
//#include "fit_samples/mc.h"

#define UNBLIND

using namespace std;

/////////////
// Helpers //
/////////////

unique_ptr<TDatime> get_date() {
  return make_unique<TDatime>();
}

void HistFactDstTauDemo(TString inputFile, TString outputDir, ArgProxy params) {
  using namespace RooFit;
  using namespace RooStats;
  using namespace HistFactory;

  RooMsgService::instance().setGlobalKillBelow(
      RooFit::ERROR);  // avoid accidental unblinding!

  // Below: Read histogram file to generate normalization constants required to
  // make each histo normalized to unity. Not totally necessary here, but
  // convenient

  TFile   q(inputFile);
  TH1 *   htemp;
  TString mchistos[3] = {"sigmu", "sigtau", "D1"};
  double  mcN_sigmu, mcN_sigtau, mcN_D1;
  double *mcnorms[3] = {&mcN_sigmu, &mcN_sigtau, &mcN_D1};
  for (int i = 0; i < 3; i++) {
    q.GetObject("h_" + mchistos[i], htemp);
    assert(htemp != NULL);
    *(mcnorms[i]) = 1. / htemp->Integral();
    cout << "mcN_" + mchistos[i] + " = " << 1. / *(mcnorms[i]) << endl;
  }

  // Useful later to have the bin max and min for drawing
  TH3 *JUNK;
  q.GetObject("h_sigmu", JUNK);
  double    q2_low  = JUNK->GetZaxis()->GetXmin();
  double    q2_high = JUNK->GetZaxis()->GetXmax();
  const int q2_bins = JUNK->GetZaxis()->GetNbins();
  JUNK->SetDirectory(0);
  q.Close();

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
  const bool toyMC                = false;
  const bool fitfirst             = false;
  const bool slowplots            = true;
  const bool BBon3d =
      true;  // flag to enable Barlow-Beeston procedure for all histograms.
  // Should allow easy comparison of fit errors with and
  // without the technique. 3d or not is legacy from an old
  //(3+1)d fit configuration
  const int numtoys = 1;
  const int toysize = 384236;
  // Set the prefix that will appear before
  // all output for this measurement
  RooStats::HistFactory::Measurement meas("my_measurement", "my measurement");
  meas.SetOutputFilePrefix("results/my_measurement");
  meas.SetExportOnly(kTRUE);  // Tells histfactory to not run the fit and
                              // display results using its own

  meas.SetPOI("RawRDst");

  // set the lumi for the measurement.
  // only matters for the data-driven
  // pdfs the way I've set it up. in invfb
  // variable rellumi gives the relative luminosity between the
  // data used to generate the pdfs and the sample
  // we are fitting

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

  // Now that data is set up, start creating our samples
  // describing the processes to model the data

  /*********************** B0->D*munu (NORM) *******************************/

  RooStats::HistFactory::Sample sigmu("h_sigmu", "h_sigmu", inputFile.Data());
  if (useMuShapeUncerts) {
    sigmu.AddHistoSys("v1mu", "h_sigmu_v1m", inputFile.Data(), "",
                      "h_sigmu_v1p", inputFile.Data(), "");
    sigmu.AddHistoSys("v2mu", "h_sigmu_v2m", inputFile.Data(), "",
                      "h_sigmu_v2p", inputFile.Data(), "");
    sigmu.AddHistoSys("v3mu", "h_sigmu_v3m", inputFile.Data(), "",
                      "h_sigmu_v3p", inputFile.Data(), "");
  }
  if (BBon3d) sigmu.ActivateStatError();
  sigmu.SetNormalizeByTheory(kFALSE);
  sigmu.AddNormFactor("Nmu", expMu, 1e-6, 1e6);
  sigmu.AddNormFactor("mcNorm_sigmu", mcN_sigmu, 1e-9, 1.);
  chan.AddSample(sigmu);

  /************************* B0->D*taunu (SIGNAL)
   * *******************************/

  RooStats::HistFactory::Sample sigtau("h_sigtau", "h_sigtau",
                                       inputFile.Data());
  if (useTauShapeUncerts) {
    sigtau.AddHistoSys("v1mu", "h_sigtau_v1m", inputFile.Data(), "",
                       "h_sigtau_v1p", inputFile.Data(), "");
    sigtau.AddHistoSys("v2mu", "h_sigtau_v2m", inputFile.Data(), "",
                       "h_sigtau_v2p", inputFile.Data(), "");
    sigtau.AddHistoSys("v3mu", "h_sigtau_v3m", inputFile.Data(), "",
                       "h_sigtau_v3p", inputFile.Data(), "");
    sigtau.AddHistoSys("v4tau", "h_sigtau_v4m", inputFile.Data(), "",
                       "h_sigtau_v4p", inputFile.Data(), "");
  }
  if (BBon3d) sigtau.ActivateStatError();
  sigtau.SetNormalizeByTheory(kFALSE);
  sigtau.AddNormFactor("Nmu", expMu, 1e-6, 1e6);
  sigtau.AddNormFactor("RawRDst", expTau, 1e-6, 0.2);
  sigtau.AddNormFactor("mcNorm_sigtau", mcN_sigtau, 1e-9, 1.);
  chan.AddSample(sigtau);

  /************************* B0->D1munu **************************************/

  RooStats::HistFactory::Sample d1mu("h_D1", "h_D1", inputFile.Data());
  if (BBon3d) d1mu.ActivateStatError();
  if (useDststShapeUncerts) {
    d1mu.AddHistoSys("IW", "h_D1IWp", inputFile.Data(), "", "h_D1IWm",
                     inputFile.Data(), "");
  }

  d1mu.SetNormalizeByTheory(kFALSE);
  d1mu.AddNormFactor("mcNorm_D1", mcN_D1, 1e-9, 1.);
  if (!constrainDstst) {
    d1mu.AddNormFactor("ND1", 1e2, 1e-6, 1e5);
  } else {
    d1mu.AddNormFactor("NDstst0", 0.102, 1e-6, 1e0);
    d1mu.AddNormFactor("Nmu", expMu, 1e-6, 1e6);
    d1mu.AddNormFactor("fD1", 3.2, 3.2, 3.2);
    d1mu.AddOverallSys("BFD1", 0.9, 1.1);
  }
  chan.AddSample(d1mu);
  /*********************** MisID BKG (FROM DATA)
   * *******************************/

  RooStats::HistFactory::Sample misID("h_misID", "h_misID", inputFile.Data());
  // if(BBon3d) misID.ActivateStatError();

  misID.SetNormalizeByTheory(kTRUE);
  misID.AddNormFactor("NmisID", RelLumi, 1e-6, 1e5);
  chan.AddSample(misID);

  /****** END SAMPLE CHANNELS *******/

  sw3.Reset();
  sw3.Start();
  meas.AddChannel(chan);
  RooStats::HistFactory::Channel chan2(chan);
  chan2.SetName("FakeD");
  // meas.AddChannel(chan2);

  meas.CollectHistograms();
  /*meas.AddConstantParam("mcNorm_sigmu");
    meas.AddConstantParam("mcNorm_sigtau");
    meas.AddConstantParam("mcNorm_D1");
    meas.AddConstantParam("fD1");
    meas.AddConstantParam("NDstst0");
    meas.AddConstantParam("NmisID");
  */

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
  // bin parameters are analyitically minimized at each step
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
  RooDataSet *toyresults = new RooDataSet("toyresults", "toyresults", *theVars,
                                          StoreError(*theVars));
  RooDataSet *toyminos =
      new RooDataSet("toyminos", "toyminos", *theVars, StoreError(*theVars));
  // The following code is very messy. Sorry.
  if (toyMC) {
    double checkvar;
    double checkvarmean = 0;

    if (fitfirst) {
      nll_hf                 = model_hf->createNLL(*data);
      RooMinuit *minuit_temp = new RooMinuit(*nll_hf);
      minuit_temp->setPrintLevel(-1);
      minuit_temp->optimizeConst(1);
      minuit_temp->setErrorLevel(0.5);  // 1-sigma = DLL of 0.5
      minuit_temp->setOffsetting(kTRUE);
      minuit_temp->fit("smh");
      poi->setVal(expTau * 0.85);
      checkvar = ((RooRealVar *)allpars->find("Nmu"))->getVal();
      cout << checkvar << endl;
    } else {
      // lets set some params
      if (useMuShapeUncerts) {
        ((RooRealVar *)(mc->GetNuisanceParameters()->find("alpha_v1mu")))
            ->setVal(1.06);
        ((RooRealVar *)(mc->GetNuisanceParameters()->find("alpha_v2mu")))
            ->setVal(-0.159);
        ((RooRealVar *)(mc->GetNuisanceParameters()->find("alpha_v3mu")))
            ->setVal(-1.75);
      }
      if (useTauShapeUncerts) {
        ((RooRealVar *)(mc->GetNuisanceParameters()->find("alpha_v4tau")))
            ->setVal(0.0002);
      }
      if (useDststShapeUncerts) {
        ((RooRealVar *)(mc->GetNuisanceParameters()->find("alpha_IW")))
            ->setVal(0.2);  //-2.187);
      }
    }

    w->saveSnapshot("GENPARS", *allpars, kTRUE);

    RooDataSet *datanom;
    cerr << "Attempting to generate toyMC..." << endl;
    sw2.Reset();
    sw2.Start();
    double running_mean = 0;
    double running_RMS  = 0;
    for (int runnum = 0; runnum < numtoys; runnum++) {
      w->loadSnapshot("GENPARS");
      cerr << "DEBUG CHECK: Ntau=" << poi->getVal() << endl;
      cout << "PROGRESS: SAMPLE NUMBER " << runnum << " STARTING GENERATION... "
           << endl;
      RooDataSet *data2 = model->generate(
          RooArgSet(*x, *y, *z, model->indexCat()), Name("test"), AllBinned(),
          NumEvents(toysize), Extended());
      double wsum = 0.;
      data2->Print();
      cout << "DONE" << endl;
      w->loadSnapshot("TMCPARS");
      nll_hf                = model_hf->createNLL(*data2);
      RooMinuit *minuit_toy = new RooMinuit(*nll_hf);
      minuit_toy->optimizeConst(1);
      // minuit_toy->setPrintLevel(-1);
      minuit_toy->setOffsetting(kFALSE);
      minuit_toy->setOffsetting(kTRUE);
      minuit_toy->setErrorLevel(0.5);
      minuit_toy->setStrategy(2);
      // minuit_toy->setEps(5e-16);
      cout << "PROGRESS: FITTING SAMPLE " << runnum << " NOW...\t";
      minuit_toy->fit("smh");
      cout << " DONE. SAVING RESULT.\n" << endl;
      checkvarmean += ((RooRealVar *)allpars->find("Nmu"))->getVal();
      cout << "DEBUG CHECK: <Nmu>-Nmu_input = "
           << checkvarmean / (runnum + 1) - checkvar << endl;
      poierror.setVal(poi->getError());
      toyresults->add(*theVars);
      cout << "PROGRESS: ATTEMPTING MINOS FOR SAMPLE " << runnum << " NOW...\t";
      minuit_toy->minos(RooArgSet(*poi));
      RooFitResult *testresult = minuit_toy->save("TOY", "TOY");
      result                   = testresult;
      double edm               = testresult->edm();
      if (edm > 0.1) {
        cout << "BAD FIT. SKIPPING..." << endl;
        continue;
      }
      cout << " DONE. SAVING RESULT.\n" << endl;
      toyminos->add(*theVars);
      delete minuit_toy;
      delete nll_hf;
      if (runnum + 1 < numtoys) {
        delete data2;
        delete testresult;
      }
      data         = data2;
      double pulli = (poi->getVal() - expTau * 0.85) / poi->getError();
      running_mean += pulli;
      running_RMS += pulli * pulli;
      cout << "RUNNING MEAN IS " << running_mean / (runnum + 1)
           << "\tRUNNING RMS IS " << sqrt(running_RMS / (runnum + 1)) << endl;
    }
    sw2.Stop();
    RooFormulaVar poi_pull("poi_pull", "title", "(@0-0.03428)/@1",
                           RooArgList(*poi, poierror));
    RooRealVar *  pulls = (RooRealVar *)toyminos->addColumn(poi_pull);
    pulls->setRange(-5, 5);
    pulls->setBins(50);
    RooRealVar  fitmu("fitmu", "#mu", 0., -5, 5);
    RooRealVar  fitsig("fitsig", "#sigma", 1., 0.1, 10);
    RooGaussian gaus("gaus", "gaus", *pulls, fitmu, fitsig);
    gaus.fitTo(*toyminos, Range(-5, 5));
    TCanvas *toytest = new TCanvas("toytest", "toytest");
    toytest->Divide(2, 1);
    toytest->cd(1);
    RooPlot *testframe = pulls->frame(Title("POI Pull"));
    toyminos->plotOn(testframe);
    gaus.plotOn(testframe);
    gaus.paramOn(testframe);
    testframe->Draw();
    // toytest->cd(2);
    // RooPlot *errframe = poierror.frame(Title("POI Error"));
    // toyresults->plotOn(errframe, Cut("poi_pull > -5 && poi_pull < 5"));
    // errframe->Draw();
    toytest->cd(2);
    RooPlot *valframe = poi->frame(Title("POI"));
    toyresults->plotOn(valframe);
    valframe->Draw();
  }

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
    if (toyMC) {
      w->loadSnapshot("TMCPARS");
    } else {
      w->saveSnapshot("TMCPARS", *allpars, kTRUE);
    }
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
  if (toyMC) {
    printf("Stopwatch: Generated test data in %f seconds\n", sw2.RealTime());
  }

  ///////////
  // Plots //
  ///////////

  setGlobalPlotStyle();
  auto t = makeLabel();

  // Plot fit variables
  RooPlot *mm2_frame = x->frame(Title("m^{2}_{miss}"));
  RooPlot *El_frame  = y->frame(Title("E_{#mu}"));
  RooPlot *q2_frame  = z->frame(Title("q^{2}"));

  const int nframes             = 3;
  RooPlot * drawframes[nframes] = {mm2_frame, El_frame, q2_frame};

  RooHist *resids[nframes];

  for (int i = 0; i < nframes; i++) {
    data->plotOn(drawframes[i], DataError(RooAbsData::Poisson),
                 Cut("channelCat==0"), MarkerSize(0.4), DrawOption("ZP"));
    model_hf->plotOn(drawframes[i], Slice(*idx), ProjWData(*idx, *data),
                     DrawOption("F"), FillColor(kRed));
    model_hf->plotOn(drawframes[i], Slice(*idx), ProjWData(*idx, *data),
                     DrawOption("F"), FillColor(kViolet),
                     Components("*misID*,*sigmu*,*D1*"));
    model_hf->plotOn(drawframes[i], Slice(*idx), ProjWData(*idx, *data),
                     DrawOption("F"), FillColor(kBlue + 1),
                     Components("*misID*,*sigmu*"));
    model_hf->plotOn(drawframes[i], Slice(*idx), ProjWData(*idx, *data),
                     DrawOption("F"), FillColor(kOrange),
                     Components("*misID*"));
    resids[i] = drawframes[i]->pullHist();
    data->plotOn(drawframes[i], DataError(RooAbsData::Poisson),
                 Cut("channelCat==0"), MarkerSize(0.4), DrawOption("ZP"));
  }

  cout << "Plot fit variables..." << endl;
  auto fit_var_frames  = vector<RooPlot *>{mm2_frame, El_frame, q2_frame};
  auto fit_var_anchors = vector<double>{8.7, 2250, 11.1e6};

  auto c1 = plotFitVars(fit_var_frames, fit_var_anchors, "c1", 1000, 300);
  c1->SaveAs(outputDir + "/" + "c1.root");
  c1->SaveAs(outputDir + "/" + "c1.pdf");

  // Slow plot related stuff
  RooPlot *mm2q2_frame[q2_bins];
  RooPlot *Elq2_frame[q2_bins];

  RooPlot *mm2_resid_frame = x->frame(Title("mm2"));
  RooPlot *El_resid_frame  = y->frame(Title("El"));
  RooPlot *q2_resid_frame  = z->frame(Title("q2"));

  RooPlot * q2frames[2 * q2_bins];
  RooPlot * q2bframes[2 * q2_bins];

  for (int i = 0; i < q2_bins; i++) {
    mm2q2_frame[i]         = x->frame();
    Elq2_frame[i]          = y->frame();
    q2frames[i]            = mm2q2_frame[i];
    q2frames[i + q2_bins]  = Elq2_frame[i];
    q2bframes[i]           = x->frame();
    q2bframes[i + q2_bins] = y->frame();
  }

  const int ncomps = 10;

  char     cutstrings[q2_bins][128];
  char     rangenames[q2_bins][32];
  char     rangelabels[q2_bins][128];
  RooHist *mm2q2_pulls[q2_bins];
  RooHist *Elq2_pulls[q2_bins];

  for (int i = 0; i < q2_bins; i++) {
    double binlow  = q2_low + i * (q2_high - q2_low) / q2_bins;
    double binhigh = q2_low + (i + 1) * (q2_high - q2_low) / q2_bins;
    sprintf(rangelabels[i], "%.2f < q^{2} < %.2f", binlow * 1e-6,
            binhigh * 1e-6);
    sprintf(cutstrings[i],
            "obs_z_Dstmu_kinematic > %f && obs_z_Dstmu_kinematic < %f && "
            "channelCat==0",
            q2_low + i * (q2_high - q2_low) / q2_bins,
            q2_low + (i + 1) * (q2_high - q2_low) / q2_bins);
    sprintf(rangenames[i], "q2bin_%d", i);
    z->setRange(rangenames[i], binlow, binhigh);
  }

  if (slowplots == true) {
    cout << "Drawing Slow Plots" << endl;
    for (int i = 0; i < q2_bins; i++) {
      data->plotOn(mm2q2_frame[i], Cut(cutstrings[i]),
                   DataError(RooAbsData::Poisson), MarkerSize(0.4),
                   DrawOption("ZP"));
      data->plotOn(Elq2_frame[i], Cut(cutstrings[i]),
                   DataError(RooAbsData::Poisson), MarkerSize(0.4),
                   DrawOption("ZP"));
      model_hf->plotOn(mm2q2_frame[i], Slice(*idx), ProjWData(*idx, *data),
                       ProjectionRange(rangenames[i]), DrawOption("F"),
                       FillColor(kRed));
      model_hf->plotOn(Elq2_frame[i], Slice(*idx), ProjWData(*idx, *data),
                       ProjectionRange(rangenames[i]), DrawOption("F"),
                       FillColor(kRed));

      // Grab pulls
      mm2q2_pulls[i] = mm2q2_frame[i]->pullHist();
      Elq2_pulls[i]  = Elq2_frame[i]->pullHist();

      model_hf->plotOn(mm2q2_frame[i], Slice(*idx), ProjWData(*idx, *data),
                       ProjectionRange(rangenames[i]), DrawOption("F"),
                       FillColor(kViolet), Components("*sigmu*,*D1*,*misID*"));
      model_hf->plotOn(Elq2_frame[i], Slice(*idx), ProjWData(*idx, *data),
                       ProjectionRange(rangenames[i]), DrawOption("F"),
                       FillColor(kViolet), Components("*sigmu*,*D1*,*misID*"));
      model_hf->plotOn(mm2q2_frame[i], Slice(*idx), ProjWData(*idx, *data),
                       ProjectionRange(rangenames[i]), DrawOption("F"),
                       FillColor(kBlue + 1), Components("*sigmu*,*misID*"));
      model_hf->plotOn(Elq2_frame[i], Slice(*idx), ProjWData(*idx, *data),
                       ProjectionRange(rangenames[i]), DrawOption("F"),
                       FillColor(kBlue + 1), Components("*sigmu*,*misID*"));
      model_hf->plotOn(mm2q2_frame[i], Slice(*idx), ProjWData(*idx, *data),
                       ProjectionRange(rangenames[i]), DrawOption("F"),
                       FillColor(kOrange), Components("*misID*"));
      model_hf->plotOn(Elq2_frame[i], Slice(*idx), ProjWData(*idx, *data),
                       ProjectionRange(rangenames[i]), DrawOption("F"),
                       FillColor(kOrange), Components("*misID*"));
      data->plotOn(mm2q2_frame[i], Cut(cutstrings[i]),
                   DataError(RooAbsData::Poisson), MarkerSize(0.4),
                   DrawOption("ZP"));
      data->plotOn(Elq2_frame[i], Cut(cutstrings[i]),
                   DataError(RooAbsData::Poisson), MarkerSize(0.4),
                   DrawOption("ZP"));
    }
  }

  TCanvas *c2;
  if (slowplots == true) {
    c2 = new TCanvas("c2", "c2", 1200, 600);
    c2->Divide(q2_bins, 2);
    double max_scale  = 1.05;
    double max_scale2 = 1.05;
    char   thename[32];
    for (int k = 0; k < q2_bins * 2; k++) {
      c2->cd(k + 1);
      sprintf(thename, "bottompad_%d", k);

      auto pad_bot = setBinnedFitVarPad(thename);
      pad_bot->Draw();
      pad_bot->cd();

      TH1 *    temphist2lo, *temphist2, *tempdathist;
      RooHist *temphist;
      if (k < q2_bins) {
        temphist = mm2q2_pulls[k];
      } else {
        temphist = Elq2_pulls[k - q2_bins];
      }
      temphist->SetFillColor(kBlue);
      temphist->SetLineColor(kWhite);

      q2bframes[k]->addPlotable(temphist, "B");

      setBinnedFitVarPullFrameStyle(q2bframes[k]);
      q2bframes[k]->Draw();

      double xloc = -2.25;
      if (k >= q2_bins) xloc = 50;
      t->SetTextSize(0.33 * 0.22 / 0.3);
      t->DrawLatex(xloc, -2, "-2");
      t->DrawLatex(xloc * 0.99, 2, " 2");

      c2->cd(k + 1);
      sprintf(thename, "toppad_%d", k);

      auto padtop = setBinnedFitVarPad(thename, 0., 0.3, 1., 1., 0.02, 0,
          false);
      padtop->Draw();
      padtop->cd();

      q2frames[k]->SetMinimum(1e-4);
      if (k < q2_bins)
        q2frames[k]->SetMaximum(q2frames[k]->GetMaximum() * max_scale);
      if (k >= q2_bins)
        q2frames[k]->SetMaximum(q2frames[k]->GetMaximum() * max_scale2);

      setBinnedFitVarMainFrameStyle(q2frames[k], rangelabels[(k % q2_bins)]);
      q2frames[k]->Draw();

      t->SetTextSize(0.07);
      t->SetTextAlign(33);
      t->SetTextAngle(90);

      TString thetitle = q2frames[k]->GetYaxis()->GetTitle();
      c2->cd((k < q2_bins) * (2 * k + 1) +
             (k >= q2_bins) * (2 * (k + 1 - q2_bins)));
      if (k >= q2_bins) {
        t->DrawLatex(0.01, 0.99, thetitle);
      }
      if (k < q2_bins) {
        t->DrawLatex(0.01, 0.99, thetitle);
      }
      t->SetTextAlign(22);
      t->SetTextAngle(0);
      padtop->cd();
      t->SetTextSize(0.09 * 0.78 / 0.7);
      if (k >= q2_bins) {
        t->DrawLatex(2250, q2frames[k]->GetMaximum() * 0.92, "Demo");
      }
      if (k < q2_bins) {
        t->DrawLatex(8.7, q2frames[k]->GetMaximum() * 0.92, "Demo");
      }
    }
  }
  cerr << data->sumEntries() << '\t'
       << model_hf->expectedEvents(RooArgSet(*x, *y, *z, *idx)) << endl;

  if (c2 != NULL) {
    c2->SaveAs(outputDir + "/" + "c2.root");
    c2->SaveAs(outputDir + "/" + "c2.pdf");
  }
}

//////////
// Main //
//////////

int main(int argc, char **argv) {
  // Parser ////////////////////////////////////////////////////////////////////
  cxxopts::Options argparse("HistFactDstTauDemo",
                            "a demo R(D*) HistFactory fitter.");

  // clang-format off
  argparse.add_options()
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
    ("doToyMC", "perform a toy MC study")
    ("fitFirst", "fit before generate toy MC")
    ("numToys", "number of toy MC sample to generate", cxxopts::value<int>()
     ->default_value("1"))
    ("toySize", "size of the generated toy MC", cxxopts::value<int>()
     ->default_value("384236"))
    ////
    ("slowPlots", "use the slower plotting method")
    ////
    ("expTau", "?", cxxopts::value<double>()
     ->default_value(to_string(0.252 * 0.1742 * 0.781 / 0.85)))
    ("expMu", "?", cxxopts::value<double>()
     ->default_value("50e3"))
    ;
  // clang-format on

  auto parsed_args = argparse.parse(argc, argv);
  auto mode        = parsed_args["mode"].as<string>();

  // Define default values for modes
  auto parsed_args_proxy = ArgProxy(parsed_args, mode);
  // clang-format off
  parsed_args_proxy.set_default("fullFit", map<string, any>{
      {"constrainDstst", true},
      {"useMinos", true},
      {"useMuShapeUncerts", true},
      {"useTauShapeUncerts", true},
      {"useDststShapeUncerts", true},
      {"fixShapes", false},
      {"fixShapesDstst", false},
      {"bbOn3D", true},
      {"doFit", true},
      {"doToyMC", false},
      {"fitFirst", false},
      {"slowPlots", true}
  });
  // clang-format on

  if (parsed_args.count("help")) {
    cout << argparse.help() << endl;
    exit(0);
  }

  auto inputFile = TString(parsed_args["inputFile"].as<string>());
  auto outputDir = TString(parsed_args["outputDir"].as<string>());

  HistFactDstTauDemo(inputFile, outputDir, parsed_args_proxy);

  return 0;
}
