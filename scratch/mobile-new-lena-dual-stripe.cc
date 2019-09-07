#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/mobility-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>
#include <ns3/config-store-module.h>
#include <ns3/buildings-module.h>
#include <ns3/point-to-point-helper.h>
#include <ns3/applications-module.h>
#include <ns3/log.h>
#include <iomanip>
#include <ios>
#include <string>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("LenaDualStripe");

bool AreOverlapping (Box a, Box b)
{
  return !((a.xMin > b.xMax) || (b.xMin > a.xMax) || (a.yMin > b.yMax) || (b.yMin > a.yMax));
}

void
PrintGnuplottableBuildingListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  uint32_t index = 0;
  for (BuildingList::Iterator it = BuildingList::Begin (); it != BuildingList::End (); ++it)
    {
      ++index;
      Box box = (*it)->GetBoundaries ();
      outFile << "set object " << index
              << " rect from " << box.xMin  << "," << box.yMin
              << " to "   << box.xMax  << "," << box.yMax
              << " front fs empty "
              << std::endl;
    }
}


void 
PrintGnuplottableUeListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<LteUeNetDevice> uedev = node->GetDevice (j)->GetObject <LteUeNetDevice> ();
          if (uedev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << uedev->GetImsi ()
                      << "\" at "<< pos.x << "," << pos.y << " left font \"Helvetica,10\" textcolor rgb \"grey\" front point pt 1 ps 0.3 lc rgb \"grey\" offset 0,0"
                      << std::endl;
            }
        }
    }
}

void 
PrintGnuplottableEnbListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<LteEnbNetDevice> enbdev = node->GetDevice (j)->GetObject <LteEnbNetDevice> ();
          if (enbdev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << enbdev->GetCellId ()
                      << "\" at "<< pos.x << "," << pos.y
                      << " left font \"Helvetica,10\" textcolor rgb \"green\" front  point pt 2 ps 0.3 lc rgb \"green\" offset 0,0"
                      << std::endl;
            }
        }
    }
}


static ns3::GlobalValue g_nMacroEnbSites ("nMacroEnbSites",
                                          "How many macro sites there are",
                                          ns3::UintegerValue (7),
                                          ns3::MakeUintegerChecker<uint32_t> ());

static ns3::GlobalValue g_nMacroEnbSitesX ("nMacroEnbSitesX",
                                           "(minimum) number of sites along the X-axis of the hex grid",
                                           ns3::UintegerValue (2),
                                           ns3::MakeUintegerChecker<uint32_t> ());

static ns3::GlobalValue g_interSiteDistance ("interSiteDistance",
                                             "min distance between two nearby macro cell sites",
                                             ns3::DoubleValue (500),
                                             ns3::MakeDoubleChecker<double> ());

static ns3::GlobalValue g_areaMarginFactor ("areaMarginFactor",
                                            "how much the UE area extends outside the macrocell grid, "
                                            "expressed as fraction of the interSiteDistance",
                                            ns3::DoubleValue (0.5),
                                            ns3::MakeDoubleChecker<double> ());

static ns3::GlobalValue g_macroUeDensity ("macroUeDensity",
                                          "How many macrocell UEs there are per square meter",
                                          ns3::DoubleValue (0.00002),
                                          ns3::MakeDoubleChecker<double> ());

static ns3::GlobalValue g_macroEnbTxPowerDbm ("macroEnbTxPowerDbm",
                                              "TX power [dBm] used by macro eNBs",
                                              ns3::DoubleValue (46.0),
                                              ns3::MakeDoubleChecker<double> ());

static ns3::GlobalValue g_macroEnbDlEarfcn ("macroEnbDlEarfcn",
                                            "DL EARFCN used by macro eNBs",
                                            ns3::UintegerValue (100),
                                            ns3::MakeUintegerChecker<uint16_t> ());

static ns3::GlobalValue g_macroEnbBandwidth ("macroEnbBandwidth",
                                             "bandwidth [num RBs] used by macro eNBs",
                                             ns3::UintegerValue (25),
                                             ns3::MakeUintegerChecker<uint16_t> ());

static ns3::GlobalValue g_simTime ("simTime",
                                   "Total duration of the simulation [s]",
                                   ns3::DoubleValue (30),
                                   ns3::MakeDoubleChecker<double> ());

static ns3::GlobalValue g_generateRem ("generateRem",
                                       "if true, will generate a REM and then abort the simulation;"
                                       "if false, will run the simulation normally (without generating any REM)",
                                       ns3::BooleanValue (false),
                                       ns3::MakeBooleanChecker ());

