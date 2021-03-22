/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// General libraries
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#include "ns3/log.h"
#include "ns3/command-line.h"
#include "ns3/object-vector.h"
#include "ns3/pointer.h"
#include "ns3/config.h"
#include "ns3/rng-seed-manager.h"
#include "sim.h"
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ns3 helpers
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/mac-low.h"
#include "ns3/mobility-helper.h"
#include "ns3/s1g-wifi-mac-helper.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/yans-wifi-helper.h"
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ns3 models
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#include "ns3/ap-wifi-mac.h"
#include "ns3/arp-cache.h"
#include "ns3/node-list.h"
#include "ns3/ipv4-packet-probe.h"
#include "ns3/ipv4-raw-socket-impl.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/wifi-net-device.h"
#include "ns3/yans-wifi-phy.h"
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initial configuration
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("halow_bs");

static std::map <Mac48Address, double> saturated_times, assoc_times;
static std::map <Mac48Address, int> assoc_indices;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Container definition
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
struct Experiment {
  ApplicationContainer serverApp;
  Ipv4InterfaceContainer apNodeInterface;
  NodeContainer wifiStaNode;
  NodeContainer wifiSaturatedStaNode;
  NodeContainer wifiApNode;
  NetDeviceContainer staDevice;
  NetDeviceContainer apDevice;
  YansWifiChannelHelper channel;
  YansWifiPhyHelper phy;
  Ssid ssid = Ssid ("ns380211ah");
  WifiHelper wifi;
  InternetStackHelper stack;
  Ipv4AddressHelper address;
} *experiment;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// PopulateArpCache function
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
static void
PopulateArpCache ()
{
  Ptr<ArpCache> arp = CreateObject<ArpCache> ();
  arp->SetAliveTimeout (Seconds(3600 * 24 * 365));
  for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i)
    {
      Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
      NS_ASSERT(ip !=0);
      ObjectVectorValue interfaces;
      ip->GetAttribute("InterfaceList", interfaces);
      for(ObjectVectorValue::Iterator j = interfaces.Begin(); j != interfaces.End (); j++)
        {
          Ptr<Ipv4Interface> ipIface = (j->second)->GetObject<Ipv4Interface> ();
          NS_ASSERT(ipIface != 0);
          Ptr<NetDevice> device = ipIface->GetDevice();
          NS_ASSERT(device != 0);
          Mac48Address addr = Mac48Address::ConvertFrom(device->GetAddress ());
          for(uint32_t k = 0; k < ipIface->GetNAddresses (); k ++)
            {
              Ipv4Address ipAddr = ipIface->GetAddress (k).GetLocal();
              if(ipAddr == Ipv4Address::GetLoopback())
                continue;
              ArpCache::Entry * entry = arp->Add(ipAddr);
              entry->MarkWaitReply(0);
              entry->MarkAlive(addr);
            }
      }
    }
  for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i)
    {
      Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
      NS_ASSERT(ip !=0);
      ObjectVectorValue interfaces;
      ip->GetAttribute("InterfaceList", interfaces);
      for(ObjectVectorValue::Iterator j = interfaces.Begin(); j !=interfaces.End (); j ++)
        {
          Ptr<Ipv4Interface> ipIface = (j->second)->GetObject<Ipv4Interface> ();
          ipIface->SetAttribute("ArpCache", PointerValue(arp));
        }
    }
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// nonSaturatedAssoc function
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
static void
nonSaturatedAssoc (Mac48Address address)
{
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// Verify if the address was associated yet
  if (assoc_times.find(address) != assoc_times.end())
    {
      std::cerr << "Double association from " << address << std::endl;
    }
  assoc_times[address] = Simulator::Now().GetSeconds();
  sum_time += Simulator::Now().GetMicroSeconds();
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Get authentication time and stop the simulation
  if (assoc_times.size () == Nsta)
    {      
      std::ofstream results;
      results.open ("fa_data/simulationTime.dat", std::ofstream::out | std::ofstream::app);
      if (results.is_open()){results << Nsta << "," << Nsaturated << ","
																		 << Simulator::Now().GetSeconds() - initialTimeNewStations  << ","
																		 << Simulator::Now().GetSeconds() << "," 
                                     << algorithm << "," << simSeed << "\n";}                                              
      results.close ();
      Simulator::Stop();
      //Simulator::Stop(Seconds (2));
      NS_LOG_UNCOND("All " << assoc_times.size () << " Stations Conected in " << 
                  Simulator::Now().GetSeconds() -initialTimeNewStations  << " sec");
    }
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CreateAssociatingStas function
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
static void
CreateAssociatingStas(void)
{

  initialTimeNewStations = Simulator::Now ().GetSeconds ();  
  experiment->wifiStaNode.Create (Nsta);

  S1gWifiMacHelper mac = S1gWifiMacHelper::Default ();

  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (experiment->ssid),
               "ActiveProbing", BooleanValue (false),
               "AssocRequestTimeout", TimeValue (MicroSeconds(AssReqTimeout)));

  experiment->staDevice = experiment->wifi.Install (experiment->phy, mac, experiment->wifiStaNode);

  Config::Set ("/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxPacketNumber", UintegerValue(60000));
  Config::Set ("/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxDelay", TimeValue (NanoSeconds (6000000000000)));

  // mobility.
  MobilityHelper mobility;
  

  if (hidden == 2)
    {
      mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
                                       "X", StringValue ("ns3::UniformRandomVariable[Min=1180.0|Max=1200.0]"),
                                       "Y", StringValue ("ns3::UniformRandomVariable[Min=990.0|Max=1010.0]"),
                                       "Z", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    }
  else if (hidden == 1)
    {
      mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
                                       "X", StringValue ("ns3::UniformRandomVariable[Min=800.0|Max=1200.0]"),
                                       "Y", StringValue ("ns3::UniformRandomVariable[Min=800.0|Max=1200.0]"),
                                       "Z", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    }
  else

    {
      mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
                                       "X", StringValue ("ns3::UniformRandomVariable[Min=990.0|Max=1010.0]"),
                                       "Y", StringValue ("ns3::UniformRandomVariable[Min=990.0|Max=1010.0]"),
                                       "Z", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    }
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(experiment->wifiStaNode);

   /* Internet stack*/
  experiment->stack.Install (experiment->wifiStaNode);
  Ipv4InterfaceContainer staNodeInterface;
  staNodeInterface = experiment->address.Assign (experiment->staDevice);

  //trace association
  for (uint16_t i = 0; i < Nsta; i++)
    {
      Ptr<NetDevice> device = experiment->staDevice.Get(i);
      Ptr<WifiNetDevice> sta_wifi = device->GetObject<WifiNetDevice>();
      Ptr<StaWifiMac> sta_mac = DynamicCast<StaWifiMac>(sta_wifi->GetMac());
      sta_mac->TraceConnectWithoutContext ("Assoc", MakeCallback (&nonSaturatedAssoc));

      Mac48Address addr = Mac48Address::ConvertFrom(device->GetAddress ());
      assoc_indices[addr] = i;
    }

  associating_stas_created = true;
  Ptr<WifiNetDevice> ap = experiment->apDevice.Get(0)->GetObject<WifiNetDevice>();
  Ptr<ApWifiMac> apMac = DynamicCast<ApWifiMac>(ap->GetMac());
  apMac->SetAssociatingStasAppear ();
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// saturatedAssoc function
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
static void
saturatedAssoc (Mac48Address address)
{
  if (saturated_times.find(address) != saturated_times.end())
    {
      std::cerr << "Double association from " << address << std::endl;
    }
  saturated_times[address] = Simulator::Now().GetSeconds();
  if (saturated_times.size() == Nsaturated && !associating_stas_created)
    {
      double randomInterval = std::stod (TrafficInterval, nullptr);
      Ptr<UniformRandomVariable> m_rv = CreateObject<UniformRandomVariable> ();
      //UDP flow
      UdpServerHelper myServer (9);
      experiment->serverApp = myServer.Install (experiment->wifiApNode);
      experiment->serverApp.Start (Seconds (0));

      UdpClientHelper myClient (experiment->apNodeInterface.GetAddress (0), 9); //address of remote node
      myClient.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
      myClient.SetAttribute ("Interval", TimeValue (Time (TrafficInterval)));
      myClient.SetAttribute ("PacketSize", UintegerValue (payloadSize));
      for (uint16_t i = 0; i < Nsaturated; i++)
        {
          double randomStart = m_rv->GetValue (0, randomInterval);

          ApplicationContainer clientApp = myClient.Install (experiment->wifiSaturatedStaNode.Get(i));
          clientApp.Start (Seconds (1 + randomStart));
        }

      m_rv = CreateObject<UniformRandomVariable> ();
      uint16_t offset = m_rv->GetValue (1, 5);

      Ptr<WifiNetDevice> ap = experiment->apDevice.Get(0)->GetObject<WifiNetDevice>();
      Ptr<ApWifiMac> apMac = DynamicCast<ApWifiMac>(ap->GetMac());
      apMac->SetSaturatedAssociated ();
      Simulator::Schedule(Seconds(offset), &CreateAssociatingStas);
    }
}
static void
ap_assoc_trace (Ptr<ApWifiMac> apMac)
{
  Ptr<WifiMacQueue> m_queue = apMac->GetQueueInfo();
  Ptr<MacLow> m_low = apMac->GetMlowInfo();
  
  std::ofstream outfile;
  outfile.open(oss.str(), std::ofstream::out | std::ofstream::app);       
  if (outfile.is_open())
    {
      outfile << Simulator::Now().GetSeconds() << "," 
              << apMac-> GetAuthResp () << ","      // authentication Request received = authentication responses sended to the queue
              << m_queue->GetAuthRepNumber () << ","
              << m_low->GetAuthRpS () << ","       
              << m_low->GetAuthRespAck () - prevInfo.at(0) << "," 
              << apMac-> GetAssocResp () << ","   // association Request received = association responses sended to the queue
              << m_queue->GetAssocRepNumber () << ","
              << m_low->GetAssRpS () << ","      
              << m_low->GetAssRespAck () - prevInfo.at(1) << ","
              << apMac->GetAuthenThreshold () << "\n";                           
    }
  prevInfo.at(0) = m_low->GetAuthRespAck ();
  prevInfo.at(1) = m_low->GetAssRespAck ();
  outfile.close();
}
  
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// main function
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int main (int argc, char *argv[])
{

  start = std::chrono::high_resolution_clock::now();
  experiment = new Experiment;
  CommandLine cmd;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Scenario parameters
  cmd.AddValue ("simSeed", "Seed for random generator. Default: 1", simSeed);
  cmd.AddValue ("Nsaturated", "number of saturated stations", Nsaturated);
  cmd.AddValue ("Nsta", "number of connecting stations", Nsta);
  cmd.AddValue ("hidden", "Saturated STAs should be hidden", hidden);
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // optional parameters
  cmd.AddValue ("payloadSize", "Size of payload", payloadSize);
  cmd.AddValue ("BeaconInterval", "Beacon interval time in us", BeaconInterval);
  cmd.AddValue ("DataMode", "Date mode", DataMode);
  cmd.AddValue ("bandWidth", "bandwidth in MHz", bandWidth);
  cmd.AddValue ("UdpInterval", "traffic mode", TrafficInterval);
  cmd.AddValue ("algorithm","choice algorithm", algorithm);
  cmd.AddValue ("AssReqTimeout", "Association request timeout", AssReqTimeout);
  cmd.AddValue ("MinValue", "Minimum connected stations", minvalue);
  cmd.AddValue ("Value", "Delta for algorithm", value);
  cmd.AddValue ("addlifetime", "adding timeout MaxMSDULifetime", life);
  cmd.AddValue ("protocol", "adding choice of centralized or distributed protocol", protocol);
  cmd.AddValue ("authslot", "authenticate slot for distributed protocol", authslot);
  cmd.AddValue ("tiMin", "minimal time interval for DAC", tiMin);
  cmd.AddValue ("tiMax", "maximal time interval for DAC", tiMax);  
  cmd.AddValue ("enableRaw", "RAW should be used", enableRaw);
  cmd.AddValue ("frameCapture", "enable frame capture", dataCapture);
  cmd.AddValue ("preambleCapture", "enable preamble capture", preambleCapture);
  cmd.AddValue ("SinrDiffCapture", "Sinr Diff for packet capture (dB)", sinrDiffCapture);
  cmd.Parse (argc,argv);
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Log for OpenGym interface
  NS_LOG_UNCOND("Ns3 parameters:");
  NS_LOG_UNCOND("--Nsta: " << Nsta);
  NS_LOG_UNCOND("--Nsaturated: " << Nsaturated);
  NS_LOG_UNCOND("--scnType: " << hidden);
  NS_LOG_UNCOND("--seed: " << simSeed);
  NS_LOG_UNCOND("--PID: " << getpid());
  NS_LOG_UNCOND("--Algorithm: " << algorithm);     

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Setup seed
  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (simSeed);
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Create node containers
  experiment->wifiSaturatedStaNode.Create (Nsaturated);
  experiment->wifiApNode.Create (1);
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Channel configuration:
  experiment->channel = YansWifiChannelHelper ();
  hidden == 0 ? experiment->channel.AddPropagationLoss ("ns3::FriisPropagationLossModel") :
  							experiment->channel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel",
								"Exponent", DoubleValue(5) ,"ReferenceLoss", DoubleValue(8.0), "ReferenceDistance", DoubleValue(1.0));
  experiment->channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Configure physical layer
  experiment->phy = YansWifiPhyHelper::Default ();
  experiment->phy.SetErrorRateModel ("ns3::YansErrorRateModel");
  experiment->phy.SetChannel (experiment->channel.Create ());
  experiment->phy.Set ("ShortGuardEnabled", BooleanValue (false));
  experiment->phy.Set ("ChannelWidth", UintegerValue (bandWidth));
  experiment->phy.Set ("EnergyDetectionThreshold", DoubleValue (-116.0));
  experiment->phy.Set ("CcaMode1Threshold", DoubleValue (-119.0));
  experiment->phy.Set ("RxNoiseFigure", DoubleValue (3.0));
  experiment->phy.Set ("TxPowerEnd", DoubleValue (30.0));
  experiment->phy.Set ("TxPowerStart", DoubleValue (30.0));
  experiment->phy.Set ("TxGain", DoubleValue (3.0));
  experiment->phy.Set ("RxGain", DoubleValue (3.0));
  experiment->phy.Set ("LdpcEnabled", BooleanValue (true));
  experiment->phy.Set ("EnablePreambleCapture", BooleanValue (preambleCapture));
  experiment->phy.Set ("EnableDataCapture", BooleanValue (dataCapture));
  experiment->phy.Set ("SinrDiffCapture", DoubleValue (sinrDiffCapture));
 //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 // Configure the remote station manager
  experiment->wifi = WifiHelper::Default ();
  experiment->wifi.SetStandard (WIFI_PHY_STANDARD_80211ah);
  S1gWifiMacHelper mac = S1gWifiMacHelper::Default ();
  experiment->wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
  																					"DataMode", StringValue (DataMode),
                                    				"ControlMode", StringValue (DataMode));
  Config::SetDefault("ns3::DcaTxop::AddTransmitMSDULifetime", UintegerValue (life)); //set to 0
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Configure the access-point MAC layer and install configurations 
  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (experiment->ssid),
               "BeaconInterval", TimeValue (MicroSeconds(BeaconInterval)),
     					 "AuthenProtocol", UintegerValue (protocol),
     					 "Algorithm", UintegerValue (algorithm),
     					 "Nsaturated", UintegerValue (Nsaturated),
     					 "MinValue", UintegerValue (minvalue),
     					 "Value", UintegerValue (value),
               "NAssociating", UintegerValue (Nsta));
  if (enableRaw)
    {
      mac.SetType ("ns3::ApWifiMac",
                   "RawEnabled", BooleanValue (true),
                   "NRawStations", UintegerValue (Nsaturated),
                   "SlotFormat", UintegerValue (1),
                   "SlotCrossBoundary", UintegerValue (1),
                   "SlotDurationCount", UintegerValue (1),
                   "SlotNum", UintegerValue (1));
    }
  experiment->apDevice = experiment->wifi.Install (experiment->phy, mac, experiment->wifiApNode);
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Configure the saturated-stations MAC layer and install configurations
  experiment->phy.Set ("TxPowerEnd", DoubleValue (16.0206));    // 40 mW
  experiment->phy.Set ("TxPowerStart", DoubleValue (16.0206));  // 40 mW
  experiment->phy.Set ("TxGain", DoubleValue (1.0));
  experiment->phy.Set ("RxGain", DoubleValue (1.0));

  mac.SetType ("ns3::StaWifiMac",
                "Ssid", SsidValue (experiment->ssid),
                "ActiveProbing", BooleanValue (false),
    						"AssocRequestTimeout", TimeValue (MicroSeconds(AssReqTimeout)));
  NetDeviceContainer saturatedStaDevice;
  saturatedStaDevice = experiment->wifi.Install (experiment->phy, mac, experiment->wifiSaturatedStaNode);
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Configure queue
  Config::Set ("/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxPacketNumber", UintegerValue(60000));
  Config::Set ("/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxDelay", TimeValue (NanoSeconds (6000000000000)));
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Configure and install the saturated-stations MobilityHelper
  MobilityHelper mobility;
  if (hidden == 2)
    {
      mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
                                         "X", StringValue ("ns3::UniformRandomVariable[Min=800.0|Max=820.0]"),
                                         "Y", StringValue ("ns3::UniformRandomVariable[Min=990.0|Max=1010.0]"),
                                         "Z", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    }
  else if (hidden == 1)
    {
      mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
                                       "X", StringValue ("ns3::UniformRandomVariable[Min=800.0|Max=1200.0]"),
                                       "Y", StringValue ("ns3::UniformRandomVariable[Min=800.0|Max=1200.0]"),
                                       "Z", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    }
  else
    {
      mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
                                       "X", StringValue ("ns3::UniformRandomVariable[Min=990.0|Max=1010.0]"),
                                       "Y", StringValue ("ns3::UniformRandomVariable[Min=990.0|Max=1010.0]"),
                                       "Z", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    }
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(experiment->wifiSaturatedStaNode);
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Configure and install the access-point MobilityHelper
  MobilityHelper mobilityAp;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (1000.0, 1000.0, 0.0));
  mobilityAp.SetPositionAllocator (positionAlloc);
  mobilityAp.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobilityAp.Install(experiment->wifiApNode);
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // install the Internet stack
  experiment->stack.Install (experiment->wifiApNode);
  experiment->stack.Install (experiment->wifiSaturatedStaNode);
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // configure IP addressing 
  experiment->address.SetBase ("192.168.0.0", "255.255.0.0");
  Ipv4InterfaceContainer saturatedStaNodeInterface;
  saturatedStaNodeInterface = experiment->address.Assign (saturatedStaDevice);
  experiment->apNodeInterface = experiment->address.Assign (experiment->apDevice);
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Setup out file 
  oss.str("");
  oss << "fa_data/fa_" << algorithm << "_" << Nsta << "_" << 
              Nsaturated << "_" << simSeed << ".dat";
  std::ofstream outfile;
  outfile.open(oss.str(), std::ofstream::out | std::ofstream::trunc);
  if (outfile.is_open()) 
  { 
    outfile << "SimTime,AuthReq,qAuthRep,AuthRespTx,AuthRespAck,AssReq," <<
                "qAssReq,AssReqTx,AssReqAck,Threshold\n";
  }
  outfile.close();     
  for (int i = 0 ; i < 2; i++){prevInfo.push_back(0);}
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //configure callback to generate an event when a beacon is sending
  Ptr<WifiNetDevice> apWifiDevice = experiment->apDevice.Get(0)->GetObject<WifiNetDevice>();
  Ptr<ApWifiMac> apmac = DynamicCast<ApWifiMac>(apWifiDevice->GetMac());
  apmac->GetBEQueue()->SetAifsn (1);
  apmac->GetVOQueue()->SetAifsn (1);
  apmac->GetVIQueue()->SetAifsn (1);
  apmac->GetBKQueue()->SetAifsn (1);
  apmac->GetDcaTxop()->SetAifsn (1);
  apmac->TraceConnectWithoutContext ("beaconS1G", MakeCallback (&ap_assoc_trace));
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //trace saturated-stations association
  if (Nsaturated > 0 )
    {
      for (uint16_t kk = 0; kk < Nsaturated; kk++)
        {
          Ptr<WifiNetDevice> sta_wifi = saturatedStaDevice.Get(kk)->GetObject<WifiNetDevice>();
          Ptr<StaWifiMac> sta_mac = DynamicCast<StaWifiMac>(sta_wifi->GetMac());          
          sta_mac->GetBEQueue()->SetAifsn (2);
          sta_mac->GetVOQueue()->SetAifsn (2);
          sta_mac->GetVIQueue()->SetAifsn (2);
          sta_mac->GetBKQueue()->SetAifsn (2);
          sta_mac->GetDcaTxop()->SetAifsn (2);
          sta_mac->TraceConnectWithoutContext ("Assoc", MakeCallback (&saturatedAssoc));
        }
    }
  else
    {
      Ptr<WifiNetDevice> apWifiDevice = experiment->apDevice.Get(0)->GetObject<WifiNetDevice>();
      Ptr<ApWifiMac> apmac = DynamicCast<ApWifiMac>(apWifiDevice->GetMac());
      apmac->SetSaturatedAssociated ();
      CreateAssociatingStas();
    }
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Final configurations 
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  PopulateArpCache ();
  Simulator::Run ();
  Simulator::Destroy ();
  delete experiment;

return 0;
}
