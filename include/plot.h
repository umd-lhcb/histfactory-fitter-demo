// Author: Yipeng Sun
// License: BSD 2-clause
// Last Change: Mon Jan 03, 2022 at 03:13 AM +0100

#ifndef _FIT_DEMO_PLOT_H_
#define _FIT_DEMO_PLOT_H_

#include <vector>

#include <TAxis.h>
#include <TCanvas.h>
#include <TLatex.h>
#include <TROOT.h>
#include <TString.h>
#include <TVirtualPad.h>

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

//////////////////////////////////////////
// Plot binned fit variables with pulls //
//////////////////////////////////////////

TPad* setBinnedFitVarPad(char const* plot_name, double xlow = 0.,
                         double ylow = 0., double xup = 1., double yup = 0.3,
                         float tMargin = 0., float bMargin = 0.5,
                         bool gridy = true) {
  auto pad = new TPad(plot_name, plot_name, xlow, ylow, xup, yup);

  pad->SetFillColor(0);
  pad->SetFillStyle(0);
  pad->SetTickx();
  pad->SetTicky();
  if (gridy) pad->SetGridy();

  pad->SetLeftMargin(pad->GetLeftMargin() + 0.08);
  pad->SetTopMargin(tMargin);
  pad->SetRightMargin(0.04);
  pad->SetBottomMargin(bMargin);

  return pad;
}

void setBinnedFitVarPullFrameStyle(RooPlot* frame) {
  frame->SetTitle("");

  frame->GetXaxis()->SetTitleSize(0.36 * 0.22 / 0.3);
  frame->GetXaxis()->SetTitleOffset(0.78);
  frame->GetXaxis()->SetTickLength(0.10);
  frame->GetXaxis()->SetLabelSize(0.33 * 0.22 / 0.3);

  frame->GetYaxis()->SetTitle("Pulls");
  frame->GetYaxis()->SetTitleSize(0.33 * 0.22 / 0.3);
  frame->GetYaxis()->SetTitleOffset(0.2);
  frame->GetYaxis()->SetTickLength(0.05);
  frame->GetYaxis()->SetLabelSize(0.33 * 0.22 / 0.3);
  frame->GetYaxis()->SetLabelOffset(99);
  frame->GetYaxis()->SetNdivisions(205);
}

void setBinnedFitVarMainFrameStyle(RooPlot* frame, char const* lbl) {
  frame->SetTitle(lbl);
  frame->SetTitleFont(132, "t");

  frame->GetXaxis()->SetTitleSize(0.09 * 0.78 / 0.7);
  frame->GetXaxis()->SetTitleOffset(0.95);
  frame->GetXaxis()->SetLabelSize(0.09 * 0.78 / 0.7);

  frame->GetYaxis()->SetTitleSize(0.09 * 0.78 / 0.7);
  frame->GetYaxis()->SetTitleOffset(0.95);
  frame->GetYaxis()->SetLabelSize(0.09 * 0.78 / 0.7);
  frame->GetYaxis()->SetNdivisions(506);
}

std::unique_ptr<TCanvas> plotBinnedFitVarsWithPulls(
    std::vector<RooPlot*>& frames, std::vector<RooPlot*>& pulls,
    std::vector<char const*>& cuts, std::vector<char const*>& lbls,
    char const* name, int width, int height) {
  auto cvs  = std::make_unique<TCanvas>(name, name, width, height);
  auto lbl  = makeLabel();
  auto bins = cuts.size();
  char plotName[32];

  cvs->Divide(bins, 2);

  for (auto idx = 0; idx < frames.size(); idx++) {
    // Bottom pulls
    cvs->cd(idx + 1);
    sprintf(plotName, "bottompad_%d", idx);

    auto padBot = setBinnedFitVarPad(plotName);
    padBot->Draw();
    padBot->cd();

    // Prepare for bottom pull histogram
    auto histPull = pulls[idx];
  }

  return cvs;
}

/////////////
// Plot C1 //
/////////////

void plotC1(std::vector<RooRealVar*> vars, std::vector<TString> titles,
            RooAbsData* data, HistFactorySimultaneous* hf_model) {
  using namespace RooFit;

  std::vector<RooPlot*> frames{};
  std::vector<RooHist*> resids{};

  for (int idx = 0; idx < vars.size(); idx++) {
    frames.push_back(vars[idx]->frame(Title(titles[idx])));
  }
}

#endif
