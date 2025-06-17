#include <stdio.h>
double pars_[4];

TFitResultPtr fit(TProfile * pfx);

int analysis()
{
    gStyle->SetOptStat(0);
    gStyle->SetOptFit(0);   

    TCanvas * c1 = new TCanvas("c1","",700,500);

    std::string fname = "LAMonitor_2025C_4T_DECO_393183-393183.root";

    std::cout << "Opening file: " << fname << std::endl;
    TFile * f = new TFile(fname.c_str(),"old");

    TProfile * pfx = ( (TH2D*) f->Get("TIB_L1a_thetatrack_nstrips") )->ProfileX("TIB_L1_pfx");

    // Initial parameters
    pars_[0] = -0.1;
    pars_[1] = 500.;
    pars_[2] = 800.;
    pars_[3] = 0.001;

    TFitResult pfx_fit_result = *(fit(pfx));
    c1 -> Update();

    return 0;
}


TFitResultPtr fit(TProfile * pfx)
{
    TF1Convolution *f_conv;
    f_conv = new TF1Convolution("[1]*TMath::Abs(TMath::Tan(x)-TMath::Tan([0]))+([2]/TMath::Sqrt(TMath::Cos(x)))","exp(-0.5*((x)/[0])**2)",-0.9,0.9,true);
    f_conv->SetNofPointsFFT(1024);
   
    TF1 *func = new TF1("fit_function",*f_conv, -0.9, 0.9, f_conv->GetNpar());
   
    func -> SetParName(0,"#theta_{L}");
    func -> SetParName(1,"a");
    func -> SetParName(2,"b");
    func -> SetParName(3,"#sigma");

    func->SetParameters(pars_);
   
    func -> SetParLimits(0,-0.25,0.1);
    func -> SetParLimits(1,100,2000);
    func -> SetParLimits(2,100,2000);
    func -> FixParameter(3,0.0011);
     
    auto fitres = pfx -> Fit("fit_function","ES+","", -0.4, 0.3);
    
    return fitres;
}