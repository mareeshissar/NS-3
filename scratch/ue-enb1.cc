#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/mobility-module.h>
#include <ns3/lte-module.h>
#include <ns3/buildings-helper.h>
#include <ns3/point-to-point-helper.h>

using namespace ns3;

int main (int argc, char *argv[])
{
  LogComponentEnable ("LteEnbPhy", LOG_LEVEL_INFO);

        double Time = 0.5;
        uint16_t j = 0;
        uint16_t h = 1;
        uint16_t numberOfUEs = 5;
        uint16_t numberOfeNbs = 2;

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();

    NodeContainer enbNodes;
    enbNodes.Create (numberOfeNbs);
    NodeContainer ueNodes;
    ueNodes.Create (numberOfUEs);

    MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (enbNodes);
    mobility.Install (ueNodes);

    //Install an LTE protocol stack on the UE and eNBs
    NetDeviceContainer enbDevs;
    enbDevs = lteHelper->InstallEnbDevice (enbNodes);
    NetDeviceContainer ueDevs;
    ueDevs = lteHelper->InstallUeDevice (ueNodes);

    // Attach the UEs to an eNB.
      for (uint16_t i = 0; i < numberOfUEs; i++)
      {
          if ( i % 2 != 0 ){
            lteHelper->Attach (ueDevs.Get(i), enbDevs.Get(j));
        }
        else{
            lteHelper->Attach (ueDevs.Get(i), enbDevs.Get(h));
        }
      }

    // Activate an EPS Bearer including the setup of the Radio Bearer between an eNB and its attached UE
    enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
    EpsBearer bearer (q);
    lteHelper->ActivateDataRadioBearer (ueDevs, bearer);

    Simulator::Stop (Seconds (Time));

 // Configure Radio Environment Map (REM) output
   // for LTE-only simulations always use /ChannelList/0 which is the downlink channel
   Ptr<RadioEnvironmentMapHelper> remHelper = CreateObject<RadioEnvironmentMapHelper> ();
   remHelper->SetAttribute ("ChannelPath", StringValue ("/ChannelList/0"));
   remHelper->SetAttribute ("OutputFile", StringValue ("rem1.out"));
   remHelper->SetAttribute ("XMin", DoubleValue (-100.0));
   remHelper->SetAttribute ("XMax", DoubleValue (400.0));
   remHelper->SetAttribute ("YMin", DoubleValue (-100.0));
   remHelper->SetAttribute ("YMax", DoubleValue (300.0));
   remHelper->SetAttribute ("Z", DoubleValue (0.0));
   remHelper->Install ();

    Simulator::Run ();

    Simulator::Destroy ();
    return 0;
}

