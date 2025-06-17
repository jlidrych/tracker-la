#ifndef CalibTreesUtils_h_
#define CalibTreesUtils_h_ 1


#include <iostream>
#include "TVector3.h"
#include "ROOT/RVec.hxx"
#include "ROOT/RDataFrame.hxx"
#include "DataFormats/SiStripDetId/interface/SiStripDetId.h"

using namespace ROOT::VecOps;
using namespace ROOT::RDF;
using rvec_f  = RVec<float>;
using rvec_i  = RVec<int>;
using rvec_ui = RVec<unsigned int>;
using rvec_b  = RVec<bool> ;
using rvec_s  = RVec<std::string>;


struct FilterInfo {
  std::string filter;
  std::string title;
};


// functions declarations

/// map of the magnetic field for each run (extracted from the Runs tree)
std::map<unsigned int, float> map_bfield(RNode df, const std::string & c_run="run",const std::string & c_bfield="magField_origin");

/// map of each module orientation per run (extracted from the Runs tree)
std::map<unsigned int, std::map<unsigned int, int> > map_module_orientation(RNode df, const std::string & c_run="run", const std::string & c_rawid="Det_rawid",const std::string & c_zofy="Det_globalZofunitlocalY");

// flag good tracks from selection
rvec_b track_selection(const rvec_f & pt, const rvec_i & hitsvalid, const rvec_f & chi2ndof,const float & ptmin, const int & hitsvalidmin,  const float & chi2ndofmax );

/// vector of the indices of a float list(RVec<float>)
rvec_i list_index_f(const rvec_f & list);

/// vector with barrel layer names
rvec_s barrel_layers();

/// vector with the name of the location of the modules and their type, e.g. TIB_L1s: stereo module at TIB layer 1
rvec_s modules_location_type(const rvec_ui & rawid);

/// name of the location of a given module and its type, e.g. TIB_L1s: stereo module at TIB layer 1
std::string module_location_type(const unsigned int & mod);

/// Get the track local theta angle
rvec_f local_theta_track(const rvec_f & dir_x, const rvec_f & dir_y, const rvec_f & dir_z, const rvec_i & orientation);

/// Get the track local tan(theta)
rvec_f local_tantheta_track(const rvec_f & dir_x, const rvec_f & dir_y, const rvec_f & dir_z, const rvec_i & orientation);

/// flag cluster with good track
rvec_b cluster_track_good(const rvec_b & goodtrack , const rvec_i &trackidx );

/// run filter
FilterInfo run_filter(const int &, int &, int & ,const int &, const int &);

/// mag field filter
FilterInfo bfield_filter(const int &);


// magnetic field for each run
std::map<unsigned int, float> map_bfield(RNode df, const std::string & c_run, const std::string & c_bfield)
{
   
   std::map<unsigned int, float> mapb;
   auto runs = df.Take<unsigned int>(c_run);
   auto bfields = df.Take<float>(c_bfield);
   for ( size_t i = 0 ; i < runs->size() ; ++i ) 
      mapb[runs->at(i)] = bfields->at(i);     // Is this safe in muti-thread if there are duplicated entries in RVec?
   return mapb;
}      

// module orientation for each module for each run
std::map<unsigned int, std::map<unsigned int, int> > map_module_orientation(RNode df, const std::string & c_run, const std::string & c_rawid, const std::string & c_zofy)
{
   
   std::map<unsigned int, std::map<unsigned int, int> > mapr;
   std::map<unsigned int, int> mapmo;
   auto runs = df.Take<unsigned int>(c_run);
   auto rawids = df.Take<rvec_ui>(c_rawid);
   auto zofys = df.Take<rvec_f>(c_zofy);
   
   
   for ( size_t i = 0 ; i < runs->size() ; ++i )
   {
      auto r_rawid = rawids->at(i);
      auto r_zofy = zofys->at(i);
      mapmo.clear();
      // can I do this without the loop? the loop would be to fill the map
      // auto orientation = r_zofy < 0 ? -1 : 1;
      for ( size_t j = 0; j < r_rawid.size(); ++j )
      {
         unsigned int mod = r_rawid[j];
         int orientation = r_zofy[j] < 0 ? -1 : 1;  // WARNING this is true for the barrel, not sure endcaps, for which this variable should not be used??
         mapmo[mod] = orientation;
      }
      mapr[runs->at(i)] = mapmo;
   }
   return mapr;
}      

