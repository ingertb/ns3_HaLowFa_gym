#ifndef FRAME_CAPTURE_MODEL_H
#define FRAME_CAPTURE_MODEL_H

#include "ns3/object.h"

namespace ns3 {

class FrameCaptureModel : public Object
{
public:
  static TypeId GetTypeId (void);
  FrameCaptureModel ();
  double DataCaptureProbability (double sinr);
  double DataRecoveryProbability (double sinr);
};

} // namespace ns3

#endif /* FRAME_CAPTURE_TRANSITION_RATE_H */
