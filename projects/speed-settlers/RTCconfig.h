//RTCconfig.h - configures the features of the RTCplus library

#ifndef RTCconfig_h
#define RTCconfig_h

//RTCUSEWDT - forces the clock to use the WatchdogTimer, configured for the crystal.
//#define RTCUSEWDT 


//RTCWITHDATE - enables date support. Comment out to remove date support, Uncomment to turn date support on.
#define RTCWITHDATE

//RTCSUBSECONDS - enables 1/256 second ticks. Comment out for 1 second ticks, Uncomment  for 1/256 ticks
#define RTCSUBSECONDS


//RTCUSEVLOCLOCK - enables use of the builtin VLO clock instead of the crystal. Comment out for Crystal, Uncomment for VLO
//                 note that this option is meaningless if RTCUSEWDT is defined. If 
#define RTCUSEVLOCLOCK
#define VLOFREQUENCY 10715  //Actual frequency of the VLO. In theory should be 11999 for 12kHz clock, but tweak until you get something reasonably accurate.

#endif