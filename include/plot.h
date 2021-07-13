// Author: Yipeng Sun
// License: BSD 2-clause
// Last Change: Tue Jul 13, 2021 at 03:52 AM +0200

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
  auto lbl = std::unique_ptr<TLatex>{new TLatex()};
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
                                       char const* name, int width, int height,
                                       TLatex* lbl) {
  auto cvs = std::unique_ptr<TCanvas>{new TCanvas(name, name, width, height)};
  cvs->SetTickx();
  cvs->SetTicky();
  cvs->Divide(frames.size(), 1);

  for (auto idx = 0; idx < frames.size(); idx++) {
    set_fit_vars_frame_style(cvs.get(), frames[idx], idx + 1);
    frames[idx]->Draw();
    lbl->DrawLatex(anchors[idx], frames[idx]->GetMaximum() * 0.95, "Demo");
  }

  return cvs;
}

#endif
