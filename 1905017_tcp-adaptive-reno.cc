#include "tcp-adaptive-reno.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "rtt-estimator.h"
#include "tcp-socket-base.h"

NS_LOG_COMPONENT_DEFINE ("TcpAdaptiveReno");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpAdaptiveReno);

TypeId
TcpAdaptiveReno::GetTypeId (void)
{
  static TypeId tid = TypeId("ns3::TcpAdaptiveReno")
    .SetParent<TcpNewReno>()
    .SetGroupName ("Internet")
    .AddConstructor<TcpAdaptiveReno>()
    .AddAttribute("FilterType", "Use this to choose no filter or Tustin's approximation filter",
                  EnumValue(TcpAdaptiveReno::TUSTIN), MakeEnumAccessor(&TcpAdaptiveReno::m_fType),
                  MakeEnumChecker(TcpAdaptiveReno::NONE, "None", TcpAdaptiveReno::TUSTIN, "Tustin"))
    .AddTraceSource("EstimatedBW", "The estimated bandwidth",
                    MakeTraceSourceAccessor(&TcpAdaptiveReno::m_currentBW),
                    "ns3::TracedValueCallback::Double")
  ;
  return tid;
}


TcpAdaptiveReno::TcpAdaptiveReno(void){
    m_minRtt = m_currentRtt = m_jPacketLRtt = m_conjRtt = m_prevConjRtt = Time(0);
    m_incWnd = m_baseWnd = m_probeWnd = 0;
    NS_LOG_FUNCTION(this);
}


TcpAdaptiveReno::TcpAdaptiveReno(const TcpAdaptiveReno& sock):TcpWestwoodPlus(sock){
    
    m_minRtt = m_currentRtt = m_jPacketLRtt = m_conjRtt = m_prevConjRtt = Time(0);
    m_incWnd = m_baseWnd = m_probeWnd = 0;
    NS_LOG_FUNCTION(this);
    NS_LOG_LOGIC("Invoked the copy constructor");
}

TcpAdaptiveReno::~TcpAdaptiveReno (void)
{
}



void TcpAdaptiveReno::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t packetsAcked,
                                 const Time& rtt)
{
  if (!rtt.IsZero()) {
    m_ackedSegments += packetsAcked;
    
    if (!m_minRtt.IsZero() && rtt <= m_minRtt) {
      m_minRtt = rtt;
    } else if (m_minRtt.IsZero()) {
      m_minRtt = rtt;
    }

    m_currentRtt = rtt;
    TcpWestwoodPlus::EstimateBW(rtt, tcb);
  }
}



double TcpAdaptiveReno::EstimateCongestionLevel()
{
  bool shouldSmooth = m_prevConjRtt < m_minRtt;
  float smoothingFactor = shouldSmooth ? 0 : 0.85;

  double conjRtt = shouldSmooth ? m_prevConjRtt.GetSeconds() : (smoothingFactor * m_prevConjRtt.GetSeconds() + (1 - smoothingFactor) * m_jPacketLRtt.GetSeconds());
  m_conjRtt = Seconds(conjRtt);

  double rttDiff = m_currentRtt.GetSeconds() - m_minRtt.GetSeconds();
  double conjRttDiff = conjRtt - m_minRtt.GetSeconds();

  double congestionRatio = rttDiff / conjRttDiff;
  return std::min(congestionRatio, 1.0);
}





void TcpAdaptiveReno::EstimateIncWnd(Ptr<TcpSocketState> tcb)
{
  double congestion = EstimateCongestionLevel();
  int scalingFactor_m = 1000;
  
  double maxIncWndBitRate = m_currentBW.Get().GetBitRate() / scalingFactor_m;
  double maxIncWndSize = static_cast<double>(tcb->m_segmentSize * tcb->m_segmentSize);
  double m_maxIncWnd = maxIncWndBitRate * maxIncWndSize;
  
  double invAlpha = 0.1; // 1 / alpha
  double expAlpha = std::exp(10); // exp(alpha)
  
  double invAlphaMinusOne = invAlpha - 1;
  
  double beta = 2 * m_maxIncWnd * (invAlphaMinusOne / expAlpha);
  double gamma = 1 - 2 * m_maxIncWnd * (invAlphaMinusOne / expAlpha);
  
  m_incWnd = static_cast<int>((m_maxIncWnd / expAlpha) + (beta * congestion) + gamma);
}





void TcpAdaptiveReno::CongestionAvoidance(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  if (segmentsAcked > 0)
  {
    EstimateIncWnd(tcb);

    double adder = static_cast<double>(tcb->m_segmentSize * tcb->m_segmentSize) / tcb->m_cWnd.Get();
    adder = std::max(1.0, adder);
    m_baseWnd += static_cast<uint32_t>(adder);

    m_probeWnd = std::max(
        (double)(m_probeWnd + m_incWnd / (int)tcb->m_cWnd.Get()),
        (double)0);

    tcb->m_cWnd = m_baseWnd + m_probeWnd;
  }
}


uint32_t TcpAdaptiveReno::GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight)
{
  if (m_prevConjRtt < m_minRtt) {
    m_prevConjRtt = m_conjRtt;
  }
  m_jPacketLRtt = m_currentRtt;
  
  double congestion = EstimateCongestionLevel();

  uint32_t ssthresh = 2 * tcb->m_segmentSize;
  double divisor = 1.0 + congestion;
  double ratio = static_cast<double>(tcb->m_cWnd) / divisor;
  ssthresh = static_cast<uint32_t>(std::max(ratio, static_cast<double>(ssthresh)));

  m_baseWnd = ssthresh;
  m_probeWnd = 0;

  return ssthresh;
}





Ptr<TcpCongestionOps> TcpAdaptiveReno::Fork()
{
  return CreateObject<TcpAdaptiveReno>(*this);
}


} 
