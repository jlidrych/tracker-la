# tracker-la
Lorentz Angle for the CMS silicon strip tracker

Uses nanoAOD-like calibTrees


## Installation and running
```
cmsrel CMSSW_15_0_7
cd CMSSW_15_0_7/src/
cmsenv
git cms-init
git cms-addpkg Configuration/DataProcessing 
git cms-addpkg CalibTracker/SiStripLorentzAngle
cd CalibTracker/SiStripLorentzAngle
git clone https://github.com/robervalwalsh/tracker-la.git bin

scram b -j4
hash -r; cd $CMSSW_BASE/src/CalibTracker/SiStripLorentzAngle/bin
LAMonitor -c la_monitor_df.cfg
```

## Main codes files

* `LAMonitor.cc`
* `LAMonitorConfig.h`
* `CalibTreesUtils.h`
* `Utils.h`
* `la_monitor_df.cfg`

Additional directory containing list of runs and their readout modes
* `readout`

## calibTreeNano root files storage
* `/eos/cms/store/group/dpg_tracker_strip/comm_tracker/Strip/Calibration/calibrationtree/Cosmics*`

## Configuration file

The file `la_monitor_df.cfg` is an example of a configuration file for the LAMonitor. 

**Dataset**

The `calibTree*.root` files must be in the directory `calibTreeDirectory`

**Multithread**

Multithread is default. The user can switch it off with the option

```
multithread = false
```

**Run selection**

The user can either select a single run

```
run=321654
```

or run ranges, from given first and final runs

```
initialRun = 321456
finalRun = 321654
```

or from a given initial run until the last available run
```
initialRun = 321456
finalRun = -1
```

or from the first available run until a given final run
```
initialRun = -1
finalRun = 321654
```

By default all these parameters have value **-1**, meaning that the whole dataset will be considered.<br>
If none of the options is set, then **all** runs will be considered.<br>
If both single run and run ranges are defined simultaneously in the config the run range takes precedence. 

**Readout mode**

If a `readoutList` is given then the analysis will consider only runs with the given `readoutMode` either DECO or PEAK, **default=DECO**.


**Magnetic field**

The user can select the magentic field using the option `bField`, which should be either **4 (default)** for 3.8T, or **0** for 0T runs.

**Track selection**

The track selection can be defined by the parameters

```
ptMin = 5.
nHitsValidMin = 8
chi2ndofMax = 1.4
```
If `bField = 0` no pt selection is performed.