// vector of the indices of a float list(RVec<float>)
rvec_i list_index_f(const rvec_f & list)
{
   rvec_i idx; 
   for ( size_t i = 0; i < list.size() ; ++i )
      idx.push_back(i);
   return idx;
}

// vector with barrel layer names
rvec_s barrel_layers()
{
   rvec_s dets_barrel = {"TIB","TOB"};
   rvec_s layers_barrel = {"L1a","L1s","L2a","L2s","L3a","L4a","L5a","L6a"};
   rvec_s barrel;
   for ( auto & d: dets_barrel )
   {
      for (auto & l : layers_barrel)
      {
         if ( d =="TIB" and (l=="L5a" || l=="L6a") ) continue;
         barrel.push_back(d+"_"+l);
      }
   }
   return barrel;
}


// vector with the name of the location of the modules and their type,
// e.g. TIB_L1s: stereo module at TIB layer 1
rvec_s modules_location_type(const rvec_ui & rawid)
{
   rvec_s mlt;
   
   for ( auto & mod : rawid )
   {
      mlt.push_back(module_location_type(mod));
   }
   return mlt;
   
}


// Get the track local theta angle
rvec_f local_theta_track(const rvec_f & dir_x, const rvec_f & dir_y, const rvec_f & dir_z, const rvec_i & orientation)
{
   rvec_f theta;
   for ( auto & o : orientation )
   {
      auto i = &o - &orientation[0];
      TVector3 dir(dir_x[i],dir_y[i],dir_z[i]); // to do: use RVec properties w/o loop
      float th = dir.Theta();
      float c_phi = TMath::Cos(dir.Phi()); // integrate
      theta.push_back(o*th*c_phi);
   }
   return theta;
}

// Get the track local tan(theta)
rvec_f local_tantheta_track(const rvec_f & dir_x, const rvec_f & dir_y, const rvec_f & dir_z, const rvec_i & orientation)
{
   rvec_f tantheta;
   for ( auto & o : orientation )
   {
      auto i = &o - &orientation[0];
      TVector3 dir(dir_x[i],dir_y[i],dir_z[i]); // to do: use RVec properties w/o loop
      float th = dir.Theta();
      float c_phi = TMath::Cos(dir.Phi()); // integrate
      float tanth = TMath::Tan(th*c_phi);
      tantheta.push_back(o*tanth);
   }
   return tantheta;
}

// flag cluster with good track
rvec_b cluster_track_good(const rvec_b & goodtrack , const rvec_i &trackidx ) 
{
   rvec_b is_good;
   is_good.clear();

   for ( auto & i : trackidx )
   {
      is_good.push_back(goodtrack[i]);
   }
   return is_good;
}

// flag good tracks from selection
rvec_b track_selection(const rvec_f & pt, const rvec_i & hitsvalid, const rvec_f & chi2ndof,const float & ptmin, const int & hitsvalidmin,  const float & chi2ndofmax )
{
   // using integer instead of boolen due to problems in Print and Histograms
   rvec_b is_good;
   is_good.clear();

   for ( auto & trk : pt )
   {
      auto t = &trk - &pt[0];
      
      if ( pt[t] < ptmin || hitsvalid[t] < hitsvalidmin || chi2ndof[t] > chi2ndofmax )
      {
         is_good.push_back(false);
         continue;
      }      
      is_good.push_back(true);
   }
   return is_good;
}

