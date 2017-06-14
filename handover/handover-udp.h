/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   handover-udp.h
 * Author: soyo
 *
 * Created on 25 May 2017, 3:48 PM
 */


#ifndef HANDOVER_UDP_H
#define HANDOVER_UDP_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/lte-module.h"
#include <ostream>
namespace ns3 {

class Socket;
class Packet;
struct handoverreport {
  uint64_t imsi;
  uint16_t cellId;
  uint16_t  rnti;
  LteRrcSap::MeasurementReport report;
};



class HandoverUdpClient : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  HandoverUdpClient ();

  virtual ~HandoverUdpClient ();

  /**
   * \brief set the remote address and port
   * \param ip remote IP address
   * \param port remote port
   */
  void SetRemote (Address ip, uint16_t port);
  /**
   * \brief set the remote address
   * \param addr remote address
   */
  void SetRemote (Address addr);
  void Send (uint64_t imsi, uint16_t cellId, uint16_t rnti);
  void Sendm(uint64_t imsi, uint16_t cellId, uint16_t rnti, LteRrcSap::MeasurementReport report);  
protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  /**
   * \brief Send a packet
   */

  
  
  uint32_t m_count; //!< Maximum number of packets the application will send
  Time m_interval; //!< Packet inter-send time
  uint32_t m_size; //!< Size of the sent packet (including the SeqTsHeader)

  uint32_t m_sent; //!< Counter for sent packets
  Ptr<Socket> m_socket; //!< Socket
  Address m_peerAddress; //!< Remote peer address
  uint16_t m_peerPort; //!< Remote peer port
  EventId m_sendEvent; //!< Event to send the next packet

};

} // namesp
#endif /* HANDOVER_UDP_H */

