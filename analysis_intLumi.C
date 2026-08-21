#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include "TCanvas.h"
#include "TFile.h"
#include "TF1.h"
#include "TF1Convolution.h"
#include "TFitResult.h"
#include "TFitResultPtr.h"
#include "TH2D.h"
#include "TH3D.h"
#include "TMath.h"
#include "TProfile.h"
#include "TStyle.h"
#include "TTree.h"

double pars_[4];


TFitResultPtr fit(TProfile *pfx, const std::string &fitName);

int analysis_intLumi(double lumiWindow = 5.0)
{
    gStyle->SetOptStat(0);
    gStyle->SetOptFit(0);

    TTree *outTree = new TTree("fitResults", "fitResults");

    double lumiMinOut;
    double lumiMaxOut;
    double lumiCenterOut;
    double thetaL;
    double thetaLError;
    double chi2;
    int ndf;
    int fitStatus;
    double nEntries;

    outTree->Branch("lumiMin", &lumiMinOut);
    outTree->Branch("lumiMax", &lumiMaxOut);
    outTree->Branch("lumiCenter", &lumiCenterOut);
    outTree->Branch("thetaL", &thetaL);
    outTree->Branch("thetaLError", &thetaLError);
    outTree->Branch("chi2", &chi2);
    outTree->Branch("ndf", &ndf);
    outTree->Branch("fitStatus", &fitStatus);
    outTree->Branch("nEntries", &nEntries);

    if (lumiWindow <= 0.0) {
        std::cerr << "ERROR: lumiWindow must be > 0" << std::endl;
        return 1;
    }

    TCanvas *c1 = new TCanvas("c1","",700,500);

    std::string fname = "LAMonitor_Part3_4T_DECO_392159-396595.root";
    std::string histoname = "TOB_L1a_thetatrack_nstrips_intLumi";

    std::cout << "Opening file: "<< fname<< std::endl;

    TFile *f = TFile::Open(fname.c_str(),"READ");

    if (!f || f->IsZombie()) {
        std::cerr << "ERROR: could not open file "<< fname << std::endl;
        return 1;
    }

    TH3D *h3 = dynamic_cast<TH3D *>(f->Get(histoname.c_str()));

    if (!h3) {
        std::cerr << "ERROR: histogram " << histoname<< " not found"<< std::endl;
        return 1;
    }

    TAxis *zAxis = h3->GetZaxis();

    const double zMin = zAxis->GetXmin();
    const double zMax = zAxis->GetXmax();

    std::cout << "Integrated luminosity range: " << zMin << " - " << zMax << std::endl;

    std::cout << "Projection window: " << lumiWindow << std::endl;

    // Initial fit parameters
    pars_[0] = -0.1;
    pars_[1] = 500.;
    pars_[2] = 800.;
    pars_[3] = 0.001;

    int iWindow = 0;

    for ( double lumiMin = zMin; lumiMin < zMax; lumiMin += lumiWindow)
    {
        const double lumiMax = std::min(lumiMin + lumiWindow, zMax);

        //
        // Use [lumiMin, lumiMax)
        //
        int zBinMin = zAxis->FindFixBin(lumiMin);

        int zBinMax;

        if (lumiMax < zMax) {
            zBinMax = zAxis->FindFixBin(std::nextafter(lumiMax,lumiMin));
        } else {
            zBinMax = zAxis->GetNbins();
        }

        if (zBinMin < 1) {
            zBinMin = 1;
        }

        if (zBinMax > zAxis->GetNbins()) {
            zBinMax = zAxis->GetNbins();
        }

        std::cout << "\nWindow " << iWindow<< ": "<< lumiMin << " - " << lumiMax << " fb^-1" << "   z bins " << zBinMin << " - " << zBinMax << std::endl;

        // Select z range
        zAxis->SetRange(zBinMin, zBinMax);

        // Project TH3D -> TH2D
        TH2D *h2 = dynamic_cast<TH2D *>( h3->Project3D("yx"));

        if (!h2) {
            std::cerr << "ERROR: Project3D failed" << std::endl;
            ++iWindow;
            continue;
        }

        std::string h2Name =histoname + std::to_string(iWindow);
        h2->SetName(h2Name.c_str());
        // Skip empty projections
        if (h2->GetEntries() == 0) {
            std::cout<< "No entries, skipping." << std::endl;
            delete h2;
            ++iWindow;
            continue;
        }

        // TH2D -> ProfileX
        std::string profileName = histoname + "pfx_lumi_" + std::to_string(iWindow);

        TProfile *pfx = h2->ProfileX(profileName.c_str());

        if (!pfx || pfx->GetEntries() == 0) {
            std::cout<< "Empty profile, skipping."<< std::endl;
            delete pfx;
            delete h2;
            ++iWindow;
            continue;
        }

        // Fit profile
        std::string fitName = "fit_function_" + std::to_string(iWindow);
        TFitResultPtr fitResult = fit(pfx, fitName);
        if ( fitResult.Get() && fitResult->IsValid()) {
            std::cout<< "Fit OK" << std::endl;
            std::cout<< "theta_L = "<< fitResult->Parameter(0)<< " +/- "<< fitResult->ParError(0)<< std::endl;
        } else {
            std::cout<< "Fit failed"<< std::endl;
        }

        // Optional drawing
        c1->cd();

        pfx->SetTitle(Form("Integrated luminosity %.2f - %.2f fb^{-1}",lumiMin,lumiMax));
        pfx->Draw();
        c1->SaveAs(Form("fit-lumiMin%.2f-lumiMax%.2f.pdf",lumiMin, lumiMax));
        c1->Update();

        lumiMinOut = lumiMin;
        lumiMaxOut = lumiMax;
        lumiCenterOut = 0.5 * (lumiMin + lumiMax);

        thetaL = fitResult->Parameter(0);
        thetaLError = fitResult->ParError(0);

        chi2 = fitResult->Chi2();
        ndf = fitResult->Ndf();

        fitStatus = fitResult->Status();

        nEntries = pfx->GetEntries();

        outTree->Fill();

        //
        // Keep ROOT objects alive if you want to inspect them
        // interactively. Therefore we do not delete h2/pfx here.
        //

        ++iWindow;
    }

    // Restore full z-axis range
    zAxis->SetRange(0,0);

    std::cout<< "\nProcessed "<< iWindow<< " luminosity windows."<< std::endl;
    TFile *outFile = new TFile("fitResults.root", "RECREATE");
    outTree->Write();
    outFile->Close();
    return 0;
}


TFitResultPtr fit(
    TProfile *pfx,
    const std::string &fitName
)
{
    TF1Convolution *f_conv =
        new TF1Convolution(
            "[1]*TMath::Abs(TMath::Tan(x)-TMath::Tan([0]))"
            "+([2]/TMath::Sqrt(TMath::Cos(x)))",
            "exp(-0.5*((x)/[0])**2)", -0.9, 0.9, true
        );

    f_conv->SetNofPointsFFT(1024);

    TF1 *func = new TF1(fitName.c_str(),*f_conv,-0.9,0.9,f_conv->GetNpar());

    func->SetParName(0,"#theta_{L}");
    func->SetParName(1,"a");
    func->SetParName(2,"b");
    func->SetParName(3,"#sigma");
    func->SetParameters(pars_);
    func->SetParLimits(0,-0.25,0.1);
    func->SetParLimits(1,100,2000);
    func->SetParLimits(2,100,2000);
    func->FixParameter(3,0.0011);

    auto fitres = pfx->Fit(func,"ES+","",-0.4,0.3);

    return fitres;
}