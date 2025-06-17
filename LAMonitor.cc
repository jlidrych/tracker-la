#include "CalibTreesUtils.h"
#include "LAMonitorConfig.h"
#include "ROOT/RDFHelpers.hxx"
#include "ROOT/RCsvDS.hxx"

int main(int argc, char * argv[])
{
   const char * refit = cfg_fit_.c_str();
   
   // Read configuration
   if ( LAMonitorConfig(argc, argv) != 0 ) return -1;

   // Multi-threading
   if ( cfg_mt_ )
   {
      ROOT::EnableImplicitMT();
      std::cout << "Multi-threading enabled" << std::endl; 
   }
   
   // Read Events and Runs dataframes
   std::string filename = cfg_calibTreeDir_+"/calibTree*.root";
   ROOT::RDataFrame df_evt = ROOT::RDataFrame("Events",filename.c_str());
   ROOT::RDataFrame df_run = ROOT::RDataFrame("Runs",filename.c_str());
   
   // // READOUT mode
   // // Making a list of the readout mode to be excluded
   // std::string remove_readout = cfg_readout_mode_ == "DECO" ? "PEAK" : "DECO";
   // ROOT::RDF::RResultPtr<std::vector<Long64_t> > runs_remove;
   // if ( cfg_readout_ != "" )
   // {
   //    ROOT::RDataFrame df_readout = ROOT::RDF::MakeCsvDataFrame(cfg_readout_.c_str());
   //    auto df_readout_remove = df_readout.Filter(Form("mode == \"%s\"",remove_readout.c_str()));
   //    runs_remove = df_readout_remove.Take<Long64_t>("run");
   // }
   
   // *** Process Run information - input to Event ***
   
   // magField - run map from Runs tree
   // N.B. I was not able to use the Runs dataframe directly in a thread safe way
   auto bfield_map = map_bfield(AsRNode(df_run));
   auto bfield = [&bfield_map](const unsigned int & run)
   {
      return bfield_map[run];
   };
   
   // module orientation
   auto mod_orientation_map = map_module_orientation(AsRNode(df_run));
   auto module_orientation = [&mod_orientation_map](const unsigned int & run, const rvec_ui & rawid)
   {
      rvec_i mo;
      for ( auto & mod : rawid )
      {
         mo.push_back(mod_orientation_map[run][mod]);
      }

      return mo;
   };
   
   // *** Process Event information ***
   // Track selection
   // flag good tracks - selection parameters captured
   std::string track_selection = Form("pt > %4.1f GeV, chi2/ndf < %4.1f, hitsvalid >= %2d",cfg_ptmin_,cfg_chi2max_,cfg_hitsmin_);
   if ( cfg_bfield_ == 0 )
      track_selection = Form("chi2/ndf < %4.1f, hitsvalid >= %2d",cfg_chi2max_,cfg_hitsmin_);
   auto track_good = [](const rvec_f & pt, const rvec_i & hitsvalid, const rvec_f & chi2ndof)
   {
      rvec_b is_good;
      float ptmin = cfg_ptmin_;
      if ( cfg_bfield_ == 0 ) ptmin = 0.;
      for ( size_t t = 0; t < hitsvalid.size() ; ++t )
         is_good.push_back(pt[t] > ptmin && hitsvalid[t] >= cfg_hitsmin_ && chi2ndof[t] < cfg_chi2max_);
      return is_good;
   };

//    // flag good tracks at 0 Tesla - selection parameters captured
//    auto track_good_0T = [](const rvec_i & hitsvalid, const rvec_f & chi2ndof)
//    {
//       rvec_b is_good;
//       for ( size_t t = 0; t < hitsvalid.size() ; ++t )
//          is_good.push_back(hitsvalid[t] >= cfg_hitsmin_ && chi2ndof[t] < cfg_chi2max_);
//       return is_good;
//    };
// 
   // Convert to common type RNode
   /* "The conversion to ROOT::RDF::RNode is cheap, but it will introduce an extra virtual 
   call during the RDataFrame event loop (in most cases, the resulting performance impact 
   should be negligible)." RDataFrame doxygen
   */
   auto df_evt_1 = RNode(df_evt);

   // Run/Run range filter
   // Available run range
   int min_run = int(*df_run.Min("run"));
   int max_run = int(*df_run.Max("run"));
   auto run = cfg_run_;
   auto irun = cfg_initial_run_;
   auto frun = cfg_final_run_;
   auto run_selection = run_filter(run,irun,frun,min_run,max_run);
   std::string run_range = Form("%d-%d",irun,frun);
   if ( run > 0 && irun<0 && frun<0 )
      run_range = Form("%d",run);
   // if ( cfg_readout_ != "" )
   //    run_selection.title += Form(" (excluding runs in %s mode)",remove_readout.c_str());
   // Remove runs with non-desired readout more
   // if ( runs_remove )
   // {
   //    for ( auto & rr : runs_remove )  df_evt_1 = df_evt_1.Filter(Form("run!=%lld",rr));
   // }
   df_evt_1 = df_evt_1.Filter(run_selection.filter,run_selection.title);
      
   // Magnetic field filter
   auto magfield = cfg_bfield_;
   if ( magfield != 4 && magfield != 0 )
   {
      std::cout << "Magnetic field neither 4 nor 0 Tesla!" << std::endl;
      return -1;
   }
   auto bfield_selection = bfield_filter(magfield);
   df_evt_1 = df_evt_1
      .Define("bfield"                 ,bfield                  ,{"run"})
      .Filter(bfield_selection.filter,bfield_selection.title)
   ;
      
   // some just-in-case filters, they are expected to cut nothing
   // investigate otherwise
   df_evt_1 = df_evt_1
      .Filter(Form("n%strack>0",refit),"Tracks >= 1"  ) 
      .Filter("ncluster>0"  ,"Clusters >= 1")
   ;
   
   
   // Good tracks
   df_evt_1 = df_evt_1
      .Define(Form("%strack_good",refit) ,track_good    ,{Form("%strack_pt",refit),Form("%strack_hitsvalid",refit),Form("%strack_chi2ndof",refit)});
   
   // Track selection
   df_evt_1 = df_evt_1
      .Define(Form("%strack_index",refit)        ,list_index_f            ,{Form("%strack_pt",refit)})
      .Define("seltrack_pt"            ,Form("%strack_pt[%strack_good]",refit,refit))
      .Filter("seltrack_pt.size()>0"   ,"Selected tracks >= 1, "+track_selection)
      .Define("seltrack_n"             ,"seltrack_pt.size()")
      .Define("seltrack_index"         ,Form("%strack_index[%strack_good]",refit,refit))
      .Define("seltrack_eta"           ,Form("%strack_eta[%strack_good]",refit,refit))
      .Define("seltrack_phi"           ,Form("%strack_phi[%strack_good]",refit,refit))
      .Define("seltrack_chi2ndof"      ,Form("%strack_chi2ndof[%strack_good]",refit,refit))
      .Define("seltrack_hitsvalid"     ,Form("%strack_hitsvalid[%strack_good]",refit,refit))
   ;
   
   // Clusters of selected tracks
   df_evt_1 = df_evt_1
      .Define("cluster_track_good"         , cluster_track_good ,{Form("%strack_good",refit),"cluster_trackindex"})
      .Define("selcluster_rawid"           , "cluster_rawid[cluster_track_good]")
      .Filter("selcluster_rawid.size()>0"  , "Selected tracks clusters >= 1")
      .Define("selcluster_n"               , "selcluster_rawid.size()")
      .Define("selcluster_nstrips"         , "cluster_nstrips[cluster_track_good]")
      .Define("selcluster_trackindex"      , "cluster_trackindex[cluster_track_good]")
      .Define("selcluster_localdirx"       , "cluster_localdirx[cluster_track_good]")
      .Define("selcluster_localdiry"       , "cluster_localdiry[cluster_track_good]")
      .Define("selcluster_localdirz"       , "cluster_localdirz[cluster_track_good]")
      .Define("selcluster_location_type"   , modules_location_type                     ,{"selcluster_rawid"})                      // name of location/type of module
      .Define("selcluster_orientation"     , module_orientation                        ,{"run","selcluster_rawid"})
      .Define("selcluster_thetatrack"      , local_theta_track                         ,{"selcluster_localdirx","selcluster_localdiry","selcluster_localdirz","selcluster_orientation"})
      .Define("selcluster_tanthetatrack"   , local_tantheta_track                      ,{"selcluster_localdirx","selcluster_localdiry","selcluster_localdirz","selcluster_orientation"})
   ;

   // output file 
   std::string output_filename = cfg_outputfile_.c_str();
   output_filename += Form("_%dT",magfield);
   output_filename += Form("_%s",cfg_readout_mode_.c_str());
   output_filename += Form("_%s",run_range.c_str());
   output_filename += ".root";
   
   TFile * f_out = new TFile(output_filename.c_str(),"RECREATE");
   
   // get the barrel layers
   auto layers = barrel_layers();
   
   std::map<std::string, TH1DModel> hmod1d;
   std::map<std::string, TH2DModel> hmod2d;
   std::map<std::string, TProfile1DModel> hmodpf;
   std::map<std::string, RResultPtr<::TH1D > > h1d;
   std::map<std::string, RResultPtr<::TH2D > > h2d;
   std::map<std::string, RResultPtr<::TProfile > > hpf;
   
   hmod1d["seltrack_n"]                = TH1DModel("seltrack_n","",1000, 0, 1000);
   hmod1d["seltrack_pt"]               = TH1DModel("seltrack_pt","",2000, 0,1000);
   hmod1d["seltrack_eta"]              = TH1DModel("seltrack_eta","",100, -4, 4);
   hmod1d["seltrack_phi"]              = TH1DModel("seltrack_phi","",80, -3.2, 3.2);
   hmod1d["seltrack_chi2ndof"]         = TH1DModel("seltrack_chi2ndof","",100, 0, 5);
   hmod1d["seltrack_hitsvalid"]        = TH1DModel("seltrack_hitsvalid","",50, 0, 50);
   
   h1d["seltrack_n"]                = df_evt_1.Histo1D(hmod1d["seltrack_n"],"seltrack_n");
   h1d["seltrack_pt"]               = df_evt_1.Histo1D(hmod1d["seltrack_pt"],"seltrack_pt");
   h1d["seltrack_eta"]              = df_evt_1.Histo1D(hmod1d["seltrack_eta"],"seltrack_eta");
   h1d["seltrack_phi"]              = df_evt_1.Histo1D(hmod1d["seltrack_phi"],"seltrack_phi");
   h1d["seltrack_chi2ndof"]         = df_evt_1.Histo1D(hmod1d["seltrack_chi2ndof"],"seltrack_chi2ndof");
   h1d["seltrack_hitsvalid"]        = df_evt_1.Histo1D(hmod1d["seltrack_hitsvalid"],"seltrack_hitsvalid");
    
   h1d["seltrack_n"]         -> Write(); 
   h1d["seltrack_pt"]        -> Write(); 
   h1d["seltrack_eta"]       -> Write(); 
   h1d["seltrack_phi"]       -> Write(); 
   h1d["seltrack_chi2ndof"]  -> Write(); 
   h1d["seltrack_hitsvalid"] -> Write();    
   
   // loop over layers
   for ( size_t l = 0 ; l < layers.size() ; ++l )
   {
      const char * layer = layers[l].c_str();
      const char * location = Form("selcluster_location_type == \"%s\"",layer);
      df_evt_1 = df_evt_1
         .Define(layers[l]+"_nstrips"     , Form("selcluster_nstrips[%s]"   ,location))
         .Define(layers[l]+"_thetatrack"   , Form("selcluster_thetatrack[%s]",location))
         .Define(layers[l]+"_tanthetatrack", Form("selcluster_tanthetatrack[%s]",location))
         .Define(layers[l]+"_trackindex"   , Form("selcluster_trackindex[%s]",location))
      ;
// Histo models
      hmod1d[Form("%s_nstrips",layer)]                = TH1DModel(Form("%s_nstrips",layer),"",20, 0, 20);
      hmod1d[Form("%s_thetatrack",layer)]             = TH1DModel(Form("%s_thetatrack",layer),"",360, -0.9, 0.9);
      hmod1d[Form("%s_tanthetatrack",layer)]          = TH1DModel(Form("%s_tanthetatrack",layer),"",360, -0.9, 0.9);
      hmod2d[Form("%s_thetatrack_nstrips",layer)]     = TH2DModel(Form("%s_thetatrack_nstrips",layer),"",360, -0.9, 0.9, 20, 0, 20);
      hmod2d[Form("%s_tanthetatrack_nstrips",layer)]  = TH2DModel(Form("%s_tanthetatrack_nstrips",layer),"",360, -0.9, 0.9, 20, 0, 20);
      hmodpf[Form("%s_thetatrack_nstrips_pfx",layer)] = TProfile1DModel(Form("%s_thetatrack_nstrips_pfx",layer),"",360, -0.9, 0.9);
      hmodpf[Form("%s_tanthetatrack_nstrips_pfx",layer)] = TProfile1DModel(Form("%s_tanthetatrack_nstrips_pfx",layer),"",360, -0.9, 0.9);
      
// Fill Histos
      h1d[Form("%s_nstrips",layer)]                = df_evt_1.Histo1D(hmod1d[Form("%s_nstrips",layer)],Form("%s_nstrips",layer));
      h1d[Form("%s_thetatrack",layer)]             = df_evt_1.Histo1D(hmod1d[Form("%s_thetatrack",layer)],Form("%s_thetatrack",layer));
      h1d[Form("%s_tanthetatrack",layer)]          = df_evt_1.Histo1D(hmod1d[Form("%s_tanthetatrack",layer)],Form("%s_tanthetatrack",layer));
      h2d[Form("%s_thetatrack_nstrips",layer)]     = df_evt_1.Histo2D(hmod2d[Form("%s_thetatrack_nstrips",layer)],Form("%s_thetatrack",layer),Form("%s_nstrips",layer));
      h2d[Form("%s_tanthetatrack_nstrips",layer)]  = df_evt_1.Histo2D(hmod2d[Form("%s_tanthetatrack_nstrips",layer)],Form("%s_tanthetatrack",layer),Form("%s_nstrips",layer));
      hpf[Form("%s_thetatrack_nstrips_pfx",layer)] = df_evt_1.Profile1D(hmodpf[Form("%s_thetatrack_nstrips_pfx",layer)],Form("%s_thetatrack",layer),Form("%s_nstrips",layer));
      hpf[Form("%s_tanthetatrack_nstrips_pfx",layer)] = df_evt_1.Profile1D(hmodpf[Form("%s_tanthetatrack_nstrips_pfx",layer)],Form("%s_tanthetatrack",layer),Form("%s_nstrips",layer));
      
// Write Histos
      h1d[Form("%s_nstrips",layer)]  -> Write();  
      h1d[Form("%s_thetatrack",layer)] -> Write();
      h1d[Form("%s_tanthetatrack",layer)] -> Write();
      h2d[Form("%s_thetatrack_nstrips",layer)] -> Write();
      h2d[Form("%s_tanthetatrack_nstrips",layer)] -> Write();
      hpf[Form("%s_thetatrack_nstrips_pfx",layer)] -> Write();
      hpf[Form("%s_tanthetatrack_nstrips_pfx",layer)] -> Write();
   }
   f_out -> Close();

   
   // Request cut-flow report
   auto report = df_evt_1.Report(); 
   
   std::cout << "===============" << std::endl;
   std::cout << "Cut Flow Report" << std::endl;
   std::cout << "===============" << std::endl;
   report->Print();  
   std::cout << "===============" << std::endl;
   
   
   return 0;
}