static ns3::GlobalValue g_remRbId ("remRbId",
                                   "Resource Block Id of Data Channel, for which REM will be generated;"
                                   "default value is -1, what means REM will be averaged from all RBs of "
                                   "Control Channel",
                                   ns3::IntegerValue (-1),
                                   MakeIntegerChecker<int32_t> ());

static ns3::GlobalValue g_epc ("epc",
                               "If true, will setup the EPC to simulate an end-to-end topology, "
                               "with real IP applications over PDCP and RLC UM (or RLC AM by changing "
                               "the default value of EpsBearerToRlcMapping e.g. to RLC_AM_ALWAYS). "
                               "If false, only the LTE radio access will be simulated with RLC SM. ",
                               ns3::BooleanValue (true),
                               ns3::MakeBooleanChecker ());

static ns3::GlobalValue g_epcDl ("epcDl",
                                 "if true, will activate data flows in the downlink when EPC is being used. "
                                 "If false, downlink flows won't be activated. "
                                 "If EPC is not used, this parameter will be ignored.",
                                 ns3::BooleanValue (true),
                                 ns3::MakeBooleanChecker ());

static ns3::GlobalValue g_epcUl ("epcUl",
                                 "if true, will activate data flows in the uplink when EPC is being used. "
                                 "If false, uplink flows won't be activated. "
                                 "If EPC is not used, this parameter will be ignored.",
                                 ns3::BooleanValue (true),
                                 ns3::MakeBooleanChecker ());

static ns3::GlobalValue g_useUdp ("useUdp",
                                  "if true, the UdpClient application will be used. "
                                  "Otherwise, the BulkSend application will be used over a TCP connection. "
                                  "If EPC is not used, this parameter will be ignored.",
                                  ns3::BooleanValue (true),
                                  ns3::MakeBooleanChecker ());

static ns3::GlobalValue g_fadingTrace ("fadingTrace",
                                       "The path of the fading trace (by default no fading trace "
                                       "is loaded, i.e., fading is not considered)",
                                       ns3::StringValue (""),
                                       ns3::MakeStringChecker ());

static ns3::GlobalValue g_numBearersPerUe ("numBearersPerUe",
                                           "How many bearers per UE there are in the simulation",
                                           ns3::UintegerValue (1),
                                           ns3::MakeUintegerChecker<uint16_t> ());

static ns3::GlobalValue g_srsPeriodicity ("srsPeriodicity",
                                          "SRS Periodicity (has to be at least "
                                          "greater than the number of UEs per eNB)",
                                          ns3::UintegerValue (80),
                                          ns3::MakeUintegerChecker<uint16_t> ());
