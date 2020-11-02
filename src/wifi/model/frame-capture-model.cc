#include "frame-capture-model.h"

#include <cmath>
#include <iostream>
/**
 *	The model for frame body capture and recovery is a*erf((sinr-b)/c)+d.
 *	a, b, c and d are calibrated based on a hardware test using 802.11p device
 */
namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (FrameCaptureModel);

TypeId
FrameCaptureModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FrameCaptureModel")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
    .AddConstructor<FrameCaptureModel> ()
    ;
    return tid;
}
FrameCaptureModel::FrameCaptureModel ()
{
}

double
FrameCaptureModel::DataCaptureProbability (double sinr)
{
  if (sinr < 0)
    return 0.0;

  double result = 0.0;
  double a = 0.4989;
  double b = 9.356;
  double c = 0.8722;
  double d = 0.5;
  result = a * erf((sinr-b)/c) + d;

  if (result > 1.0)
    {
      result = 1.0;
    }
  else if (result < 0.0)
    {
      result = 0.0;
    }

  return result;
}

double
FrameCaptureModel::DataRecoveryProbability (double sinr)
{
  if (sinr < 0)
    return 0.0;
  
  double result = 0.0;
  double a = 0.4997;
  double b = 3.557;
  double c = 1.292;
  double d = 0.5;
  result = a * erf((sinr-b)/c) + d;

  if (result > 1.0)
    {
      result = 1.0;
    }
  else if (result < 0.0)
    {
      result = 0.0;
    }

  return result;
}
}	