/*
run :     UInt_t
luminosityBlock :     UInt_t
event :     ULong64_t
norigtrack :     UInt_t
origtrack_chi2ndof :     ROOT::VecOps::RVec<Float_t>
origtrack_eta :     ROOT::VecOps::RVec<Float_t>
origtrack_phi :     ROOT::VecOps::RVec<Float_t>
origtrack_pt :     ROOT::VecOps::RVec<Float_t>
origtrack_hitsvalid :     ROOT::VecOps::RVec<Int_t>
ncluster :     UInt_t
cluster_variance :     ROOT::VecOps::RVec<Float_t>
cluster_localdirx :     ROOT::VecOps::RVec<Float_t>
cluster_localdiry :     ROOT::VecOps::RVec<Float_t>
cluster_localdirz :     ROOT::VecOps::RVec<Float_t>
cluster_barycenter :     ROOT::VecOps::RVec<Float_t>
cluster_localx :     ROOT::VecOps::RVec<Float_t>
cluster_rhlocalx :     ROOT::VecOps::RVec<Float_t>
cluster_rhlocalxerr :     ROOT::VecOps::RVec<Float_t>
cluster_trackindex :     ROOT::VecOps::RVec<Int_t>
cluster_rawid :     ROOT::VecOps::RVec<UInt_t>
cluster_nstrips :     ROOT::VecOps::RVec<UInt_t>
ntrack :     UInt_t
track_chi2ndof :     ROOT::VecOps::RVec<Float_t>
track_eta :     ROOT::VecOps::RVec<Float_t>
track_phi :     ROOT::VecOps::RVec<Float_t>
track_pt :     ROOT::VecOps::RVec<Float_t>
track_hitsvalid :     ROOT::VecOps::RVec<Int_t>


*/