// run filter
FilterInfo run_filter(const int &run,int &irun,int &frun,const int &min,const int &max)
{
   FilterInfo filter;
   bool single_run = ( run>0 && irun<0 && frun<0 );
   
   filter.filter = "";
   filter.title  = "";
   if ( single_run )
   {
      filter.filter = Form("run==%d",run);
      filter.title = Form("Run: %d",run);
   }
   else
   {
      if ( irun < 0 ) irun = min;
      if ( frun < 0 || frun < irun ) frun = max;
      filter.filter = Form("run>=%d && run<=%d",irun,frun);
      filter.title = Form("Runs: %d - %d",irun,frun);
   }
   return filter;
   
}

FilterInfo bfield_filter(const int & b)
{
   FilterInfo filter;
   filter.filter = "";
   filter.title = "";
   if ( b == 4 ) filter.filter = "bfield>3.7";
   if ( b == 0 ) filter.filter = "bfield<0.1";
   if ( filter.filter != "" )
      filter.title = Form("Magnetic field: %d Tesla",b);
   
   return filter;   
}


// name of the location of a given module and its type,
// e.g. TIB_L1s: stereo module at TIB layer 1
std::string module_location_type( const unsigned int & mod)
{
   const SiStripDetId detid(mod);
   std::string subdet = "";
   if ( detid.subDetector() == SiStripDetId::TIB )  subdet = "TIB";
   if ( detid.subDetector() == SiStripDetId::TOB )  subdet = "TOB";
   if ( detid.subDetector() == SiStripDetId::TID )  subdet = "TID";
   if ( detid.subDetector() == SiStripDetId::TEC )  subdet = "TEC";

   // Barrel  
   int layer = int((mod >> 14) & 0x7);
   std::string type  = (detid.stereo() ? "s": "a");
   std::string d_l_t = Form("%s_L%d%s",subdet.c_str(),layer,type.c_str());
   
   // Endcaps
   if ( subdet == "TID" || subdet == "TEC" )
   {
      d_l_t = Form("%s_L",subdet.c_str());
   }
   
   return d_l_t;
   // TEC+-, TID+- (see also at the bottom of this file
   // side = int((id_>>sideStartBit_) & sideMask_);
   // wheel = ((id_>>wheelStartBit_) & wheelMask_);
   // ring = ((id_>>ringStartBit_) & ringMask_);
}


#endif

// TEC
//   static const unsigned int sideStartBit_=           18;
//   static const unsigned int wheelStartBit_=          14;  
//   static const unsigned int petal_fw_bwStartBit_=    12;
//   static const unsigned int petalStartBit_=          8;
//   static const unsigned int ringStartBit_=           5;
//   static const unsigned int moduleStartBit_=         2;
//   static const unsigned int sterStartBit_=           0;
//   /// two bits would be enough, but  we could use the number "0" as a wildcard
//   static const unsigned int sideMask_=          0x3;
//   static const unsigned int wheelMask_=         0xF;
//   static const unsigned int petal_fw_bwMask_=   0x3;
//   static const unsigned int petalMask_=         0xF;
//   static const unsigned int ringMask_=          0x7;
//   static const unsigned int moduleMask_=        0x7;
//   static const unsigned int sterMask_=          0x3;


// TID

  /// two bits would be enough, but  we could use the number "0" as a wildcard
//   static const unsigned int sideStartBit_=          13;
//   static const unsigned int wheelStartBit_=         11;
//   static const unsigned int ringStartBit_=          9;
//   static const unsigned int module_fw_bwStartBit_=  7;
//   static const unsigned int moduleStartBit_=        2;
//   static const unsigned int sterStartBit_=          0;
//   /// two bits would be enough, but  we could use the number "0" as a wildcard
//   static const unsigned int sideMask_=           0x3;
//   static const unsigned int wheelMask_=          0x3;
//   static const unsigned int ringMask_=           0x3;
//   static const unsigned int module_fw_bwMask_=   0x3;
//   static const unsigned int moduleMask_=         0x1F;
//   static const unsigned int sterMask_=           0x3;