static ns3::GlobalValue g_outdoorUeMinSpeed ("outdoorUeMinSpeed",
                                             "Minimum speed value of macro UE with random waypoint model [m/s].",
                                             ns3::DoubleValue (16.6667),
                                             ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_outdoorUeMaxSpeed ("outdoorUeMaxSpeed",
                                             "Maximum speed value of macro UE with random waypoint model [m/s].",
                                             ns3::DoubleValue (16.6667),
                                             ns3::MakeDoubleChecker<double> ());

int main (int argc, char *argv[])
{
  // change some default attributes so that they are reasonable for
  // this scenario, but do this before processing command line
  // arguments, so that the user is allowed to override these settings 
  Config::SetDefault ("ns3::UdpClient::Interval", TimeValue (MilliSeconds (1)));
  Config::SetDefault ("ns3::UdpClient::MaxPackets", UintegerValue (1000000));
  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (10 * 1024));

  CommandLine cmd;
  cmd.Parse (argc, argv);
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();
  // parse again so you can override input file default values via command line
  cmd.Parse (argc, argv); 

  // the scenario parameters get their values from the global attributes defined above
  UintegerValue uintegerValue;
  IntegerValue integerValue;
  DoubleValue doubleValue;
  BooleanValue booleanValue;
  StringValue stringValue;

  
  GlobalValue::GetValueByName ("nMacroEnbSites", uintegerValue);
  uint32_t nMacroEnbSites = uintegerValue.Get ();
  GlobalValue::GetValueByName ("nMacroEnbSitesX", uintegerValue);
  uint32_t nMacroEnbSitesX = uintegerValue.Get ();
  GlobalValue::GetValueByName ("interSiteDistance", doubleValue);
  double interSiteDistance = doubleValue.Get ();
  GlobalValue::GetValueByName ("macroUeDensity", doubleValue);
  double macroUeDensity = doubleValue.Get ();
  GlobalValue::GetValueByName ("areaMarginFactor", doubleValue);
  double areaMarginFactor = doubleValue.Get ();
  GlobalValue::GetValueByName ("macroEnbTxPowerDbm", doubleValue);
  double macroEnbTxPowerDbm = doubleValue.Get ();
  GlobalValue::GetValueByName ("macroEnbDlEarfcn", uintegerValue);
  uint32_t macroEnbDlEarfcn = uintegerValue.Get ();
  GlobalValue::GetValueByName ("macroEnbBandwidth", uintegerValue);
  uint16_t macroEnbBandwidth = uintegerValue.Get ();
  GlobalValue::GetValueByName ("simTime", doubleValue);
  double simTime = doubleValue.Get ();
  GlobalValue::GetValueByName ("epc", booleanValue);
  bool epc = booleanValue.Get ();
  GlobalValue::GetValueByName ("epcDl", booleanValue);
  bool epcDl = booleanValue.Get ();
  GlobalValue::GetValueByName ("epcUl", booleanValue);
  bool epcUl = booleanValue.Get ();
  GlobalValue::GetValueByName ("useUdp", booleanValue);
  bool useUdp = booleanValue.Get ();
  GlobalValue::GetValueByName ("generateRem", booleanValue);
  bool generateRem = booleanValue.Get ();
  GlobalValue::GetValueByName ("remRbId", integerValue);
  int32_t remRbId = integerValue.Get ();
  GlobalValue::GetValueByName ("fadingTrace", stringValue);
  std::string fadingTrace = stringValue.Get ();
  GlobalValue::GetValueByName ("numBearersPerUe", uintegerValue);
  uint16_t numBearersPerUe = uintegerValue.Get ();
  GlobalValue::GetValueByName ("srsPeriodicity", uintegerValue);
  uint16_t srsPeriodicity = uintegerValue.Get ();
  GlobalValue::GetValueByName ("outdoorUeMinSpeed", doubleValue);
  uint16_t outdoorUeMinSpeed = doubleValue.Get ();
  GlobalValue::GetValueByName ("outdoorUeMaxSpeed", doubleValue);
  uint16_t outdoorUeMaxSpeed = doubleValue.Get ();
  
  Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (srsPeriodicity));

  Box macroUeBox;
  double ueZ = 1.5;
  if (nMacroEnbSites > 0)
    {
      uint32_t currentSite = nMacroEnbSites -1;
      uint32_t biRowIndex = (currentSite / (nMacroEnbSitesX + nMacroEnbSitesX + 1));
      uint32_t biRowRemainder = currentSite % (nMacroEnbSitesX + nMacroEnbSitesX + 1);
      uint32_t rowIndex = biRowIndex*2 + 1;
      if (biRowRemainder >= nMacroEnbSitesX)
        {
          ++rowIndex;
        }
      uint32_t nMacroEnbSitesY = rowIndex;
      NS_LOG_LOGIC ("nMacroEnbSitesY = " << nMacroEnbSitesY);

      macroUeBox = Box (-areaMarginFactor*interSiteDistance, 
                        (nMacroEnbSitesX + areaMarginFactor)*interSiteDistance, 
                        -areaMarginFactor*interSiteDistance, 
                        (nMacroEnbSitesY -1)*interSiteDistance*sqrt (0.75) + areaMarginFactor*interSiteDistance,
                        ueZ, ueZ);
    }
  

