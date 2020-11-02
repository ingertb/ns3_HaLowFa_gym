/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006, 2009 INRIA
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
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */


#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/pointer.h"
#include "ns3/string.h"
#include "ap-wifi-mac.h"
#include "mac-low.h"
#include "mac-tx-middle.h"
#include "mac-rx-middle.h"
#include "mgt-headers.h"
#include "msdu-aggregator.h"
#include "amsdu-subframe-header.h"
#include "wifi-phy.h"
#include "ns3/boolean.h"
#include "qos-tag.h"  
#include "dcf-manager.h"  
#include "extension-headers.h" 
#include "ns3/uinteger.h"
#include "wifi-mac-queue.h"
#include "ns3/core-module.h"  
#include "ns3/assert.h"
//*************************************
//HaLow
#include <fstream>
//*************************************

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE("ApWifiMac");

	NS_OBJECT_ENSURE_REGISTERED(ApWifiMac);

	TypeId
		ApWifiMac::GetTypeId(void)
	{
		static TypeId tid = TypeId("ns3::ApWifiMac")
			.SetParent<RegularWifiMac>()
			.SetGroupName("Wifi")
			.AddConstructor<ApWifiMac>()
			.AddAttribute("BeaconInterval", "Delay between two beacons",
				TimeValue(MicroSeconds(102400)),
				MakeTimeAccessor(&ApWifiMac::GetBeaconInterval,
					&ApWifiMac::SetBeaconInterval),
				MakeTimeChecker())
			.AddAttribute("BeaconJitter", "A uniform random variable to cause the initial beacon starting time (after simulation time 0) "
				"to be distributed between 0 and the BeaconInterval.",
				StringValue("ns3::UniformRandomVariable"),
				MakePointerAccessor(&ApWifiMac::m_beaconJitter),
				MakePointerChecker<UniformRandomVariable>())
			.AddAttribute("Outputpath", "output path to store info of sensors and offload stations",
				StringValue("stationfile"),
				MakeStringAccessor(&ApWifiMac::m_outputpath),
				MakeStringChecker())
			.AddAttribute("EnableBeaconJitter", "If beacons are enabled, whether to jitter the initial send event.",
				BooleanValue(false),
				MakeBooleanAccessor(&ApWifiMac::m_enableBeaconJitter),
				MakeBooleanChecker())
			.AddAttribute("BeaconGeneration", "Whether or not beacons are generated.",
				BooleanValue(true),
				MakeBooleanAccessor(&ApWifiMac::SetBeaconGeneration,
					&ApWifiMac::GetBeaconGeneration),
				MakeBooleanChecker())
			.AddAttribute("NRawGroupStas", "Number of stations in one Raw Group",
				UintegerValue(6000),
				MakeUintegerAccessor(&ApWifiMac::GetRawGroupInterval,
					&ApWifiMac::SetRawGroupInterval),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("NRawStations", "Number of total stations support RAW",
				UintegerValue(100),
				MakeUintegerAccessor(&ApWifiMac::GetTotalStaNum,
					&ApWifiMac::SetTotalStaNum),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("SlotFormat", "Slot format",
				UintegerValue(1),
				MakeUintegerAccessor(&ApWifiMac::GetSlotFormat,
					&ApWifiMac::SetSlotFormat),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("SlotCrossBoundary", "cross slot boundary or not",
				UintegerValue(1),
				MakeUintegerAccessor(&ApWifiMac::GetSlotCrossBoundary,
					&ApWifiMac::SetSlotCrossBoundary),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("SlotDurationCount", "slot duration count",
				UintegerValue(1000),
				MakeUintegerAccessor(&ApWifiMac::GetSlotDurationCount,
					&ApWifiMac::SetSlotDurationCount),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("SlotNum", "Number of slot",
				UintegerValue(2),
				MakeUintegerAccessor(&ApWifiMac::GetSlotNum,
					&ApWifiMac::SetSlotNum),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("RPSsetup", "configuration of RAW",
				RPSVectorValue(),
				MakeRPSVectorAccessor(&ApWifiMac::m_rpsset),
				MakeRPSVectorChecker())
			.AddAttribute("RawEnabled", "Whether or not to use the RAW.",
				BooleanValue(false),
				MakeBooleanAccessor(&ApWifiMac::SetRawEnabled,
					&ApWifiMac::GetRawEnabled),
				MakeBooleanChecker())
			.AddAttribute("AuthenProtocol", "choice of centralized (0) or distributed (1) authentication control protocol",
				UintegerValue(0),
				MakeUintegerAccessor(&ApWifiMac::GetProtocol,
					&ApWifiMac::SetProtocol),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("AuthenSlot", "number of authenticate SLOT in BI for distributed protocol",
				UintegerValue(8),
				MakeUintegerAccessor(&ApWifiMac::GetAuthSlot,
					&ApWifiMac::SetAuthSlot),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("TIMin", "value of minimal time interval for DAC",
				UintegerValue(8),
				MakeUintegerAccessor(&ApWifiMac::GetTiMin,
					&ApWifiMac::SetTiMin),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("TIMax", "value of maximal time interval for DAC",
				UintegerValue(255),
				MakeUintegerAccessor(&ApWifiMac::GetTiMax,
					&ApWifiMac::SetTiMax),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("Algorithm", "choice of algorithm of connection of stations",
				UintegerValue(3),
				MakeUintegerAccessor(&ApWifiMac::GetAlgorithm,
					&ApWifiMac::SetAlgorithm),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("Nsaturated", "number of saturated STAs",
				UintegerValue(0),
				MakeUintegerAccessor(&ApWifiMac::GetNsaturated,
					&ApWifiMac::SetNsaturated),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("Value", "Delta for algorithm",
				UintegerValue(60),
				MakeUintegerAccessor(&ApWifiMac::GetValue,
					&ApWifiMac::SetValue),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("MinValue", "the minimum connected station",
				UintegerValue(2),
				MakeUintegerAccessor(&ApWifiMac::GetMinValue,
					&ApWifiMac::SetMinValue),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("LearningThreshold", "Threshold on the queue size for starting the learning phase",
				UintegerValue(0),
				MakeUintegerAccessor(&ApWifiMac::GetLearningThreshold,
					&ApWifiMac::SetLearningThreshold),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("NewGroupThreshold", "Threshold on the queue size for re-learning during the working phase",
				UintegerValue(100),
				MakeUintegerAccessor(&ApWifiMac::GetNewGroupThreshold,
					&ApWifiMac::SetNewGroupThreshold),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("NAssociating", "Number of associating STAs",
				UintegerValue(100),
				MakeUintegerAccessor(&ApWifiMac::GetNAssociating,
					&ApWifiMac::SetNAssociating),
				MakeUintegerChecker<uint32_t>())
			//******************************************
			//HaLow
			.AddTraceSource ("beaconS1G", "Send beacon S1G.",
			                 MakeTraceSourceAccessor (&ApWifiMac::m_beaconS1G),
			                 "ns3::Mac48Address::TracedCallback")
			.AddTraceSource ("AssAlg", "Set association algorithm.",
			                 MakeTraceSourceAccessor (&ApWifiMac::m_setAlg),
			                 "ns3::Mac48Address::TracedCallback")
			;

		return tid;
	}

	ApWifiMac::ApWifiMac()
	{
		NS_LOG_FUNCTION(this);
		m_beaconDca = CreateObject<DcaTxop>();
		m_beaconDca->SetAifsn(1);
		m_beaconDca->SetMinCw(0);
		m_beaconDca->SetMaxCw(0);
		m_beaconDca->SetLow(m_low);
		m_beaconDca->SetManager(m_dcfManager);
		m_beaconDca->SetTxMiddle(m_txMiddle);

		//Let the lower layers know that we are acting as an AP.
		SetTypeOfStation(AP);

		m_enableBeaconGeneration = false;
		AuthenThreshold = 0;
		m_saturatedAssociated = false;
		m_associatingStasAppear = false;
		m_secondWaveAppear = false;

		m_queueLast = 0;
		m_delta = 1;
		m_cac_state = WAIT;
		m_cac_wait_counter = 5;
		m_cac_improve_counter = 5;
		T1 = 1023;

		m_authSlot = 8;

		m_counterAssocSuccess = 0;
		m_counterAuthSuccess = 0;
		t = 0;
		y = 0;

		contAuthResp = 0;
		contAssocResp = 0;

		//m_SlotFormat = 0;
	}

	ApWifiMac::~ApWifiMac()
	{
		NS_LOG_FUNCTION(this);
	}

	void
		ApWifiMac::DoDispose()
	{
		NS_LOG_FUNCTION(this);
		m_beaconDca = 0;
		m_enableBeaconGeneration = false;
		m_beaconEvent.Cancel();
		RegularWifiMac::DoDispose();
	}

	void
		ApWifiMac::SetAddress(Mac48Address address)
	{
		NS_LOG_FUNCTION(this << address);
		//As an AP, our MAC address is also the BSSID. Hence we are
		//overriding this function and setting both in our parent class.
		RegularWifiMac::SetAddress(address);
		RegularWifiMac::SetBssid(address);
	}

	void
		ApWifiMac::SetBeaconGeneration(bool enable)
	{
		NS_LOG_FUNCTION(this << enable);
		if (!enable)
		{
			m_beaconEvent.Cancel();
		}
		else if (enable && !m_enableBeaconGeneration)
		{
			m_beaconEvent = Simulator::ScheduleNow(&ApWifiMac::SendOneBeacon, this);
		}
		m_enableBeaconGeneration = enable;
	}

	bool
		ApWifiMac::GetBeaconGeneration(void) const
	{
		NS_LOG_FUNCTION(this);
		return m_enableBeaconGeneration;
	}

	Time
		ApWifiMac::GetBeaconInterval(void) const
	{
		NS_LOG_FUNCTION(this);
		return m_beaconInterval;
	}

	uint32_t
		ApWifiMac::GetRawGroupInterval(void) const
	{
		NS_LOG_FUNCTION(this);
		return m_rawGroupInterval;
	}

	uint32_t
		ApWifiMac::GetTotalStaNum(void) const
	{
		NS_LOG_FUNCTION(this);
		return m_totalStaNum;
	}

	uint32_t
		ApWifiMac::GetSlotFormat(void) const
	{
		return m_SlotFormat;
	}

	uint32_t
		ApWifiMac::GetSlotCrossBoundary(void) const
	{
		return  m_slotCrossBoundary;
	}

	uint32_t
		ApWifiMac::GetAlgorithm(void) const
	{
		return algorithm;
	}

	uint32_t
		ApWifiMac::GetNsaturated(void) const
	{
		return Nsaturated;
	}

	uint32_t
		ApWifiMac::GetProtocol(void) const
	{
		return m_protocol;
	}

	uint32_t
		ApWifiMac::GetAuthSlot(void) const
	{
		return m_authSlot;
	}

	uint32_t
		ApWifiMac::GetTiMin(void) const
	{
		return m_tiMin;
	}

	uint32_t
		ApWifiMac::GetTiMax(void) const
	{
		return m_tiMax;
	}

	uint32_t
		ApWifiMac::GetValue(void) const
	{
		return value;
	}

	uint32_t
		ApWifiMac::GetMinValue(void) const
	{
		return minvalue;
	}


	uint32_t
		ApWifiMac::GetSlotDurationCount(void) const
	{
		return m_slotDurationCount;
	}

	uint32_t
		ApWifiMac::GetSlotNum(void) const
	{
		return m_slotNum;
	}

	bool
		ApWifiMac::GetRawEnabled(void) const
	{
		return m_rawEnabled;
	}

	uint32_t
		ApWifiMac::GetNAssociating(void) const
	{
		return m_nAssociating;
	}

	uint32_t
		ApWifiMac::GetLearningThreshold(void) const
	{
		return m_learningThreshold;
	}

	uint32_t
		ApWifiMac::GetNewGroupThreshold(void) const
	{
		return m_newGroupThreshold;
	}

	void
		ApWifiMac::SetWifiRemoteStationManager(Ptr<WifiRemoteStationManager> stationManager)
	{
		NS_LOG_FUNCTION(this << stationManager);
		m_beaconDca->SetWifiRemoteStationManager(stationManager);
		RegularWifiMac::SetWifiRemoteStationManager(stationManager);
	}

	void
		ApWifiMac::SetLinkUpCallback(Callback<void> linkUp)
	{
		NS_LOG_FUNCTION(this << &linkUp);
		RegularWifiMac::SetLinkUpCallback(linkUp);

		//The approach taken here is that, from the point of view of an AP,
		//the link is always up, so we immediately invoke the callback if
		//one is set
		linkUp();
	}

	void
		ApWifiMac::SetBeaconInterval(Time interval)
	{
		NS_LOG_FUNCTION(this << interval);
		if ((interval.GetMicroSeconds() % 1024) != 0)
		{
			NS_LOG_WARN("beacon interval should be multiple of 1024us (802.11 time unit), see IEEE Std. 802.11-2012");
		}
		m_beaconInterval = interval;
	}

	void
		ApWifiMac::SetRawGroupInterval(uint32_t interval)
	{
		NS_LOG_FUNCTION(this << interval);
		m_rawGroupInterval = interval;
	}

	void
		ApWifiMac::SetTotalStaNum(uint32_t num)
	{
		NS_LOG_FUNCTION(this << num);
		m_totalStaNum = num;
		//m_S1gRawCtr.RAWGroupping (m_totalStaNum, 1, m_beaconInterval.GetMicroSeconds ());
		//m_S1gRawCtr.configureRAW ();

	}

	void
		ApWifiMac::SetSlotFormat(uint32_t format)
	{
		NS_ASSERT(format <= 1);
		m_SlotFormat = format;
	}

	void
		ApWifiMac::SetSlotCrossBoundary(uint32_t cross)
	{
		NS_ASSERT(cross <= 1);
		m_slotCrossBoundary = cross;
	}

	void
		ApWifiMac::SetSlotDurationCount(uint32_t count)
	{
		NS_ASSERT((!m_SlotFormat & (count < 256)) || (m_SlotFormat & (count < 2048)));
		m_slotDurationCount = count;
	}

	void
		ApWifiMac::SetSlotNum(uint32_t count)
	{
		NS_ASSERT((!m_SlotFormat & (count < 64)) || (m_SlotFormat & (count < 8)));
		m_slotNum = count;
	}

	void
		ApWifiMac::SetAlgorithm(uint32_t alg)
	{
		NS_ASSERT((alg == 0) || (alg == 1) || (alg == 2) || (alg == 3) || (alg == 4) || (alg == 5) || (alg == 6) || (alg == 7) || (alg == 8) || (alg == 9) || (alg == 10) || (alg == 100) || (alg == 101));
		algorithm = alg;
	}

	void
		ApWifiMac::SetNsaturated(uint32_t Nsatu)
	{
		Nsaturated = Nsatu;
	}

	void
		ApWifiMac::SetProtocol(uint32_t prot)
	{
		NS_ASSERT((prot == 0) || (prot == 1));
		m_protocol = prot;
	}

	void
		ApWifiMac::SetAuthSlot(uint32_t slot)
	{
		m_authSlot = slot;
	}

	void
		ApWifiMac::SetTiMin(uint32_t timin)
	{
		m_tiMin = timin;
	}

	void
		ApWifiMac::SetTiMax(uint32_t timax)
	{
		m_tiMax = timax;
	}

	void
		ApWifiMac::SetValue(uint32_t val)
	{
		value = val;
	}

	void
		ApWifiMac::SetMinValue(uint32_t minval)
	{
		minvalue = minval;
	}

	void
		ApWifiMac::SetRawEnabled(bool enabled)
	{
		m_rawEnabled = enabled;
	}

	void
		ApWifiMac::SetNAssociating(uint32_t num)
	{
		m_nAssociating = num;
	}

	void
		ApWifiMac::SetSaturatedAssociated(void)
	{
		m_saturatedAssociated = true;
	}

	void
		ApWifiMac::SetAssociatingStasAppear(void)
	{
		m_associatingStasAppear = true;
	}

	void
		ApWifiMac::SetSecondWaveAppear(void)
	{
		m_secondWaveAppear = true;
	}

	void
		ApWifiMac::SetLearningThreshold(uint32_t t)
	{
		m_learningThreshold = t;
	}

	void
		ApWifiMac::SetNewGroupThreshold(uint32_t t)
	{
		m_newGroupThreshold = t;
	}

	void
		ApWifiMac::StartBeaconing(void)
	{
		NS_LOG_FUNCTION(this);
		SendOneBeacon();
	}

	int64_t
		ApWifiMac::AssignStreams(int64_t stream)
	{
		NS_LOG_FUNCTION(this << stream);
		m_beaconJitter->SetStream(stream);
		return 1;
	}

	void
		ApWifiMac::ForwardDown(Ptr<const Packet> packet, Mac48Address from,
			Mac48Address to)
	{
		NS_LOG_FUNCTION(this << packet << from << to);
		//If we are not a QoS AP then we definitely want to use AC_BE to
		//transmit the packet. A TID of zero will map to AC_BE (through \c
		//QosUtilsMapTidToAc()), so we use that as our default here.
		uint8_t tid = 0;

		//If we are a QoS AP then we attempt to get a TID for this packet
		if (m_qosSupported)
		{
			tid = QosUtilsGetTidForPacket(packet);
			//Any value greater than 7 is invalid and likely indicates that
			//the packet had no QoS tag, so we revert to zero, which'll
			//mean that AC_BE is used.
			if (tid > 7)
			{
				tid = 0;
			}
		}

		ForwardDown(packet, from, to, tid);
	}

	void
		ApWifiMac::ForwardDown(Ptr<const Packet> packet, Mac48Address from,
			Mac48Address to, uint8_t tid)
	{
		NS_LOG_FUNCTION(this << packet << from << to << static_cast<uint32_t> (tid));
		WifiMacHeader hdr;

		//For now, an AP that supports QoS does not support non-QoS
		//associations, and vice versa. In future the AP model should
		//support simultaneously associated QoS and non-QoS STAs, at which
		//point there will need to be per-association QoS state maintained
		//by the association state machine, and consulted here.
		if (m_qosSupported)
		{
			hdr.SetType(WIFI_MAC_QOSDATA);
			hdr.SetQosAckPolicy(WifiMacHeader::NORMAL_ACK);
			hdr.SetQosNoEosp();
			hdr.SetQosNoAmsdu();
			//Transmission of multiple frames in the same TXOP is not
			//supported for now
			hdr.SetQosTxopLimit(0);
			//Fill in the QoS control field in the MAC header
			hdr.SetQosTid(tid);
		}
		else
		{
			hdr.SetTypeData();
		}

		if (m_htSupported)
		{
			hdr.SetNoOrder();
		}
		hdr.SetAddr1(to);
		hdr.SetAddr2(GetAddress());
		hdr.SetAddr3(from);
		hdr.SetDsFrom();
		hdr.SetDsNotTo();

		if (m_qosSupported)
		{
			//Sanity check that the TID is valid
			NS_ASSERT(tid < 8);
			m_edca[QosUtilsMapTidToAc(tid)]->Queue(packet, hdr);
		}
		else
		{
			m_dca->Queue(packet, hdr);
		}
	}

	void
		ApWifiMac::Enqueue(Ptr<const Packet> packet, Mac48Address to, Mac48Address from)
	{
		NS_LOG_FUNCTION(this << packet << to << from);
		if (to.IsBroadcast() || m_stationManager->IsAssociated(to))
		{
			ForwardDown(packet, from, to);
		}
	}

	void
		ApWifiMac::Enqueue(Ptr<const Packet> packet, Mac48Address to)
	{
		NS_LOG_FUNCTION(this << packet << to);
		//We're sending this packet with a from address that is our own. We
		//get that address from the lower MAC and make use of the
		//from-spoofing Enqueue() method to avoid duplicated code.
		Enqueue(packet, to, m_low->GetAddress());
	}

	bool
		ApWifiMac::SupportsSendFrom(void) const
	{
		NS_LOG_FUNCTION(this);
		return true;
	}

	SupportedRates
		ApWifiMac::GetSupportedRates(void) const
	{
		NS_LOG_FUNCTION(this);
		SupportedRates rates;
		//If it is an HT-AP then add the BSSMembershipSelectorSet
		//which only includes 127 for HT now. The standard says that the BSSMembershipSelectorSet
		//must have its MSB set to 1 (must be treated as a Basic Rate)
		//Also the standard mentioned that at leat 1 element should be included in the SupportedRates the rest can be in the ExtendedSupportedRates
		if (m_htSupported)
		{
			for (uint32_t i = 0; i < m_phy->GetNBssMembershipSelectors(); i++)
			{
				rates.SetBasicRate(m_phy->GetBssMembershipSelector(i));
			}
		}
		//Send the set of supported rates and make sure that we indicate
		//the Basic Rate set in this set of supported rates.
	   // NS_LOG_LOGIC ("ApWifiMac::GetSupportedRates  1 " ); //for test
		for (uint32_t i = 0; i < m_phy->GetNModes(); i++)
		{
			WifiMode mode = m_phy->GetMode(i);
			rates.AddSupportedRate(mode.GetDataRate());
			//Add rates that are part of the BSSBasicRateSet (manufacturer dependent!)
			//here we choose to add the mandatory rates to the BSSBasicRateSet,
			//exept for 802.11b where we assume that only the two lowest mandatory rates are part of the BSSBasicRateSet
			if (mode.IsMandatory()
				&& ((mode.GetModulationClass() != WIFI_MOD_CLASS_DSSS) || mode == WifiPhy::GetDsssRate1Mbps() || mode == WifiPhy::GetDsssRate2Mbps()))
			{
				m_stationManager->AddBasicMode(mode);
			}
		}
		// NS_LOG_LOGIC ("ApWifiMac::GetSupportedRates  2 " ); //for test
		//set the basic rates
		for (uint32_t j = 0; j < m_stationManager->GetNBasicModes(); j++)
		{
			WifiMode mode = m_stationManager->GetBasicMode(j);
			rates.SetBasicRate(mode.GetDataRate());
		}
		//NS_LOG_LOGIC ("ApWifiMac::GetSupportedRates   " ); //for test
		return rates;
	}

	HtCapabilities
		ApWifiMac::GetHtCapabilities(void) const
	{
		HtCapabilities capabilities;
		capabilities.SetHtSupported(1);
		capabilities.SetLdpc(m_phy->GetLdpc());
		capabilities.SetShortGuardInterval20(m_phy->GetGuardInterval());
		capabilities.SetGreenfield(m_phy->GetGreenfield());
		for (uint8_t i = 0; i < m_phy->GetNMcs(); i++)
		{
			capabilities.SetRxMcsBitmask(m_phy->GetMcs(i));
		}
		return capabilities;
	}

	void
		ApWifiMac::SendProbeResp(Mac48Address to)
	{
		NS_LOG_FUNCTION(this << to);
		WifiMacHeader hdr;
		hdr.SetProbeResp();
		hdr.SetAddr1(to);
		hdr.SetAddr2(GetAddress());
		hdr.SetAddr3(GetAddress());
		hdr.SetDsNotFrom();
		hdr.SetDsNotTo();
		Ptr<Packet> packet = Create<Packet>();
		MgtProbeResponseHeader probe;
		probe.SetSsid(GetSsid());
		probe.SetSupportedRates(GetSupportedRates());
		probe.SetBeaconIntervalUs(m_beaconInterval.GetMicroSeconds());
		if (m_htSupported)
		{
			probe.SetHtCapabilities(GetHtCapabilities());
			hdr.SetNoOrder();
		}
		packet->AddHeader(probe);

		//The standard is not clear on the correct queue for management
		//frames if we are a QoS AP. The approach taken here is to always
		//use the DCF for these regardless of whether we have a QoS
		//association or not.
		m_dca->Queue(packet, hdr);
	}

	void
		ApWifiMac::SendAuthResp(Mac48Address to, bool success)
	{
		NS_LOG_FUNCTION(this << to << success);
		WifiMacHeader hdr;
		hdr.SetAuthFrame();
		hdr.SetAddr1(to);
		hdr.SetAddr2(GetAddress());
		hdr.SetAddr3(GetAddress());
		hdr.SetDsNotFrom();
		hdr.SetDsNotTo();
		Ptr<Packet> packet = Create<Packet>();
		MgtAuthFrameHeader auth;

		StatusCode code;
		if (success)
		{
			code.SetSuccess();
		}
		else
		{
			code.SetFailure();
		}
		auth.SetAuthAlgorithmNumber(0);
		auth.SetAuthTransactionSeqNumber(2);
		auth.SetStatusCode(code);

		packet->AddHeader(auth);
		m_dca->Queue(packet, hdr);
		contAuthResp ++;
	}

	void
		ApWifiMac::SendAssocResp(Mac48Address to, bool success, uint8_t staType)
	{
		NS_LOG_FUNCTION(this << to << success);
		WifiMacHeader hdr;
		hdr.SetAssocResp();
		hdr.SetAddr1(to);
		hdr.SetAddr2(GetAddress());
		hdr.SetAddr3(GetAddress());
		hdr.SetDsNotFrom();
		hdr.SetDsNotTo();
		Ptr<Packet> packet = Create<Packet>();
		MgtAssocResponseHeader assoc;

		uint8_t mac[6];
		to.CopyTo(mac);
		uint8_t aid_l = mac[5];
		uint8_t aid_h = mac[4] & 0x1f;
		uint16_t aid = (aid_h << 8) | (aid_l << 0); //assign mac address as AID
		assoc.SetAID(aid); //
		StatusCode code;
		if (success)
		{
			code.SetSuccess();
		}
		else
		{
			code.SetFailure();
		}
		assoc.SetSupportedRates(GetSupportedRates());
		assoc.SetStatusCode(code);

		if (m_htSupported)
		{
			assoc.SetHtCapabilities(GetHtCapabilities());
			hdr.SetNoOrder();
		}


		if (m_s1gSupported && success)
		{
			m_tau = Simulator::Now() - m_lastBeacon;
			//assign AID based on station type, to do.
			if (staType == 1)
			{
				for (std::vector<uint16_t>::iterator it = m_sensorList.begin(); it != m_sensorList.end(); it++)
				{
					if (*it == aid)
						goto Addheader;
				}
				m_sensorList.push_back(aid);
			}
			else if (staType == 2)
			{
				for (std::vector<uint16_t>::iterator it = m_OffloadList.begin(); it != m_OffloadList.end(); it++)
				{
					if (*it == aid)
						goto Addheader;
				}
				m_OffloadList.push_back(aid);
			}
		}
	Addheader:
		packet->AddHeader(assoc);

		//The standard is not clear on the correct queue for management
		//frames if we are a QoS AP. The approach taken here is to always
		//use the DCF for these regardless of whether we have a QoS
		//association or not.
		m_dca->Queue(packet, hdr);
		contAssocResp ++;
	}

	void
		ApWifiMac::SendOneBeacon(void)
	{
		NS_LOG_FUNCTION(this);
		WifiMacHeader hdr;

		if (!m_s1gSupported)
			{
				m_receivedAid.clear(); //release storage
				hdr.SetBeacon();
				hdr.SetAddr1(Mac48Address::GetBroadcast());
				hdr.SetAddr2(GetAddress());
				hdr.SetAddr3(GetAddress());
				hdr.SetDsNotFrom();
				hdr.SetDsNotTo();
				Ptr<Packet> packet = Create<Packet>();
				MgtBeaconHeader beacon;
				beacon.SetSsid(GetSsid());
				beacon.SetSupportedRates(GetSupportedRates());
				beacon.SetBeaconIntervalUs(m_beaconInterval.GetMicroSeconds());
				if (m_htSupported)
					{
						beacon.SetHtCapabilities(GetHtCapabilities());
						hdr.SetNoOrder();
					}
				packet->AddHeader(beacon);
				//The beacon has it's own special queue, so we load it in there
				m_beaconDca->Queue(packet, hdr);
				m_beaconEvent = Simulator::Schedule(m_beaconInterval, &ApWifiMac::SendOneBeacon, this);
			}		
		else
			{				
				hdr.SetS1gBeacon();
				hdr.SetAddr1(Mac48Address::GetBroadcast());
				hdr.SetAddr2(GetAddress()); // for debug, not accordance with draft, need change
				hdr.SetAddr3(GetAddress()); // for debug
				Ptr<Packet> packet = Create<Packet>();
				S1gBeaconHeader beacon;
				S1gBeaconCompatibility compatibility;
				compatibility.SetBeaconInterval(m_beaconInterval.GetMicroSeconds());
				beacon.SetBeaconCompatibility(compatibility);

				uint32_t q;
				uint32_t m = m_low->GetAssRespAck();
				uint32_t z = m_low->GetAuthRespAck();	

				if (m_rawEnabled)
					{
						RPS* m_rps;
						static uint16_t RpsIndex = 0;
						if (RpsIndex < m_rpsset.rpsset.size())
							{
								m_rps = m_rpsset.rpsset.at(RpsIndex);
								NS_LOG_DEBUG("< RpsIndex =" << RpsIndex);
								RpsIndex++;
							}
						else
							{
								m_rps = m_rpsset.rpsset.at(0);
								NS_LOG_DEBUG("RpsIndex =" << RpsIndex);
								RpsIndex = 1;
							}
						beacon.SetRPS(*m_rps);
					}
				AuthenticationCtrl  AuthenCtrl;
				if (m_protocol == 0)
					{

						AuthenCtrl.SetControlType(false); //centralized
						Ptr<WifiMacQueue> MgtQueue = m_dca->GetQueue();
						uint32_t MgtQueueSize = MgtQueue->GetSize();
						q1 = MgtQueue->GetAuthRepNumber();
						q2 = MgtQueue->GetAssocRepNumber();
						q = MgtQueueSize;

						//NS_LOG_UNCOND("Queue info : |" << q1 << "|" << q2 << "|" << q << "/" << q1+q2 << "|");

						m_setAlg (this); 

						switch (algorithm)
							{
								case 1:								
									Algorithm_1 (MgtQueueSize);
									break;
								case 2:
									Algorithm_2 (MgtQueueSize);
									break;
								case 3:
									Algorithm_3 (MgtQueueSize);
									break;
								case 4:
									AuthenThreshold = 1023;
									break;
								case 5:
									Algorithm_5 (MgtQueueSize);
									break;
								case 6:
									Algorithm_6 (MgtQueueSize);
									break;
								case 7:
									Algorithm_7 (MgtQueueSize);
									break;
								case 8:									
									Algorithm_8 (MgtQueueSize);
									break;
								case 9:									
									Algorithm_9 (MgtQueueSize);
									break;
								case 10:
									Algorithm_10 (MgtQueueSize);
									break;
								case 100:
										m_beaconS1G (this);
										break;
								case 101:
										m_beaconS1G (this);
										Algorithm_10 (MgtQueueSize);
										break;
								case 0:
								default:
									Algorithm_0 (MgtQueueSize);
									break;
							}
						AuthenThreshold >= 1023 ? AuthenThreshold = 1023 : AuthenThreshold <= 0 ? AuthenThreshold = 0 : AuthenThreshold = AuthenThreshold;
						AuthenCtrl.SetThreshold(AuthenThreshold); //centralized
					}
				else
					{
						AuthenCtrl.SetControlType(true); //distributed
						AuthenCtrl.SetMinInterval(m_tiMin);
						AuthenCtrl.SetMaxInterval(m_tiMax);
						AuthenCtrl.SetSlotDuration(m_authSlot);
					}
				beacon.SetAuthCtrl(AuthenCtrl);
				packet->AddHeader(beacon);
				m_beaconDca->Queue(packet, hdr);

				m_beaconEvent = Simulator::Schedule(m_beaconInterval, &ApWifiMac::SendOneBeacon, this);
				m_queueLast = q;
				int NonSaturatedAssociatedSum = m_low->GetAssRespAck() - Nsaturated;
				if (NonSaturatedAssociatedSum > 8000 || NonSaturatedAssociatedSum < 0) {NonSaturatedAssociatedSum = 0;}
				
				if (algorithm < 11)
					{
						NS_LOG_UNCOND("SimTime "
						          << "Asctd+  "
						          << "Authcd+ "
						          << "STAsum  "
						          << "AssReq  "
						          << "AuthReq "
						          << "QAuth   "
						          << "QAss    "
						          << "Queue   "
						          << "Delta   "   // it means NEXT DELTA during learning face
						          << "Thrshld "
						          << "State   "
						          << "Improve "
						          << "WaitCnt "
						          << "ImprCnt "
						          << "State   ");

						NS_LOG_UNCOND( Simulator::Now ().GetSeconds () << "\t"
						          << m-t << "\t"
						          << z - y << "\t"
						          << NonSaturatedAssociatedSum << "\t"
						          << m_counterAssocSuccess << "\t"
						          << m_counterAuthSuccess << "\t"
						          // << MgtQueue->GetAuthRepNumber () << '\t'
						          // << MgtQueue->GetAssocRepNumber () << '\t'
						          << q1 << "\t"
						          << q2 << "\t"
						          << m_dca->GetQueue ()->GetSize () << "\t"
						          << m_delta << "\t"
						          << AuthenThreshold << "\t"
						          << m_cac_state << "\t"
						          << m_cac_improve << "\t"
						          << m_cac_wait_counter << "\t"
						          << m_cac_improve_counter << "\t"
						          << m_cac_state << "\t");
						          // m = m_low->GetAss();
						          // z = m_low->GetAuth();
						m_beaconS1G (this);
					}

				
				r1 = m_counterAuthSuccess;         // the number of successful AuthReqs
				r2 = m_counterAssocSuccess;        // the number of successful AReqs
				a1 = z - y;                        // the number of successful AuthReps  //false
				a2 = m - t;                        // the number of successful AReps
				m_lastBeacon = Simulator::Now();
				m_tau = MilliSeconds(0);
				//  sent_auth = 0;                     // alternative for counting a1
				//  sent_ass = 0;                      // alternative for counting a2
				m_counterAssocSuccess = 0;
				m_counterAuthSuccess = 0;
				t = m;
				y = z;
			}
	}

	void
		ApWifiMac::TxOk(const WifiMacHeader& hdr)
	{
		NS_LOG_FUNCTION(this);
		RegularWifiMac::TxOk(hdr);

		if (hdr.IsAuthentication())
		{
			sent_auth++;
		}

		else if (hdr.IsAssocResp()
			&& m_stationManager->IsWaitAssocTxOk(hdr.GetAddr1()))
		{
			NS_LOG_DEBUG("associated with sta=" << hdr.GetAddr1());
			m_stationManager->RecordGotAssocTxOk(hdr.GetAddr1());
			sent_ass++;
		}

	}

	void
		ApWifiMac::TxFailed(const WifiMacHeader& hdr)
	{
		NS_LOG_FUNCTION(this);
		RegularWifiMac::TxFailed(hdr);

		if (hdr.IsAuthentication())
		{
			sent_auth++;
		}

		else if (hdr.IsAssocResp()
			&& m_stationManager->IsWaitAssocTxOk(hdr.GetAddr1()))
		{
			NS_LOG_DEBUG("assoc failed with sta=" << hdr.GetAddr1());
			m_stationManager->RecordGotAssocTxFailed(hdr.GetAddr1());
			sent_ass++;
		}
	}

	void
		ApWifiMac::Receive(Ptr<Packet> packet, const WifiMacHeader* hdr)
	{
		NS_LOG_FUNCTION(this << packet << hdr);
		Mac48Address from = hdr->GetAddr2();

		if (hdr->IsData())
		{
			Mac48Address bssid = hdr->GetAddr1();
			if (!hdr->IsFromDs()
				&& hdr->IsToDs()
				&& bssid == GetAddress()
				&& m_stationManager->IsAssociated(from))
			{
				Mac48Address to = hdr->GetAddr3();
				if (to == GetAddress())
				{
					NS_LOG_DEBUG("frame for me from=" << from);
					if (hdr->IsQosData())
					{
						if (hdr->IsQosAmsdu())
						{
							NS_LOG_DEBUG("Received A-MSDU from=" << from << ", size=" << packet->GetSize());
							DeaggregateAmsduAndForward(packet, hdr);
							packet = 0;
						}
						else
						{
							ForwardUp(packet, from, bssid);
						}
					}
					else
					{
						ForwardUp(packet, from, bssid);
					}
					uint8_t mac[6];
					from.CopyTo(mac);
					uint8_t aid_l = mac[5];
					uint8_t aid_h = mac[4] & 0x1f;
					uint16_t aid = (aid_h << 8) | (aid_l << 0); //assign mac address as AID
					m_receivedAid.push_back(aid); //to change
				}
				else if (to.IsGroup()
					|| m_stationManager->IsAssociated(to))
				{
					NS_LOG_DEBUG("forwarding frame from=" << from << ", to=" << to);
					Ptr<Packet> copy = packet->Copy();

					//If the frame we are forwarding is of type QoS Data,
					//then we need to preserve the UP in the QoS control
					//header...
					if (hdr->IsQosData())
					{
						ForwardDown(packet, from, to, hdr->GetQosTid());
					}
					else
					{
						ForwardDown(packet, from, to);
					}
					ForwardUp(copy, from, to);
				}
				else
				{
					ForwardUp(packet, from, to);
				}
			}
			else if (hdr->IsFromDs()
				&& hdr->IsToDs())
			{
				//this is an AP-to-AP frame
				//we ignore for now.
				NotifyRxDrop(packet);
			}
			else
			{
				//we can ignore these frames since
				//they are not targeted at the AP
				NotifyRxDrop(packet);
			}
			return;
		}
		else if (hdr->IsMgt())
		{
			if (hdr->IsProbeReq())
			{
				NS_ASSERT(hdr->GetAddr1().IsBroadcast());
				SendProbeResp(from);
				return;
			}
			else if (hdr->GetAddr1() == GetAddress())
			{
				if (hdr->IsAssocReq())
				{
					m_counterAssocSuccess++;
					//NS_LOG_LOGIC ("Received AssocReq "); // for test
				   //first, verify that the the station's supported
				   //rate set is compatible with our Basic Rate set
					MgtAssocRequestHeader assocReq;

					packet->RemoveHeader(assocReq);

					SupportedRates rates = assocReq.GetSupportedRates();
					bool problem = false;
					for (uint32_t i = 0; i < m_stationManager->GetNBasicModes(); i++)
					{
						WifiMode mode = m_stationManager->GetBasicMode(i);
						if (!rates.IsSupportedRate(mode.GetDataRate()))
						{
							problem = true;
							break;
						}
					}

					if (m_htSupported)
					{
						//check that the STA supports all MCSs in Basic MCS Set
						HtCapabilities htcapabilities = assocReq.GetHtCapabilities();
						for (uint32_t i = 0; i < m_stationManager->GetNBasicMcs(); i++)
						{
							uint8_t mcs = m_stationManager->GetBasicMcs(i);
							if (!htcapabilities.IsSupportedMcs(mcs))
							{
								problem = true;
								break;
							}
						}

					}


					if (problem)
					{
						//One of the Basic Rate set mode is not
						//supported by the station. So, we return an assoc
						//response with an error status.
						SendAssocResp(hdr->GetAddr2(), false, 0);
					}
					else
					{
						//station supports all rates in Basic Rate Set.
						//record all its supported modes in its associated WifiRemoteStation
						for (uint32_t j = 0; j < m_phy->GetNModes(); j++)
						{
							WifiMode mode = m_phy->GetMode(j);
							if (rates.IsSupportedRate(mode.GetDataRate()))
							{
								m_stationManager->AddSupportedMode(from, mode);
							}
						}
						if (m_htSupported)
						{
							HtCapabilities htcapabilities = assocReq.GetHtCapabilities();
							m_stationManager->AddStationHtCapabilities(from, htcapabilities);
							for (uint32_t j = 0; j < m_phy->GetNMcs(); j++)
							{
								uint8_t mcs = m_phy->GetMcs(j);
								if (htcapabilities.IsSupportedMcs(mcs))
								{
									m_stationManager->AddSupportedMcs(from, mcs);
								}
							}
						}

						m_stationManager->RecordWaitAssocTxOk(from);

						if (m_s1gSupported)
						{
							S1gCapabilities s1gcapabilities = assocReq.GetS1gCapabilities();
							uint8_t sta_type = s1gcapabilities.GetStaType();
							SendAssocResp(hdr->GetAddr2(), true, sta_type);
						}
						else
						{
							//send assoc response with success status.
							SendAssocResp(hdr->GetAddr2(), true, 0);
						}

					}
					return;
				}
				else if (hdr->IsDisassociation())
				{
					m_stationManager->RecordDisassociated(from);
					uint8_t mac[6];
					from.CopyTo(mac);
					uint8_t aid_l = mac[5];
					uint8_t aid_h = mac[4] & 0x1f;
					uint16_t aid = (aid_h << 8) | (aid_l << 0);

					for (std::vector<uint16_t>::iterator it = m_sensorList.begin(); it != m_sensorList.end(); it++)
					{
						if (*it == aid)
						{
							m_sensorList.erase(it); //remove from association list
							break;
						}
					}
					return;
				}
				else if (hdr->IsAuthentication())
				{
					MgtAuthFrameHeader auth;
					packet->RemoveHeader(auth);
					m_counterAuthSuccess++;
					SendAuthResp(hdr->GetAddr2(), true);
					return;
				}
			}
		}

		//Invoke the receive handler of our parent class to deal with any
		//other frames. Specifically, this will handle Block Ack-related
		//Management Action frames.
		RegularWifiMac::Receive(packet, hdr);
	}

	void
		ApWifiMac::DeaggregateAmsduAndForward(Ptr<Packet> aggregatedPacket,
			const WifiMacHeader* hdr)
	{
		NS_LOG_FUNCTION(this << aggregatedPacket << hdr);
		MsduAggregator::DeaggregatedMsdus packets =
			MsduAggregator::Deaggregate(aggregatedPacket);

		for (MsduAggregator::DeaggregatedMsdusCI i = packets.begin();
			i != packets.end(); ++i)
		{
			if ((*i).second.GetDestinationAddr() == GetAddress())
			{
				ForwardUp((*i).first, (*i).second.GetSourceAddr(),
					(*i).second.GetDestinationAddr());
			}
			else
			{
				Mac48Address from = (*i).second.GetSourceAddr();
				Mac48Address to = (*i).second.GetDestinationAddr();
				NS_LOG_DEBUG("forwarding QoS frame from=" << from << ", to=" << to);
				ForwardDown((*i).first, from, to, hdr->GetQosTid());
			}
		}
	}

	void
		ApWifiMac::DoInitialize(void)
	{
		NS_LOG_FUNCTION(this);
		m_beaconDca->Initialize();
		m_beaconEvent.Cancel();
		if (m_enableBeaconGeneration)
		{
			if (m_enableBeaconJitter)
			{
				int64_t jitter = m_beaconJitter->GetValue(0, m_beaconInterval.GetMicroSeconds());
				NS_LOG_DEBUG("Scheduling initial beacon for access point " << GetAddress() << " at time " << jitter << " microseconds");
				m_beaconEvent = Simulator::Schedule(MicroSeconds(jitter), &ApWifiMac::SendOneBeacon, this);
			}
			else
			{
				NS_LOG_DEBUG("Scheduling initial beacon for access point " << GetAddress() << " at time 0");
				m_beaconEvent = Simulator::ScheduleNow(&ApWifiMac::SendOneBeacon, this);
			}
		}
		RegularWifiMac::DoInitialize();
	}



	//************************************
	//Halow

	void 
  ApWifiMac::SetAuthenThreshold (uint32_t AuthThr)
  {
  	AuthenThreshold = AuthThr;
  }

	uint32_t 
  ApWifiMac::GetAuthenThreshold (void)
  {
  	return AuthenThreshold;
  }

	Ptr<WifiMacQueue>
	ApWifiMac::GetQueueInfo(void)
	{
	 	return m_dca->GetQueue();
	}

	Ptr<MacLow>
	ApWifiMac::GetMlowInfo(void)
	{
	 	return m_low;
	}

	uint32_t
	ApWifiMac::GetAuthResp(void)
	{
		uint32_t tmp = contAuthResp;
	  contAuthResp = 0;
	  return tmp;
	}

	uint32_t
	ApWifiMac::GetAssocResp(void)
	{
		uint32_t tmp = contAssocResp;
	  contAssocResp = 0;
	  return tmp;
	}	

	void 
	ApWifiMac::Algorithm_0 (uint32_t &MgtQueueSize)
	{
		if (MgtQueueSize < 10)
		{
			AuthenThreshold += value;
		}
		else
		{
			if (AuthenThreshold > value)
			{
				AuthenThreshold -= value;
			}
		}
	}
	void 
	ApWifiMac::Algorithm_1 (uint32_t &MgtQueueSize)
	{
		if (algorithm == 101)
		{
			m_beaconS1G (this);
		}
		if (m_cac_state == WAIT)
		{
			if (MgtQueueSize != 0)
			{
				AuthenThreshold = 0;
				m_delta = 1;
				m_cac_improve = false;
				m_cac_state = LEARN;
			}
			else
			{
				AuthenThreshold = 1023;
			}
		}
		else if (m_cac_state == LEARN)
		{
			if (MgtQueueSize == 0)
			{
				if (AuthenThreshold == 1023)
				{
					while (m_cac_history.empty() == false)
					{
						m_cac_history.pop();
					}
					m_cac_state = WAIT;
				}
				else
				{
					if (m_cac_history.empty() == true || m_cac_history.top().threshold > AuthenThreshold + m_delta)
					{
						AuthenThreshold += m_delta;
						m_delta *= 2;
						m_cac_improve = true;
					}
					else
					{
						Triplet top = m_cac_history.top();
						m_cac_history.pop();
						AuthenThreshold = top.threshold;
						m_delta = std::max<int>(m_delta * top.delta / (m_delta + top.delta), 1);
						m_cac_improve = true;
					}
				}
			}
			else if (m_cac_improve)
			{
				m_delta = std::max<int>(m_delta / 4, 1);
				AuthenThreshold -= m_delta;
				m_cac_state = WORK;
				m_cac_wait_counter = 5;
				m_cac_improve_counter = 5;
			}
		}
		else // WORK state
		{
			if (AuthenThreshold == 1023)
			{
				if (m_cac_wait_counter > 0)
				{
					m_cac_wait_counter--;
				}
				else
				{
					while (m_cac_history.empty() == false)
					{
						m_cac_history.pop();
					}
					m_cac_state = WAIT;
				}
			}
			else if (MgtQueueSize == 0)
			{
				if (m_cac_improve_counter > 0)
				{
					m_cac_improve_counter--;
				}
				else
				{
					m_cac_improve = true;
				}
				if (m_cac_improve)
				{
					m_delta++;
				}
				if (m_cac_history.empty() == true || m_cac_history.top().threshold > AuthenThreshold + m_delta)
				{
					AuthenThreshold += m_delta;
				}
				else
				{
					Triplet top = m_cac_history.top();
					m_cac_history.pop();
					AuthenThreshold = top.threshold;
					m_delta = std::max<int>(m_delta * top.delta / (m_delta + top.delta), 1);
					m_cac_improve = true;
				}
				if (AuthenThreshold >= 1023)
				{
					AuthenThreshold = 1023;
					m_cac_improve = false;
				}
			}
			else if (MgtQueueSize > m_newGroupThreshold)
			{
				m_cac_history.push(Triplet(AuthenThreshold, m_delta));
				AuthenThreshold = 0;
				m_delta = 1;
				m_cac_improve = false;
				m_cac_state = LEARN;
			}
			else
			{
				m_cac_improve_counter = 5;
				if (m_queueLast == 0)
				{
					if (m_cac_improve)
					{
						m_delta = std::max<int>(m_delta - 1, 1);
						m_cac_improve = false;
					}
				}
			}
		}
	}
	void 
	ApWifiMac::Algorithm_2 (uint32_t &MgtQueueSize)
	{
		if (m_cac_state == WAIT)
		{
			if (MgtQueueSize != 0)
			{
				AuthenThreshold = 0;
				m_delta = 1;
				m_cac_improve = false;
				m_cac_state = LEARN;
			}
			else
			{
				AuthenThreshold = 1023;
			}
		}
		else if (m_cac_state == LEARN)
		{
			if (MgtQueueSize == 0)
			{
				if (AuthenThreshold == 1023)
				{
					while (m_cac_history.empty() == false)
					{
						m_cac_history.pop();
					}
					m_cac_state = WAIT;
				}
				else
				{
					if (m_cac_history.empty() == true || m_cac_history.top().threshold > AuthenThreshold + m_delta)
					{
						AuthenThreshold += m_delta;
						m_delta *= 2;
						m_cac_improve = true;
					}
					else
					{
						Triplet top = m_cac_history.top();
						m_cac_history.pop();
						AuthenThreshold = top.threshold;
						m_delta = std::max<int>(m_delta * top.delta / (m_delta + top.delta), 1);
						m_cac_improve = true;
					}
				}
			}
			else if (m_cac_improve)
			{
				m_delta = std::max<int>(m_delta / 4, 1);
				AuthenThreshold -= m_delta;
				m_cac_state = WORK;
				m_cac_wait_counter = 5;
				m_cac_improve_counter = 5;
			}
		}
		else // WORK state
		{
			if (AuthenThreshold == 1023)
			{
				if (m_cac_wait_counter > 0)
				{
					m_cac_wait_counter--;
				}
				else
				{
					while (m_cac_history.empty() == false)
					{
						m_cac_history.pop();
					}
					m_cac_state = WAIT;
				}
			}
			else if (MgtQueueSize == 0)
			{
				if (m_cac_improve_counter > 0)
				{
					m_cac_improve_counter--;
				}
				else
				{
					m_cac_improve = true;
				}
				// if (m_cac_improve)
				//   {
				m_delta++;
				// }
				if (m_cac_history.empty() == true || m_cac_history.top().threshold > AuthenThreshold + m_delta)
				{
					AuthenThreshold += m_delta;
				}
				else
				{
					Triplet top = m_cac_history.top();
					m_cac_history.pop();
					AuthenThreshold = top.threshold;
					m_delta = std::max<int>(m_delta * top.delta / (m_delta + top.delta), 1);
					m_cac_improve = true;
				}
				if (AuthenThreshold >= 1023)
				{
					AuthenThreshold = 1023;
					m_cac_improve = false;
				}
			}
			else if (MgtQueueSize > m_newGroupThreshold)
			{
				m_cac_history.push(Triplet(AuthenThreshold, m_delta));
				AuthenThreshold = 0;
				m_delta = 1;
				m_cac_improve = false;
				m_cac_state = LEARN;
			}
			else
			{
				m_cac_improve_counter = 5;
				m_delta = std::max<int>(m_delta - 1, 1);
				if (m_queueLast == 0)
				{
					if (m_cac_improve)
					{
						m_cac_improve = false;
					}
				}
			}
		}	
	}
	void 
	ApWifiMac::Algorithm_3 (uint32_t &MgtQueueSize)
	{
		if (m_cac_state == WAIT)
		{
			if (MgtQueueSize != 0)
			{
				AuthenThreshold = 0;
				m_delta = 1023;
				m_cac_improve = false;
				m_cac_wait_counter = 5;
				m_cac_state = LEARN;
			}
			else
			{
				AuthenThreshold = 1023;
			}
		}
		else if (m_cac_state == LEARN)
		{
			if (AuthenThreshold != 0)
			{
				if (MgtQueueSize != 0)
				{
					AuthenThreshold = 0;
				}
				else
				{
					m_delta /= 2;
					m_cac_improve = true;
					m_cac_state = WORK;
				}
			}
			else if (MgtQueueSize == 0)
			{
				m_delta /= 2;
				AuthenThreshold = m_delta;
			}
		}
		else // WORK state
		{
			if (AuthenThreshold == 1023)
			{
				m_cac_wait_counter--;
				if (m_cac_wait_counter <= 0)
				{
					m_cac_state = WAIT;
				}
			}
			else if (MgtQueueSize == 0)
			{
				if (m_cac_improve)
				{
					m_delta++;
				}
				AuthenThreshold += m_delta;
				if (AuthenThreshold >= 1023)
				{
					AuthenThreshold = 1023;
					m_cac_improve = false;
				}
			}
			else
			{
				if (m_queueLast == 0)
				{
					if (m_cac_improve)
					{
						m_delta = std::max<int>(m_delta - 1, 1);
						m_cac_improve = false;
					}
				}
			}
		}	
	}
	void 
	ApWifiMac::Algorithm_5 (uint32_t &MgtQueueSize) // ORACLE
	{
		if (!m_saturatedAssociated)
		{
			AuthenThreshold = 1023;
		}
		else if (m_associatingStasAppear)
		{
			AuthenThreshold = 1023 * value / m_nAssociating;
			m_associatingStasAppear = 0;
		}
		else if (MgtQueueSize == 0)
		{
			AuthenThreshold += 1023 * value / m_nAssociating;
		}
	}
	void 
	ApWifiMac::Algorithm_6 (uint32_t &MgtQueueSize) // ORACLE WITH COEFFICIENT
	{
		if (!m_saturatedAssociated)
		{
			AuthenThreshold = 1023;
		}
		else if (m_associatingStasAppear)
		{
			m_delta = 1023 * value / m_nAssociating;
			AuthenThreshold = m_delta;
			m_associatingStasAppear = 0;
			firstWave = m_nAssociating;
		}
		else if (m_secondWaveAppear)
		{
			T1 = AuthenThreshold;
			AuthenThreshold = 0;
			m_delta = 1023 * value / (m_nAssociating - firstWave);  // only second group taken into account
			m_secondWaveAppear = 0;
		}
		else if (MgtQueueSize == 0)
		{
			if (AuthenThreshold + m_delta > T1)     // after threshold reaches previous value
			{
				m_delta = 1023 * value / m_nAssociating; // both groups taken into account
			}
			AuthenThreshold += m_delta;
		}
		else
		{
			if (AuthenThreshold + m_delta > T1)
			{
				m_delta = 1023 * value / m_nAssociating;
			}
			double decrease_ratio;
			double L_r1 = 58;  //size of AuthReqs
			double L_a1 = 56; //size of AuthRep
			double L_r2 = 118; //size of AReqs
			double L_a2 = 89; // size of AReps
			m_delta = 1023 * value / m_nAssociating;

			decrease_ratio = (r1 * L_r1 + (a1 - q1) * L_a1 + (r2 - q1) * L_r2 + (a2 - q1 - q2) * L_a2) / (r1 * L_r1 + a1 * L_a1 + r2 * L_r2 + a2 * L_a2);
			std::cout << "Ratio " << decrease_ratio << '\n';

			if (decrease_ratio > 0)
			{
				m_delta_temp = std::max<int>(1, floor(m_delta * decrease_ratio));
				std::cout << "Temp Delta " << m_delta_temp << '\n';
				AuthenThreshold += m_delta_temp;
			}
		}
	}
	void 
	ApWifiMac::Algorithm_7 (uint32_t &MgtQueueSize)   // NEW PROPOSED ALGORITHM MODIFICATION A
	{
		if (m_cac_state == WAIT)
		{

			if (MgtQueueSize > m_learningThreshold)
			{
				AuthenThreshold = 0;
				m_delta = 1;
				m_cac_improve = false;
				m_cac_state = LEARN;
			}
			else
			{
				AuthenThreshold = 1023;
			}
		}
		else if (m_cac_state == LEARN)
		{
			if (MgtQueueSize == 0)
			{
				if (AuthenThreshold == 1023)
				{
					while (m_cac_history.empty() == false)
					{
						m_cac_history.pop();
					}
					m_cac_state = WAIT;
				}
				else
				{
					if (m_cac_history.empty() == true || m_cac_history.top().threshold > AuthenThreshold + m_delta)
					{
						AuthenThreshold += m_delta;
						m_delta *= 2;
						m_cac_improve = true;
					}
					else
					{
						Triplet top = m_cac_history.top();
						m_cac_history.pop();
						AuthenThreshold = top.threshold;
						m_delta = std::max<int>(m_delta * top.delta / (m_delta + top.delta), 1);
						m_cac_improve = true;
					}
				}
			}
			else if (m_cac_improve)
			{
				m_delta = std::max<int>(m_delta / 4, 1);
				AuthenThreshold -= m_delta;
				m_cac_state = WORK;
				m_cac_wait_counter = 5;
				m_cac_improve_counter = 5;
			}
		}
		else // WORK state
		{
			if (AuthenThreshold == 1023)
			{
				if (m_cac_wait_counter > 0)
				{
					m_cac_wait_counter--;
				}
				else
				{
					while (m_cac_history.empty() == false)
					{
						m_cac_history.pop();
					}
					m_cac_state = WAIT;
				}
			}
			else if (MgtQueueSize == 0)
			{
				if (m_cac_improve_counter > 0)
				{
					m_cac_improve_counter--;
				}
				else
				{
					m_cac_improve = true;
				}
				if (m_cac_improve)
				{
					m_delta++;
				}
				if (m_cac_history.empty() == true || m_cac_history.top().threshold > AuthenThreshold + m_delta)
				{
					AuthenThreshold += m_delta;
				}
				else
				{
					Triplet top = m_cac_history.top();
					m_cac_history.pop();
					AuthenThreshold = top.threshold;
					m_delta = std::max<int>(m_delta * top.delta / (m_delta + top.delta), 1);
					m_cac_improve = true;
				}
				if (AuthenThreshold >= 1023)
				{
					AuthenThreshold = 1023;
					m_cac_improve = false;
				}
			}
			else if (MgtQueueSize > m_newGroupThreshold)
			{
				m_cac_history.push(Triplet(AuthenThreshold, m_delta));
				AuthenThreshold = 0;
				m_delta = 1;
				m_cac_improve = false;
				m_cac_state = LEARN;
			}
			else
			{
				m_cac_improve_counter = 5;
				if (m_queueLast == 0)
				{
					if (m_cac_improve)
					{
						m_delta = std::max<int>(m_delta - 1, 1);
						m_cac_improve = false;
					}
				}
				double decrease_ratio;
				double L_r1 = 58;  //size of AuthReqs
				double L_a1 = 56; //size of AuthRep
				double L_r2 = 118; //size of AReqs
				double L_a2 = 89; // size of AReps

				decrease_ratio = (r1 * L_r1 + (a1 - q1) * L_a1 + (r2 - q1) * L_r2 + (a2 - q1 - q2) * L_a2) / (r1 * L_r1 + a1 * L_a1 + r2 * L_r2 + a2 * L_a2);
				std::cout << "Ratio " << decrease_ratio << '\n';

				if (decrease_ratio > 0)
				{
					m_delta_temp = std::max<int>(1, floor(m_delta * decrease_ratio));
					std::cout << "Temp Delta " << m_delta_temp << '\n';
					if (m_cac_history.empty() == true || m_cac_history.top().threshold > AuthenThreshold + m_delta_temp)
					{
						AuthenThreshold += m_delta_temp;
					}
					else
					{
						Triplet top = m_cac_history.top();
						m_cac_history.pop();
						AuthenThreshold = top.threshold;
						m_delta = std::max<int>(m_delta * top.delta / (m_delta + top.delta), 1);
						m_cac_improve = true;
					}
				}
			}
		}
	}
	void 
	ApWifiMac::Algorithm_8 (uint32_t &MgtQueueSize)  ///// NEW PROPOSED ALGORITHM
	{
		if (m_cac_state == WAIT)
		{
			if (MgtQueueSize != 0)
			{
				AuthenThreshold = 0;
				m_delta = 1;
				m_cac_improve = false;
				m_cac_state = LEARN;
			}
			else
			{
				AuthenThreshold = 1023;
			}
		}
		else if (m_cac_state == LEARN)
		{
			if (MgtQueueSize == 0)
			{
				if (AuthenThreshold == 1023)
				{
					while (m_cac_history.empty() == false)
					{
						m_cac_history.pop();
					}
					m_cac_state = WAIT;
				}
				else
				{
					if (m_cac_history.empty() == true || m_cac_history.top().threshold > AuthenThreshold + m_delta)
					{
						AuthenThreshold += m_delta;
						m_delta *= 2;
						m_cac_improve = true;
					}
					else
					{
						Triplet top = m_cac_history.top();
						m_cac_history.pop();
						AuthenThreshold = top.threshold;
						m_delta = std::max<int>(m_delta * top.delta / (m_delta + top.delta), 1);
						m_cac_improve = true;
					}
				}
			}
			else if (m_cac_improve)
			{
				m_delta = std::max<int>(m_delta / 4, 1);
				AuthenThreshold -= m_delta;
				m_cac_state = WORK;
				m_cac_wait_counter = 5;
				m_cac_improve_counter = 5;
			}
		}
		else // WORK state
		{
			if (AuthenThreshold == 1023)
			{
				if (m_cac_wait_counter > 0)
				{
					m_cac_wait_counter--;
				}
				else
				{
					while (m_cac_history.empty() == false)
					{
						m_cac_history.pop();
					}
					m_cac_state = WAIT;
				}
			}
			else if (MgtQueueSize == 0)
			{
				if (m_cac_improve)
				{
					//m_delta++;   // option 1                              
					//m_delta = m_delta * T_BI/tau;  // OPTION 2
					std::cout << "tau: " << m_tau.GetMilliSeconds() << '\n';
					std::cout << "BI: " << m_beaconInterval.GetMilliSeconds() << '\n';
					if (m_tau > 0)
					{
						double increase_ratio = m_beaconInterval.GetMilliSeconds() / m_tau.GetMilliSeconds();
						std::cout << "increasing Delta " << increase_ratio << " times" << '\n';
						m_delta = m_delta * increase_ratio;
					}
					// END OPTION 2
				}

				if (m_cac_history.empty() == true || m_cac_history.top().threshold > AuthenThreshold + m_delta)
				{
					AuthenThreshold += m_delta;
					m_cac_improve = true;
				}
				else
				{
					Triplet top = m_cac_history.top();
					m_cac_history.pop();
					AuthenThreshold = top.threshold;
					m_delta = std::max<int>(m_delta * top.delta / (m_delta + top.delta), 1);
					m_cac_improve = true;
				}
				if (AuthenThreshold >= 1023)
				{
					AuthenThreshold = 1023;
					m_cac_improve = false;
				}
			}
			else if (MgtQueueSize > m_newGroupThreshold)
			{
				m_cac_history.push(Triplet(AuthenThreshold, m_delta));
				AuthenThreshold = 0;
				m_delta = 1;
				m_cac_improve = false;
				m_cac_state = LEARN;
			}
			else  ////// queue is not empty
			{

				// //// option 1
				//double success_channel_ratio;
				//  double D;
				//  double T_r1 = 0.00016;  //time of AuthReqs
				//  double T_a1 = 0.00188; //time of AuthRep
				//  double T_r2 = 0.00016; //time of AReqs
				//  double T_a2 = 0.00232; // time of AReps
				//  D = r1 * T_r1 + a1 * T_a1 + r2 * T_r2 + a2 * T_a2;
				//  success_channel_ratio = fabs((D - q1*(T_a1+T_r2)-T_a2*(q1+q2))/D);
				//  std::cout << "D " << D << '\n';

				//// option 2
				double success_channel_ratio;
				double L_r1 = 58;  //size of AuthReqs
				double L_a1 = 56; //size of AuthRep
				double L_r2 = 118; //size of AReqs
				double L_a2 = 89; // size of AReps

				success_channel_ratio = (r1 * L_r1 + (a1 - q1) * L_a1 + (r2 - q1) * L_r2 + (a2 - q1 - q2) * L_a2) / (r1 * L_r1 + a1 * L_a1 + r2 * L_r2 + a2 * L_a2);
				std::cout << "Ratio " << success_channel_ratio << '\n';

				if (success_channel_ratio > 0)
				{
					m_delta = std::max<int>(1, floor(m_delta * success_channel_ratio));
					std::cout << "Floor " << m_delta << '\n';
					AuthenThreshold += m_delta;
				}
			}
		}
	}
	void 
	ApWifiMac::Algorithm_9 (uint32_t &MgtQueueSize)
	{
		if (m_cac_state == WAIT)
		{
			if (MgtQueueSize != 0)
			{
				AuthenThreshold = 0;
				m_delta = 1;
				m_cac_improve = false;
				m_cac_state = LEARN;
			}
			else
			{
				AuthenThreshold = 1023;
			}
		}
		else if (m_cac_state == LEARN)
		{
			if (MgtQueueSize == 0)
			{
				if (AuthenThreshold == 1023)
				{
					while (m_cac_history.empty() == false)
					{
						m_cac_history.pop();
					}
					m_cac_state = WAIT;
				}
				else
				{
					if (m_cac_history.empty() == true || m_cac_history.top().threshold > AuthenThreshold + m_delta)
					{
						AuthenThreshold += m_delta;
						m_delta *= 2;
						m_cac_improve = true;
					}
					else
					{
						Triplet top = m_cac_history.top();
						m_cac_history.pop();
						AuthenThreshold = top.threshold;
						m_delta = std::max<int>(m_delta * top.delta / (m_delta + top.delta), 1);
						m_cac_improve = true;
					}
				}
			}
			else if (m_cac_improve)
			{
				m_delta = std::max<int>(m_delta / 4, 1);
				AuthenThreshold -= m_delta;
				m_cac_state = WORK;
				m_cac_wait_counter = 5;
				m_cac_improve_counter = 5;
			}
		}
		else // WORK state
		{
			if (AuthenThreshold == 1023)
			{
				if (m_cac_wait_counter > 0)
				{
					m_cac_wait_counter--;
				}
				else
				{
					while (m_cac_history.empty() == false)
					{
						m_cac_history.pop();
					}
					m_cac_state = WAIT;
				}
			}
			else if (MgtQueueSize == 0)
			{
				if (m_cac_improve)
				{
					m_delta++;
				}

				if (m_cac_history.empty() == true || m_cac_history.top().threshold > AuthenThreshold + m_delta)
				{
					AuthenThreshold += m_delta;
					m_cac_improve = true;
				}
				else
				{
					Triplet top = m_cac_history.top();
					m_cac_history.pop();
					AuthenThreshold = top.threshold;
					m_delta = std::max<int>(m_delta * top.delta / (m_delta + top.delta), 1);
					m_cac_improve = true;
				}
				if (AuthenThreshold >= 1023)
				{
					AuthenThreshold = 1023;
					m_cac_improve = false;
				}
			}
			else if (MgtQueueSize <= 5)
			{
				AuthenThreshold += m_delta;
				m_cac_improve = false;
			}
			// if higher than 5 but less than 11 - keep threshold as it is, keep delta as it is
			else if (MgtQueueSize > 5 && MgtQueueSize <= 10)
			{
				m_cac_improve = false;
			}
			else if (MgtQueueSize > 10 && MgtQueueSize <= 20)
			{
				m_delta = std::max<int>(m_delta - 1, 1);
				m_cac_improve = false;
			}
			else if (MgtQueueSize > 20 && MgtQueueSize <= 30)
			{
				m_delta = std::max<int>(m_delta - 2, 1);
				m_cac_improve = false;
			}
			else if (MgtQueueSize > 30 && MgtQueueSize <= 40)
			{
				m_delta = std::max<int>(m_delta - 3, 1);
				m_cac_improve = false;
			}
			else if (MgtQueueSize > 40 && MgtQueueSize <= 50)
			{
				m_delta = std::max<int>(m_delta - 4, 1);
				m_cac_improve = false;
			}
			else if (MgtQueueSize > 50 && MgtQueueSize <= 60)
			{
				m_delta = std::max<int>(m_delta - 5, 1);
				m_cac_improve = false;
			}
			else if (MgtQueueSize > 60 && MgtQueueSize <= 70)
			{
				m_delta = std::max<int>(m_delta - 6, 1);
				m_cac_improve = false;
			}
			else if (MgtQueueSize > 70 && MgtQueueSize <= 80)
			{
				m_delta = std::max<int>(m_delta - 7, 1);
				m_cac_improve = false;
			}
			else if (MgtQueueSize > 80 && MgtQueueSize <= 90)
			{
				m_delta = std::max<int>(m_delta - 8, 1);
				m_cac_improve = false;
			}
			else if (MgtQueueSize > 90 && MgtQueueSize <= 100)
			{
				m_delta = std::max<int>(m_delta - 9, 1);
				m_cac_improve = false;
			}
			else if (MgtQueueSize > 100)
			{
				m_cac_history.push(Triplet(AuthenThreshold, m_delta));
				AuthenThreshold = 0;
				m_delta = 1;
				m_cac_improve = false;
				m_cac_state = LEARN;
			}
		}
	}
	void 
	ApWifiMac::Algorithm_10 (uint32_t &MgtQueueSize)  // NEW Algorithm with correct time slot values
	{
		if (m_cac_state == WAIT)
		{
			//    if (MgtQueueSize != 0)
			// do not start learning when there is small STA number
			if (MgtQueueSize > m_learningThreshold)
			{
				AuthenThreshold = 0;
				m_delta = 1;
				m_cac_improve = false;
				m_cac_state = LEARN;
			}
			else
			{
				AuthenThreshold = 1023;
			}
		}
		else if (m_cac_state == LEARN)
		{
			if (MgtQueueSize == 0)
			{
				if (AuthenThreshold == 1023)
				{
					while (m_cac_history.empty() == false)
					{
						m_cac_history.pop();
					}
					m_cac_state = WAIT;
				}
				else
				{
					if (m_cac_history.empty() == true || m_cac_history.top().threshold > AuthenThreshold + m_delta)
					{
						AuthenThreshold += m_delta;
						m_delta *= 2;
						m_cac_improve = true;
					}
					else
					{
						Triplet top = m_cac_history.top();
						m_cac_history.pop();
						AuthenThreshold = top.threshold;
						m_delta = std::max<int>(m_delta * top.delta / (m_delta + top.delta), 1);
						m_cac_improve = true;
					}
				}
			}
			else if (m_cac_improve)
			{
				m_delta = std::max<int>(m_delta / 4, 1);
				AuthenThreshold -= m_delta;
				m_cac_state = WORK;
				m_cac_wait_counter = 5;
				m_cac_improve_counter = 5;
			}
		}
		else // WORK state
		{
			if (AuthenThreshold == 1023)
			{
				if (m_cac_wait_counter > 0)
				{
					m_cac_wait_counter--;
				}
				else
				{
					while (m_cac_history.empty() == false)
					{
						m_cac_history.pop();
					}
					m_cac_state = WAIT;
				}
			}
			else if (MgtQueueSize == 0)
			{
				if (m_cac_improve_counter > 0)
				{
					m_cac_improve_counter--;
				}
				else
				{
					m_cac_improve = true;
				}
				if (m_cac_improve)
				{
					m_delta++;
				}
				if (m_cac_history.empty() == true || m_cac_history.top().threshold > AuthenThreshold + m_delta)
				{
					AuthenThreshold += m_delta;
				}
				else
				{
					Triplet top = m_cac_history.top();
					m_cac_history.pop();
					AuthenThreshold = top.threshold;
					m_delta = std::max<int>(m_delta * top.delta / (m_delta + top.delta), 1);
					m_cac_improve = true;
				}
				if (AuthenThreshold >= 1023)
				{
					AuthenThreshold = 1023;
					m_cac_improve = false;
				}
			}
			else if (MgtQueueSize > m_newGroupThreshold)
			{
				m_cac_history.push(Triplet(AuthenThreshold, m_delta));
				AuthenThreshold = 0;
				m_delta = 1;
				m_cac_improve = false;
				m_cac_state = LEARN;
			}
			else
			{
				m_cac_improve_counter = 5;
				if (m_queueLast == 0)
				{
					if (m_cac_improve)
					{
						m_delta = std::max<int>(m_delta - 1, 1);
						m_cac_improve = false;
					}
				}
				double decrease_ratio;
				double L_r1 = 720 + 160 + 1000; //time needed to transmit AuthReq + SIFS + Ack
				double L_a1 = 720 + 160 + 1000; //time needed to transmit AuthRep + SIFS + Ack
				double L_r2 = 1520 + 160 + 1000; //time needed to transmit AReq + SIFS + Ack
				double L_a2 = 1160 + 160 + 1000; //time needed to transmit ARep + SIFS + Ack

				decrease_ratio = (r1 * L_r1 + (a1 - q1) * L_a1 + (r2 - q1) * L_r2 + (a2 - q1 - q2) * L_a2) / (r1 * L_r1 + a1 * L_a1 + r2 * L_r2 + a2 * L_a2);

				std::cout << "Ratio " << decrease_ratio << '\n';
				if (decrease_ratio > 0)
				{
					m_delta_temp = std::max<int>(1, floor(m_delta * decrease_ratio));
					std::cout << "Temp Delta " << m_delta_temp << '\n';
					if (m_cac_history.empty() == true || m_cac_history.top().threshold > AuthenThreshold + m_delta_temp)
					{
						AuthenThreshold += m_delta_temp;
					}
					else
					{
						Triplet top = m_cac_history.top();
						m_cac_history.pop();
						AuthenThreshold = top.threshold;
						m_delta = std::max<int>(m_delta * top.delta / (m_delta + top.delta), 1);
						m_cac_improve = true;
					}
				}
			}
		}
	}

} //namespace ns3


