#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/mobility-module.h>
#include <ns3/lte-module.h>
#include <ns3/buildings-helper.h>
#include <ns3/point-to-point-helper.h>
#include <ns3/netanim-module.h>

using namespace ns3;

int main (int argc, char *argv[])
{
  LogComponentEnable ("LteEnbPhy", LOG_LEVEL_INFO);

        double Time = 0.5;
        uint16_t numberOfUEs = 50;
        uint16_t numberOfeNbs = 10;

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();

    NodeContainer enbNodes;
    enbNodes.Create (numberOfeNbs);
    NodeContainer ueNodes;
    ueNodes.Create (numberOfUEs);

    MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (enbNodes);
    mobility.Install (ueNodes);

//   MobilityHelper mobility;
   // setup the grid itself: objects are laid out
   // started from (-100,-100) with 20 objects per row,
   // the x interval between each object is 5 meters
   // and the y interval between each object is 20 meters
//   mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
//                                  "MinX", DoubleValue (-500.0),
//                                  "MinY", DoubleValue (-500.0),
//                                 "DeltaX", DoubleValue (100.0),
//                                  "DeltaY", DoubleValue (100.0),
//                                "GridWidth", UintegerValue (5),
//                                  "LayoutType", StringValue ("RowFirst"));
   // each object will be attached a static position.
   // i.e., once set by the "position allocator", the
   // position will never change.
//  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

   // finalize the setup by attaching to each object
   // in the input array a position and initializing
   // this position with the calculated coordinates.
//   mobility.Install (enbNodes);
//    mobility.Install (ueNodes);

   // iterate our nodes and print their position.
//   for (NodeContainer::Iterator j = enbNodes.Begin ();
//        j != enbNodes.End (); ++j)
//     {
//       Ptr<Node> object = *j;
//       Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
//       NS_ASSERT (position != 0);
//      Vector pos = position->GetPosition ();
//       std::cout << "x=" << pos.x << ", y=" << pos.y << ", z=" << pos.z << std::endl;
//     }

    //Install an LTE protocol stack on the UE and eNBs
    NetDeviceContainer enbDevs;
    enbDevs = lteHelper->InstallEnbDevice (enbNodes);
    NetDeviceContainer ueDevs;
    ueDevs = lteHelper->InstallUeDevice (ueNodes);

    uint16_t j=0;
    // Attach the UEs to an eNB.
    for(uint16_t i = 0; i< numberOfUEs; i++)
    {
     	if(i%5 == 0 && i!=0)
	{
          j++;
        }
	
	lteHelper->Attach (ueDevs.Get(i), enbDevs.Get(j));
 
    }
    // Activate an EPS Bearer including the setup of the Radio Bearer between an eNB and its attached UE
    enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
    EpsBearer bearer (q);
    lteHelper->ActivateDataRadioBearer (ueDevs, bearer);

//    AnimationInterface anim("anim1.xml");
//    anim.SetConstantPosition(enbNodes.Get(0), 100.0, 2.0);
//    anim.SetConstantPosition(enbNodes.Get(1), 150.0, 50.0);
//    anim.SetConstantPosition(ueNodes.Get(0), 200.0, 100.0);
//    anim.SetConstantPosition(ueNodes.Get(1), 1.0, 1.0);
//    anim.SetConstantPosition(ueNodes.Get(2), 4.0, 3.3);
//    anim.SetConstantPosition(ueNodes.Get(3), 1.0, 1.5);
//    anim.SetConstantPosition(ueNodes.Get(4), 4.0, 3.1);


  // Configure Radio Environment Map (REM) output
   // for LTE-only simulations always use /ChannelList/0 which is the downlink channel
//   Ptr<RadioEnvironmentMapHelper> remHelper = CreateObject<RadioEnvironmentMapHelper> ();
//   remHelper->SetAttribute ("ChannelPath", StringValue ("/ChannelList/0"));
//   remHelper->SetAttribute ("OutputFile", StringValue ("rem.out"));
//   remHelper->SetAttribute ("XMin", DoubleValue (-300.0));
//   remHelper->SetAttribute ("XMax", DoubleValue (400.0));
//   remHelper->SetAttribute ("YMin", DoubleValue (-300.0));
//   remHelper->SetAttribute ("YMax", DoubleValue (300.0));
//   remHelper->SetAttribute ("Z", DoubleValue (0.0));
//   remHelper->Install ();

   Ptr<LteHexGridEnbTopologyHelper> lteHexGridEnbTopologyHelper = CreateObject<LteHexGridEnbTopologyHelper> ();
   lteHexGridEnbTopologyHelper->SetLteHelper (lteHelper);
   lteHexGridEnbTopologyHelper->SetAttribute ("InterSiteDistance", DoubleValue (500.0));
   lteHexGridEnbTopologyHelper->SetAttribute ("MinX", DoubleValue (250));
   lteHexGridEnbTopologyHelper->SetAttribute ("GridWidth", UintegerValue (2));
   Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (46.0));
   lteHelper->SetEnbAntennaModelType ("ns3::ParabolicAntennaModel");
   lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (70));
   lteHelper->SetEnbAntennaModelAttribute ("MaxAttenuation",     DoubleValue (20.0));
   lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (100));
   lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (100 + 18000));
   lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (25));
   lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (25));
   NetDeviceContainer macroEnbDevs = lteHexGridEnbTopologyHelper->SetPositionAndInstallEnbDevice (enbNodes);

      Ptr<RadioEnvironmentMapHelper> remHelper = CreateObject<RadioEnvironmentMapHelper> ();
      remHelper->SetAttribute ("ChannelPath", StringValue ("/ChannelList/0"));
      remHelper->SetAttribute ("OutputFile", StringValue ("my-dual-stripe.out"));
      remHelper->SetAttribute ("XMin", DoubleValue (-1200.0));
      remHelper->SetAttribute ("XMax", DoubleValue (1200.0));
      remHelper->SetAttribute ("YMin", DoubleValue (-1200.0));
      remHelper->SetAttribute ("YMax", DoubleValue (1200.0));
      remHelper->SetAttribute ("Z", DoubleValue (1.5));
      remHelper->Install ();
     
    Simulator::Stop (Seconds (Time));
	
	lteHelper->EnablePhyTraces ();
//	lteHelper->EnableMacTraces ();
//	lteHelper->EnableRlcTraces ();
//	lteHelper->EnablePdcpTraces ();

    Simulator::Run ();

    Simulator::Destroy ();
    return 0;
}
 
