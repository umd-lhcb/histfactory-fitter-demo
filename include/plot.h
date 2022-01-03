// Author: Yipeng Sun
// License: BSD 2-clause
// Last Change: Mon Jan 03, 2022 at 11:18 PM +0100

#ifndef _FIT_DEMO_PLOT_H_
#define _FIT_DEMO_PLOT_H_

#include <vector>

#include <TAxis.h>
#include <TCanvas.h>
#include <TLatex.h>
#include <TROOT.h>
#include <TString.h>
#include <TVirtualPad.h>

#include <RooAbsData.h>
#include <RooCategory.h>
#include <RooGlobalFunc.h>
#include <RooPlot.h>
#include <RooRealVar.h>
#include <RooStats/HistFactory/HistFactorySimultaneous.h>

using RooStats::HistFactory::HistFactorySimultaneous;

/////////////
// Helpers //
/////////////

void setGlobalPlotStyle() {
  gROOT->ProcessLine("gStyle->SetLabelFont(132, \"xyz\");");
  gROOT->ProcessLine("gStyle->SetTitleFont(132, \"xyz\");");
  gROOT->ProcessLine("gStyle->SetTitleFont(132, \"t\");");
  gROOT->ProcessLine("gStyle->SetTitleSize(0.08, \"t\");");
  gROOT->ProcessLine("gStyle->SetTitleY(0.970);");
}

std::unique_ptr<TLatex> makeLabel() {
  auto lbl = std::make_unique<TLatex>();
  lbl->SetTextAlign(22);
  lbl->SetTextSize(0.06);
  lbl->SetTextFont(132);

  return lbl;
}

////////////////////////
// Plot fit variables //
////////////////////////

void setFitVarsFrameStyle(TCanvas* cvs, RooPlot* frame, int idx) {
  auto pad = cvs->cd(idx);

  pad->SetTickx();
  pad->SetTicky();

  pad->SetRightMargin(0.02);
  pad->SetLeftMargin(0.2);
  pad->SetTopMargin(0.02);
  pad->SetBottomMargin(0.13);

  frame->SetTitle("");

  frame->GetXaxis()->SetLabelSize(0.06);
  frame->GetXaxis()->SetTitleSize(0.06);
  frame->GetXaxis()->SetTitleOffset(0.9);

  frame->GetYaxis()->SetLabelSize(0.06);
  frame->GetYaxis()->SetTitleSize(0.06);
  frame->GetYaxis()->SetTitleOffset(1.75);

  TString title = frame->GetYaxis()->GetTitle();
  title.Replace(0, 6, "Candidates");
  frame->GetYaxis()->SetTitle(title);
}

std::unique_ptr<TCanvas> plotFitVars(std::vector<RooPlot*>& frames,
                                     std::vector<double>&   anchors,
                                     char const* name, int width, int height) {
  auto cvs = std::make_unique<TCanvas>(name, name, width, height);
  cvs->SetTickx();
  cvs->SetTicky();
  cvs->Divide(frames.size(), 1);

  auto lbl = makeLabel();

  for (auto idx = 0; idx < frames.size(); idx++) {
    setFitVarsFrameStyle(cvs.get(), frames[idx], idx + 1);
    frames[idx]->Draw();
    lbl->DrawLatex(anchors[idx], frames[idx]->GetMaximum() * 0.95, "Demo");
  }

  return cvs;
}

/////////////
// Plot C1 //
/////////////

std::vector<RooPlot*> plotC1(std::vector<RooRealVar*> vars,
                             std::vector<TString> titles, RooAbsData* data,
                             HistFactorySimultaneous* model_hf,
                             RooCategory*             ch_cat) {
  using namespace RooFit;

  std::vector<RooPlot*> frames{};

  for (int idx = 0; idx < vars.size(); idx++) {
    frames.push_back(vars[idx]->frame(Title(titles[idx])));
  }

  for (int idx = 0; idx < frames.size(); idx++) {
    data->plotOn(frames[idx], DataError(RooAbsData::Poisson),
                 Cut("channelCat==0"), MarkerSize(0.4), DrawOption("ZP"));

    model_hf->plotOn(frames[idx], Slice(*ch_cat), ProjWData(*ch_cat, *data),
                     DrawOption("F"), FillColor(kRed));
    model_hf->plotOn(frames[idx], Slice(*ch_cat), ProjWData(*ch_cat, *data),
                     DrawOption("F"), FillColor(kViolet),
                     Components("*misID*,*sigmu*,*D1*"));
    model_hf->plotOn(frames[idx], Slice(*ch_cat), ProjWData(*ch_cat, *data),
                     DrawOption("F"), FillColor(kBlue + 1),
                     Components("*misID*,*sigmu*"));
    model_hf->plotOn(frames[idx], Slice(*ch_cat), ProjWData(*ch_cat, *data),
                     DrawOption("F"), FillColor(kOrange),
                     Components("*misID*"));
  }

  return frames;
}

#endif