//  macroUeBox = Box (-1000, 1000, -1000, 1000, ueZ, ueZ);

  double macroUeAreaSize = (macroUeBox.xMax - macroUeBox.xMin) * (macroUeBox.yMax - macroUeBox.yMin);
  uint32_t nMacroUes = round (macroUeAreaSize * macroUeDensity);
  //uint32_t nMacroUes=100;
  NS_LOG_LOGIC ("nMacroUes = " << nMacroUes << " (density=" << macroUeDensity << ")"); 



 // uint32_t nMacroUes = 5;
  NodeContainer macroEnbs;
  macroEnbs.Create (3 * nMacroEnbSites);  //3 or not 
  NodeContainer macroUes;
  macroUes.Create (nMacroUes);

  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  Ptr <LteHelper> lteHelper = CreateObject<LteHelper> ();
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::LogNormalShadowingLossModel"));  //other path loss models to be studied
  lteHelper->SetPathlossModelAttribute("Exponent",DoubleValue (3.52));  // similarly for attributes

  Ptr<LogNormalRandomVariable> theta = CreateObject<LogNormalRandomVariable> ();
  theta->SetAttribute ("Mu", DoubleValue (0.0));
  theta->SetAttribute ("Sigma", DoubleValue (6.5));

  lteHelper->SetPathlossModelAttribute("LogNormalRv",PointerValue(theta));


  if (!fadingTrace.empty ())
    {
      lteHelper->SetAttribute ("FadingModel", StringValue ("ns3::TraceFadingLossModel"));
      lteHelper->SetFadingModelAttribute ("TraceFilename", StringValue (fadingTrace));
    }

  Ptr<PointToPointEpcHelper> epcHelper;
  if (epc)
    {
      NS_LOG_LOGIC ("enabling EPC");
      epcHelper = CreateObject<PointToPointEpcHelper> ();
      lteHelper->SetEpcHelper (epcHelper);
    }

  mobility.Install (macroEnbs);
  BuildingsHelper::Install (macroEnbs);

 Ptr<LteHexGridEnbTopologyHelper> lteHexGridEnbTopologyHelper = CreateObject<LteHexGridEnbTopologyHelper> ();
   lteHexGridEnbTopologyHelper->SetLteHelper (lteHelper);
   lteHexGridEnbTopologyHelper->SetAttribute ("InterSiteDistance", DoubleValue (interSiteDistance));
   lteHexGridEnbTopologyHelper->SetAttribute ("MinX", DoubleValue (interSiteDistance/2));
   lteHexGridEnbTopologyHelper->SetAttribute ("GridWidth", UintegerValue (nMacroEnbSitesX));
   Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (macroEnbTxPowerDbm));
   lteHelper->SetEnbAntennaModelType ("ns3::ParabolicAntennaModel");
   lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (70));
   lteHelper->SetEnbAntennaModelAttribute ("MaxAttenuation",     DoubleValue (20.0));
   lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (macroEnbDlEarfcn));
   lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (macroEnbDlEarfcn + 18000));
   lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (macroEnbBandwidth));
   lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (macroEnbBandwidth));
   NetDeviceContainer macroEnbDevs = lteHexGridEnbTopologyHelper->SetPositionAndInstallEnbDevice (macroEnbs);
 

 if (epc)
    {
      // this enables handover for macro eNBs
      lteHelper->AddX2Interface (macroEnbs);
   }


//Ptr<PositionAllocator> positionAlloc = CreateObject<RandomRoomPositionAllocator> ();
  // macro Ues
  NS_LOG_LOGIC ("randomly allocating macro UEs in " << macroUeBox << " speedMin " << outdoorUeMinSpeed << " speedMax " << outdoorUeMaxSpeed);
  if (outdoorUeMaxSpeed!=0.0)
  {
    mobility.SetMobilityModel ("ns3::SteadyStateRandomWaypointMobilityModel");
      
      Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MinX", DoubleValue (macroUeBox.xMin));
      Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MinY", DoubleValue (macroUeBox.yMin));
      Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MaxX", DoubleValue (macroUeBox.xMax));
      Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MaxY", DoubleValue (macroUeBox.yMax));
      Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::Z", DoubleValue (ueZ));
      Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MaxSpeed", DoubleValue (outdoorUeMaxSpeed));
      Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MinSpeed", DoubleValue (outdoorUeMinSpeed));


      Ptr<PositionAllocator> positionAlloc = CreateObject<RandomBoxPositionAllocator> ();
      mobility.SetPositionAllocator (positionAlloc);
      mobility.Install (macroUes);

    for (NodeContainer::Iterator it = macroUes.Begin ();
           it != macroUes.End ();
           ++it)
        {
          (*it)->Initialize ();
        }
    }

