#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <sys/stat.h>
#include <string>
#include <chrono>
#include <unistd.h>

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//Scenario parameters
static uint32_t algorithm = 100;
static uint32_t Nsta = 100;
static uint32_t Nsaturated = 20;
static int hidden = 0;
static uint32_t value = 60;
static uint32_t simSeed = 1;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//Flags
static uint32_t initflag = 0;
static bool associating_stas_created = false;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//Counters and timers
static uint32_t beaconNumber = 0;
static auto start = std::chrono::high_resolution_clock::now();
static auto finish = std::chrono::high_resolution_clock::now();
static std::chrono::duration<double> elapsed;
static double initialTimeNewStations;
static double sum_time = 0;
static double AssocTime=0;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//Information display
static std::vector<uint32_t> prevInfo;
static std::ostringstream oss;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// MyGym parameters of the enviroment	
static uint32_t port = 5555;
static bool train = false;  
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//Wireless parameters
static uint32_t minvalue = 2;
static uint32_t protocol = 0;
static uint32_t authslot = 8;
static uint32_t SlotFormat=1;
static uint32_t BeaconInterval = 500000;
static uint32_t AssReqTimeout = 512000;
static uint32_t life = 0;
static std::string TrafficInterval="0.04";
static uint32_t tiMin = 64;
static uint32_t tiMax = 255;
static uint32_t payloadSize = 100;
static double bandWidth = 1;
static double sinrDiffCapture = 10;
static bool enableRaw = false;
static bool dataCapture = false;
static bool preambleCapture = false;
static std::string DataMode = "OfdmRate600KbpsBW1MHz";  

