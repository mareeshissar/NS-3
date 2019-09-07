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
        uint16_t numberOfUEs = 5;
        uint16_t numberOfeNbs = 1;

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();

    NodeContainer enbNodes;
    enbNodes.Create (numberOfeNbs);
    NodeContainer ueNodes;
    ueNodes.Create (numberOfUEs);

    MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (enbNodes);
    mobility.Install (ueNodes);
    BuildingsHelper::Install (enbNodes);
    BuildingsHelper::Install (ueNodes);

    //Install an LTE protocol stack on the UE and eNBs
    NetDeviceContainer enbDevs;
    enbDevs = lteHelper->InstallEnbDevice (enbNodes);
    NetDeviceContainer ueDevs;
    ueDevs = lteHelper->InstallUeDevice (ueNodes);

 
    // Attach the UEs to an eNB.
    for(uint16_t i = 0; i< numberOfUEs; i++)
    {
	lteHelper->Attach (ueDevs.Get(i), enbDevs.Get(0));
 
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



//	lteHelper->EnablePhyTraces ();
//	lteHelper->EnableMacTraces ();
//	lteHelper->EnableRlcTraces ();
//	lteHelper->EnablePdcpTraces ();


  Ptr<RadioEnvironmentMapHelper> remHelper = CreateObject<RadioEnvironmentMapHelper> ();
      remHelper->SetAttribute ("ChannelPath", StringValue ("/ChannelList/0"));
      remHelper->SetAttribute ("OutputFile", StringValue ("my-dual-stripe.out"));
      remHelper->SetAttribute ("XMin", DoubleValue (-12000.0));
      remHelper->SetAttribute ("XMax", DoubleValue (12000.0));
      remHelper->SetAttribute ("YMin", DoubleValue (-12000.0));
      remHelper->SetAttribute ("YMax", DoubleValue (12000.0));
      remHelper->SetAttribute ("Z", DoubleValue (1.5));
      remHelper->Install ();

 Simulator::Stop (Seconds (Time));

    Simulator::Run ();

    Simulator::Destroy ();
    return 0;
}
 