else
  {	
      Ptr<PositionAllocator> positionAlloc = CreateObject<RandomBoxPositionAllocator> ();
      Ptr<UniformRandomVariable> xVal = CreateObject<UniformRandomVariable> ();
      xVal->SetAttribute ("Min", DoubleValue (macroUeBox.xMin));
      xVal->SetAttribute ("Max", DoubleValue (macroUeBox.xMax));
      positionAlloc->SetAttribute ("X", PointerValue (xVal));
      Ptr<UniformRandomVariable> yVal = CreateObject<UniformRandomVariable> ();
      yVal->SetAttribute ("Min", DoubleValue (macroUeBox.yMin));
      yVal->SetAttribute ("Max", DoubleValue (macroUeBox.yMax));
      positionAlloc->SetAttribute ("Y", PointerValue (yVal));
      Ptr<UniformRandomVariable> zVal = CreateObject<UniformRandomVariable> ();
      zVal->SetAttribute ("Min", DoubleValue (macroUeBox.zMin));
      zVal->SetAttribute ("Max", DoubleValue (macroUeBox.zMax));
      positionAlloc->SetAttribute ("Z", PointerValue (zVal));
      mobility.SetPositionAllocator (positionAlloc);
      mobility.Install (macroUes);
  }  

  BuildingsHelper::Install (macroUes);
  NetDeviceContainer macroUeDevs = lteHelper->InstallUeDevice (macroUes);

  Ipv4Address remoteHostAddr;
  NodeContainer ues;
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ipv4InterfaceContainer ueIpIfaces;
  Ptr<Node> remoteHost;
  NetDeviceContainer ueDevs;

