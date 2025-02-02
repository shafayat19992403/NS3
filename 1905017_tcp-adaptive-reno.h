#ifndef TCP_ADAPTIVERENO_H
#define TCP_ADAPTIVERENO_H

#include "tcp-congestion-ops.h"
#include "tcp-westwood-plus.h"
#include "ns3/tcp-recovery-ops.h"
#include "ns3/sequence-number.h"
#include "ns3/traced-value.h"
#include "ns3/event-id.h"

namespace ns3{
    class Packet;
    class Tcp_Header;
    class Time;
    class Event_Id;

    class TcpAdaptiveReno : public TcpWestwoodPlus
    {
        public:

            static TypeId GetTypeId(void);

            TcpAdaptiveReno(void);
            TcpAdaptiveReno(const TcpAdaptiveReno& sock);
            virtual ~TcpAdaptiveReno(void);

            enum FilterType{
                NONE,TUSTIN
            };

            virtual uint32_t GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight);
            virtual void PktsAcked(Ptr<TcpSocketState> tcb, uint32_t packetsAcked,const Time& rtt);
            virtual Ptr<TcpCongestionOps> Fork();

            private:
                void EstimateBW(const Time& rtt, Ptr<TcpSocketState> tcb);

                double EstimateCongestionLevel();
                void EstimateIncWnd(Ptr<TcpSocketState> tcb);
            protected:
                virtual void CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

                Time                   m_minRtt;                 //!< Minimum RTT
                Time                   m_currentRtt;             //!< Current RTT
                Time                   m_jPacketLRtt;            //!< RTT of j packet loss
                Time                   m_conjRtt;                //!< Conjestion RTT (j th event)
                Time                   m_prevConjRtt;            //!< Previous Conjestion RTT (j-1 th event)

                // Window calculations
                int32_t                m_incWnd;                 //!< Increment Window
                uint32_t               m_baseWnd;                //!< Base Window
                int32_t                m_probeWnd;               //!< Probe Window 
};  

}
#endif










           