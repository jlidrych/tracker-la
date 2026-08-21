#ifndef LAMonitorConfig_h_
#define LAMonitorConfig_h_ 1

#include "boost/program_options.hpp"
#include "boost/algorithm/string.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include "Utils.h"


int LAMonitorConfig(int argc, char * argv[]);


std::string cfg_;
bool cfg_mt_;
int cfg_run_;
int cfg_initial_run_;
int cfg_final_run_;
int cfg_bfield_;

std::string cfg_outputfile_;
std::string cfg_calibTreeDir_;
std::string cfg_jsonFile_;
std::string cfg_readout_;
std::string cfg_readout_mode_;
std::string cfg_fit_;

float cfg_etamin_;
float cfg_etamax_;
float cfg_ptmin_;
float cfg_ptmax_;
int   cfg_hitsmin_;
int   cfg_hitsmax_;
float cfg_chi2max_;
float cfg_chi2min_;

TrackSelection cfg_track_sel_;

int LAMonitorConfig(int argc, char * argv[])
{
   try
   {
      namespace po = boost::program_options;
      po::options_description desc("Options");
      desc.add_options()
         ("help,h","Show help messages")
         ("config,c",po::value<std::string>(&cfg_),"Configuration file name")
         ("multi_thread",po::value <bool> (&cfg_mt_), "Use multi-thread")
         ("tree_dir",po::value<std::string>(&cfg_calibTreeDir_),"Direcotry of the calibTree files")
         ("jsonFile",po::value<std::string>(&cfg_jsonFile_),"Path to the json file")
         ("run",po::value<int>(&cfg_run_),"Run number")
         ("initial_run",po::value <int> (&cfg_initial_run_), "Initial run number")
         ("final_run",po::value <int> (&cfg_final_run_), "Final run number")
         ;
      
      po::options_description config("Configuration");
      config.add_options()
         ("multithread",po::value <bool> (&cfg_mt_)->default_value(true), "Use multi-thread")
         ("run",po::value <int> (&cfg_run_)->default_value(-1), "Run number")
         ("initialRun",po::value <int> (&cfg_initial_run_)->default_value(-1), "Initial run number")
         ("finalRun",po::value <int> (&cfg_final_run_)->default_value(-1), "Final run number")
         ("bField",po::value <int> (&cfg_bfield_)->default_value(4), "Magnetic field")
         ("calibTreeDirectory",po::value <std::string> (&cfg_calibTreeDir_)->default_value("/nfs/dust/cms/user/walsh/store/group/dpg_tracker_strip/comm_tracker/Strip/Calibration/calibrationtree/Cosmics18Legacy/nanotest3/2018D"),"Direcotry of the calibTree files")
         ("jsonFile", po::value <std::string> (&cfg_jsonFile_)->default_value("./run3_lumi.json.gz"), "Path to the json file")
         ("readoutList",po::value <std::string> (&cfg_readout_)->default_value(""))
         ("readoutMode",po::value <std::string> (&cfg_readout_mode_)->default_value("DECO"))
         ("trackFit",po::value <std::string> (&cfg_fit_)->default_value(""))
         ("outputFile",po::value <std::string> (&cfg_outputfile_)->default_value("LAMonitor"),"Basename of the root output file")
         ("ptMin",po::value  <float> (&cfg_ptmin_)->default_value(-1), "Min track pT")
         ("ptMax",po::value  <float> (&cfg_ptmax_)->default_value(-1), "Max track pT")
         ("etaMin",po::value <float> (&cfg_etamin_)->default_value(-10), "Min track eta")
         ("etaMax",po::value <float> (&cfg_etamax_)->default_value(10) , "Max track eta")
         ("nHitsValidMin",po::value <int> (&cfg_hitsmin_)->default_value(-1) , "Min number of valid hits")
         ("nHitsValidMax",po::value <int> (&cfg_hitsmax_)->default_value(-1) , "Max number of valid hits")
         ("chi2ndofMin",po::value <float> (&cfg_chi2min_)->default_value(-1) , "Min chi2/ndof")
         ("chi2ndofMax",po::value <float> (&cfg_chi2max_)->default_value(-1) , "Max chi2/ndof");
      
      po::variables_map vm; 
      try
      {
         po::store(po::parse_command_line(argc, argv, desc), vm); // can throw
         // --help option
         
         if ( vm.count("help") )
         {
            std::cout << "SiStripLAMonitor" << std::endl
                      << desc << std::endl
                      << config << std::endl;
            return 1;
         }
         po::notify(vm);
         
         std::ifstream cfg_s(cfg_.c_str());
         po::store(po::parse_config_file(cfg_s, config), vm); // can throw
         if ( vm.count("config") )
         {

         }
         po::notify(vm);
         
         cfg_track_sel_.ptmin = cfg_ptmin_;
         cfg_track_sel_.ptmax = cfg_ptmax_;
         cfg_track_sel_.etamin = cfg_etamin_;
         cfg_track_sel_.etamax = cfg_etamax_;
         cfg_track_sel_.chi2min = cfg_chi2min_;
         cfg_track_sel_.chi2max = cfg_chi2max_;
         cfg_track_sel_.hitsmin = cfg_hitsmin_;
         cfg_track_sel_.hitsmax = cfg_hitsmax_;
         
      }
      catch(po::error& e)
      { 
         std::cerr << "ERROR: " << e.what() << std::endl << std::endl; 
         std::cerr << desc << std::endl; 
         return -1; 
      } 
      
   }
   catch(std::exception& e) 
   { 
      std::cerr << "ERROR: " << e.what() << std::endl << std::endl; 
      return -1; 
   } 
   
   return 0;}
#endif