if (epc)
    {
      NS_LOG_LOGIC ("setting up internet and remote host");

      // Create a single RemoteHost
      NodeContainer remoteHostContainer;
      remoteHostContainer.Create (1);
      remoteHost = remoteHostContainer.Get (0);
      InternetStackHelper internet;
      internet.Install (remoteHostContainer);


// Create the Internet
      PointToPointHelper p2ph;
      p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
      p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
      p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
      Ptr<Node> pgw = epcHelper->GetPgwNode ();
      NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
      Ipv4AddressHelper ipv4h;
      ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
      Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
      // in this container, interface 0 is the pgw, 1 is the remoteHost
      remoteHostAddr = internetIpIfaces.GetAddress (1);

      Ipv4StaticRoutingHelper ipv4RoutingHelper;
      Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
      remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

      ues.Add (macroUes);
      ueDevs.Add (macroUeDevs);

      internet.Install (ues);
      ueIpIfaces = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs));

      lteHelper->Attach (macroUeDevs);

     }
   
      else
    {

	  lteHelper->AttachToClosestEnb (macroUeDevs, macroEnbDevs);

    }

 if (epc)
    {
      NS_LOG_LOGIC ("setting up applications");

      // Install and start applications on UEs and remote host
      uint16_t dlPort = 10000;
      uint16_t ulPort = 20000;

      // randomize a bit start times to avoid simulation artifacts
      // (e.g., buffer overflows due to packet transmissions happening
      // exactly at the same time) 
      Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable> ();
      if (useUdp)
         {
            
          startTimeSeconds->SetAttribute ("Min", DoubleValue (0));
          startTimeSeconds->SetAttribute ("Max", DoubleValue (0.010));
        }
      else
        {
          // TCP needs to be started late enough so that all UEs are connected
          // otherwise TCP SYN packets will get lost
          startTimeSeconds->SetAttribute ("Min", DoubleValue (0.100));
          startTimeSeconds->SetAttribute ("Max", DoubleValue (0.110));
        }

      for (uint32_t u = 0; u < ues.GetN (); ++u)
        {
          Ptr<Node> ue = ues.Get (u);
          // Set the default gateway for the UE
          Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ());
          ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

          for (uint32_t b = 0; b < numBearersPerUe; ++b)
            {
              ++dlPort;
              ++ulPort;

              ApplicationContainer clientApps;
              ApplicationContainer serverApps;

              if (useUdp)
                {
                 if (epcDl)
                  {
                   NS_LOG_LOGIC ("installing UDP DL app for UE " << u);
                      UdpClientHelper dlClientHelper (ueIpIfaces.GetAddress (u), dlPort);
                      clientApps.Add (dlClientHelper.Install (remoteHost));
                      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", 
                                                           InetSocketAddress (Ipv4Address::GetAny (), dlPort));
                      serverApps.Add (dlPacketSinkHelper.Install (ue));
                    }
                  if (epcUl)
                    {
                      NS_LOG_LOGIC ("installing UDP UL app for UE " << u);
                      UdpClientHelper ulClientHelper (remoteHostAddr, ulPort);
                      clientApps.Add (ulClientHelper.Install (ue));
                      PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", 
                                                           InetSocketAddress (Ipv4Address::GetAny (), ulPort));
                      serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
                    }
                }
              else // use TCP
                {
                  if (epcDl)
                    {
                      NS_LOG_LOGIC ("installing TCP DL app for UE " << u);
                      BulkSendHelper dlClientHelper ("ns3::TcpSocketFactory",
                                                     InetSocketAddress (ueIpIfaces.GetAddress (u), dlPort));
                      dlClientHelper.SetAttribute ("MaxBytes", UintegerValue (0));
                      clientApps.Add (dlClientHelper.Install (remoteHost));
                      PacketSinkHelper dlPacketSinkHelper ("ns3::TcpSocketFactory", 
                                                           InetSocketAddress (Ipv4Address::GetAny (), dlPort));
                      serverApps.Add (dlPacketSinkHelper.Install (ue));
                    }
              if (epcUl)
                    {
                      NS_LOG_LOGIC ("installing TCP UL app for UE " << u);
                      BulkSendHelper ulClientHelper ("ns3::TcpSocketFactory",
                                                     InetSocketAddress (remoteHostAddr, ulPort));
                      ulClientHelper.SetAttribute ("MaxBytes", UintegerValue (0));
                      clientApps.Add (ulClientHelper.Install (ue));
                      PacketSinkHelper ulPacketSinkHelper ("ns3::TcpSocketFactory", 
                                                           InetSocketAddress (Ipv4Address::GetAny (), ulPort));
                      serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
                    }
                }

               Ptr<EpcTft> tft = Create<EpcTft> ();
              if (epcDl)
                {
                  EpcTft::PacketFilter dlpf;
                  dlpf.localPortStart = dlPort;
                  dlpf.localPortEnd = dlPort;
                  tft->Add (dlpf); 
                }
              if (epcUl)
                {
                  EpcTft::PacketFilter ulpf;
                  ulpf.remotePortStart = ulPort;
                  ulpf.remotePortEnd = ulPort;
                  tft->Add (ulpf);
                }

              if (epcDl || epcUl)
                {
                  EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
                  lteHelper->ActivateDedicatedEpsBearer (ueDevs.Get (u), bearer, tft);
                }
              Time startTime = Seconds (startTimeSeconds->GetValue ());
              serverApps.Start (startTime);
              clientApps.Start (startTime);

            }
            }

	}
     else
        
         {
      // for radio bearer activation purposes, consider together home UEs and macro UEs
      NetDeviceContainer ueDevs;
     // ueDevs.Add (homeUeDevs);
      ueDevs.Add (macroUeDevs);
      for (uint32_t u = 0; u < ueDevs.GetN (); ++u)
        {
          Ptr<NetDevice> ueDev = ueDevs.Get (u);
          for (uint32_t b = 0; b < numBearersPerUe; ++b)
            {
              enum EpsBearer::Qci q = EpsBearer::NGBR_VIDEO_TCP_DEFAULT;
              EpsBearer bearer (q);
              lteHelper->ActivateDataRadioBearer (ueDev, bearer);
            }
        }
    }

    BuildingsHelper::MakeMobilityModelConsistent ();

    Ptr<RadioEnvironmentMapHelper> remHelper;
  if (generateRem)
    {
     PrintGnuplottableEnbListToFile ("enbs.txt");
      PrintGnuplottableUeListToFile ("ues.txt");

      remHelper = CreateObject<RadioEnvironmentMapHelper> ();
      remHelper->SetAttribute ("ChannelPath", StringValue ("/ChannelList/0"));
      remHelper->SetAttribute ("OutputFile", StringValue ("new-lena-dual-stripe.rem"));
      remHelper->SetAttribute ("XMin", DoubleValue (macroUeBox.xMin));
      remHelper->SetAttribute ("XMax", DoubleValue (macroUeBox.xMax));
      remHelper->SetAttribute ("YMin", DoubleValue (macroUeBox.yMin));
      remHelper->SetAttribute ("YMax", DoubleValue (macroUeBox.yMax));
      remHelper->SetAttribute ("Z", DoubleValue (1.5));


      if (remRbId >= 0)
        {
          remHelper->SetAttribute ("UseDataChannel", BooleanValue (true));
          remHelper->SetAttribute ("RbId", IntegerValue (remRbId));
        }

    remHelper->Install ();
      // simulation will stop right after the REM has been generated
    }
  else
    {
      Simulator::Stop (Seconds (simTime));
    }

  lteHelper->EnablePhyTraces ();
  lteHelper->EnableMacTraces ();
  lteHelper->EnableRlcTraces ();
  if (epc)
    {
      lteHelper->EnablePdcpTraces ();
    }

  Simulator::Run ();

 lteHelper = 0;
  Simulator::Destroy ();
  return 0;
}
