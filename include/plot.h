// Author: Yipeng Sun
// License: BSD 2-clause
// Last Change: Sun Jul 18, 2021 at 02:33 AM +0200

#ifndef _FIT_DEMO_PLOT_H_
#define _FIT_DEMO_PLOT_H_

#include <vector>

#include <TAxis.h>
#include <TCanvas.h>
#include <TLatex.h>
#include <TROOT.h>
#include <TString.h>
#include <TVirtualPad.h>

#include <RooPlot.h>

/////////////
// Helpers //
/////////////

void set_global_plot_style() {
  gROOT->ProcessLine("gStyle->SetLabelFont(132, \"xyz\");");
  gROOT->ProcessLine("gStyle->SetTitleFont(132, \"xyz\");");
  gROOT->ProcessLine("gStyle->SetTitleFont(132, \"t\");");
  gROOT->ProcessLine("gStyle->SetTitleSize(0.08, \"t\");");
  gROOT->ProcessLine("gStyle->SetTitleY(0.970);");
}

std::unique_ptr<TLatex> make_label() {
  auto lbl = std::make_unique<TLatex>();
  lbl->SetTextAlign(22);
  lbl->SetTextSize(0.06);
  lbl->SetTextFont(132);

  return lbl;
}

////////////////////////
// Plot fit variables //
////////////////////////

void set_fit_vars_frame_style(TCanvas* cvs, RooPlot* frame, int idx) {
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

std::unique_ptr<TCanvas> plot_fit_vars(std::vector<RooPlot*>& frames,
                                       std::vector<double>&   anchors,
                                       char const* name, int width,
                                       int height) {
  auto cvs = std::make_unique<TCanvas>(name, name, width, height);
  cvs->SetTickx();
  cvs->SetTicky();
  cvs->Divide(frames.size(), 1);

  auto lbl = make_label();

  for (auto idx = 0; idx < frames.size(); idx++) {
    set_fit_vars_frame_style(cvs.get(), frames[idx], idx + 1);
    frames[idx]->Draw();
    lbl->DrawLatex(anchors[idx], frames[idx]->GetMaximum() * 0.95, "Demo");
  }

  return cvs;
}

//////////////////////////////////////////
// Plot binned fit variables with pulls //
//////////////////////////////////////////

TPad* set_binned_fit_var_pad(char const* plot_name, double xlow = 0.,
                             double ylow = 0., double xup = 1.,
                             double yup = 0.3, float t_margin = 0.,
                             float b_margin = 0.5, bool gridy = true) {
  auto pad = new TPad(plot_name, plot_name, xlow, ylow, xup, yup);

  pad->SetFillColor(0);
  pad->SetFillStyle(0);
  pad->SetTickx();
  pad->SetTicky();
  if (gridy) pad->SetGridy();

  pad->SetLeftMargin(pad->GetLeftMargin() + 0.08);
  pad->SetTopMargin(t_margin);
  pad->SetRightMargin(0.04);
  pad->SetBottomMargin(b_margin);

  return pad;
}

void set_binned_fit_var_pull_frame_style(RooPlot* frame, char const* title) {
  frame->SetTitle(title);

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

std::unique_ptr<TCanvas> plot_binned_fit_vars_w_pulls(
    std::vector<RooPlot*>& frames, std::vector<RooPlot*>& pulls,
    std::vector<char const*>& cuts, char const* name, int width, int height,
    std::vector<double> max_scale = {1.05, 1.05}) {
  auto cvs  = std::make_unique<TCanvas>(name, name, width, height);
  auto bins = cuts.size();
  char plot_name[32];

  cvs->Divide(bins, 2);

  for (auto idx = 0; idx < frames.size(); idx++) {
    // Bottom pulls
    cvs->cd(idx + 1);
    sprintf(plot_name, "bottompad_%d", idx);

    auto pad_bot = set_binned_fit_var_pad(plot_name);
    pad_bot->Draw();
    pad_bot->cd();

    auto hist_pull = pulls[idx];

    // Prepare for bottom pull plot
  }

  return cvs;
}

#endif
