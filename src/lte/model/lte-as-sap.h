/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 * Copyright (c) 2016, University of Padova, Dep. of Information Engineering, SIGNET lab
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 *
 * Modified by: Michele Polese <michele.polese@gmail.com>
 *          Dual Connectivity functionalities
 */


#ifndef LTE_AS_SAP_H
#define LTE_AS_SAP_H

#include <stdint.h>
#include <ns3/ptr.h>
#include <ns3/packet.h>
#include <ns3/lte-rrc-sap.h>
#include <ns3/lte-rrc-sap.h> //sjkang
namespace ns3 {

class LteEnbNetDevice;

/**
 * This class implements the Access Stratum (AS) Service Access Point
 * (SAP), i.e., the interface between the EpcUeNas and the LteUeRrc.
 * In particular, this class implements the
 * Provider part of the SAP, i.e., the methods exported by the
 * LteUeRrc and called by the EpcUeNas.
 * 
 */
class LteAsSapProvider
{
public:
  virtual ~LteAsSapProvider ();

  /**
   * \brief Set the selected Closed Subscriber Group subscription list to be
   *        used for cell selection.
   *
   * \param csgId identity of the subscribed CSG
   */
  virtual void SetCsgWhiteList (uint32_t csgId) = 0;

  /**
   * \brief Initiate Idle mode cell selection procedure.
   *
   * \param dlEarfcn the downlink carrier frequency (EARFCN)
   */
  virtual void StartCellSelection (uint16_t dlEarfcn) = 0;

  /** 
   * \brief Force the RRC entity to stay camped on a certain eNodeB.
   *
   * \param cellId the cell ID identifying the eNodeB
   * \param dlEarfcn the downlink carrier frequency (EARFCN)
   */
  virtual void ForceCampedOnEnb (uint16_t cellId, uint16_t dlEarfcn) = 0;

  /**
   * \brief Tell the RRC entity to enter Connected mode.
   *
   * If this function is called when the UE is in a situation where connecting
   * is not possible (e.g. before the simulation begin), then the UE will
   * attempt to connect at the earliest possible time (e.g. after it camps to a
   * suitable cell).
   */
  virtual void Connect (void) = 0;

  /** 
   * \brief Send a data packet.
   *
   * \param packet the packet
   * \param bid the EPS bearer ID
   */
  virtual void SendData (Ptr<Packet> packet, uint8_t bid) = 0;


  /** 
   * \brief Tell the RRC entity to release the connection.
   *
   */
  virtual void Disconnect () = 0;

  /** 
   * \brief Tell the RRC that a secondary cell was connected
   *
   */
  virtual void NotifySecondaryCellConnected (uint16_t rnti,uint16_t rnti2, uint16_t mmWaveCellId1, uint16_t mmWaveCellId2) = 0;
  virtual void NotifySecondaryCellConnected (uint16_t rnti,uint16_t mmWaveCellId) = 0;

  /** 
   * \brief Tell the LTE RRC in the UE that a secondary cell handover was performed,
   * and trigger the RLC instances update
   *
   */
  virtual void NotifySecondaryCellHandover (uint16_t oldRnti, uint16_t newRnti, uint16_t mmWaveCellId, LteRrcSap::RadioResourceConfigDedicated radioResourceConfigDedicated) = 0;
  virtual void SendRrcConnectionReconfigurationMessageToMmWaveRrc(LteRrcSap::RrcConnectionReconfiguration msg) =0; //sjkang0417

};


/**
 * This class implements the Access Stratum (AS) Service Access Point
 * (SAP), i.e., the interface between the EpcUeNas and the LteUeRrc
 * In particular, this class implements the 
 * User part of the SAP, i.e., the methods exported by the
 * EpcUeNas and called by the LteUeRrc.
 * 
 */
class LteAsSapUser
{
public:
  virtual ~LteAsSapUser ();

  /** 
   * \brief Notify the NAS that RRC Connection Establishment was successful.
   * 
   */
  virtual void NotifyConnectionSuccessful (uint16_t rnti) = 0;

  virtual void NotifyHandoverSuccessful (uint16_t rnti, uint16_t mmWaveCellId) = 0;

  virtual void NotifySecondaryCellHandoverStarted (uint16_t oldRnti, uint16_t newRnti, uint16_t mmWaveCellId, LteRrcSap::RadioResourceConfigDedicated radioResourceConfigDedicated) = 0;

  /** 
   * \brief Notify the NAS that LTE RRC received an indication to connect to a MmWave eNB
   * 
   */
  virtual void NotifyConnectToMmWave (uint16_t mmWaveCellId, uint16_t mmWaveCellId_2) = 0; //sjkang
  virtual void SendRrcReconfigurationMessageforHandover (LteRrcSap::RrcConnectionReconfiguration msg) =0;
  /** 
   * \brief Notify the NAS that RRC Connection Establishment failed.
   * 
   */
  virtual void NotifyConnectionFailed () = 0;


  /** 
   * Notify the NAS that RRC Connection was released
   * 
   */
  virtual void NotifyConnectionReleased () = 0;

  /** 
   * receive a data packet
   * 
   * \param packet the packet
   */
  virtual void RecvData (Ptr<Packet> packet) = 0;

};




/**
 * Template for the implementation of the LteAsSapProvider as a member
 * of an owner class of type C to which all methods are forwarded
 * 
 */
template <class C>
class MemberLteAsSapProvider : public LteAsSapProvider
{
public:
  MemberLteAsSapProvider (C* owner);

  // inherited from LteAsSapProvider
  virtual void SetCsgWhiteList (uint32_t csgId);
  virtual void StartCellSelection (uint16_t dlEarfcn);
  virtual void ForceCampedOnEnb (uint16_t cellId, uint16_t dlEarfcn);
  virtual void Connect (void);
  virtual void SendData (Ptr<Packet> packet, uint8_t bid);
  virtual void Disconnect ();
  virtual void NotifySecondaryCellConnected (uint16_t rnti,uint16_t rnti2, uint16_t mmWaveCellId, uint16_t mmWaveCellId2);
  virtual void NotifySecondaryCellConnected (uint16_t rnti,uint16_t mmWaveCellId); //sjkang

  virtual void NotifySecondaryCellHandover (uint16_t oldRnti, uint16_t newRnti, uint16_t mmWaveCellId, LteRrcSap::RadioResourceConfigDedicated radioResourceConfigDedicated);
  virtual void SendRrcConnectionReconfigurationMessageToMmWaveRrc(LteRrcSap::RrcConnectionReconfiguration msg); //sjkang0417
private:
  MemberLteAsSapProvider ();
  C* m_owner;
};

template <class C>
MemberLteAsSapProvider<C>::MemberLteAsSapProvider (C* owner)
  : m_owner (owner)
{
}

template <class C>
MemberLteAsSapProvider<C>::MemberLteAsSapProvider ()
{
}

template <class C>
void
MemberLteAsSapProvider<C>::SetCsgWhiteList (uint32_t csgId)
{
  m_owner->DoSetCsgWhiteList (csgId);
}

template <class C>
void
MemberLteAsSapProvider<C>::StartCellSelection (uint16_t dlEarfcn)
{
  m_owner->DoStartCellSelection (dlEarfcn);
}

template <class C>
void
MemberLteAsSapProvider<C>::ForceCampedOnEnb (uint16_t cellId, uint16_t dlEarfcn)
{
  m_owner->DoForceCampedOnEnb (cellId, dlEarfcn);
}

template <class C>
void 
MemberLteAsSapProvider<C>::Connect ()
{
  m_owner->DoConnect ();
}

template <class C>
void
MemberLteAsSapProvider<C>::SendData (Ptr<Packet> packet, uint8_t bid)
{
  m_owner->DoSendData (packet, bid);
}

template <class C>
void 
MemberLteAsSapProvider<C>::Disconnect ()
{
  m_owner->DoDisconnect ();
}

template <class C>
void 
MemberLteAsSapProvider<C>::NotifySecondaryCellConnected (uint16_t rnti1, uint16_t rnti2, uint16_t mmWaveCellId1, uint16_t mmWaveCellId2) //sjkang0710
{
  m_owner->DoNotifySecondaryCellConnected (rnti1,rnti2,  mmWaveCellId1, mmWaveCellId2);
}
template <class C>
void
MemberLteAsSapProvider<C>::NotifySecondaryCellConnected (uint16_t rnti,  uint16_t mmWaveCellId) //sjkang0710
{
  m_owner->DoNotifySecondaryCellConnected (rnti, mmWaveCellId);
}
template <class C>
void
MemberLteAsSapProvider<C>::SendRrcConnectionReconfigurationMessageToMmWaveRrc(LteRrcSap::RrcConnectionReconfiguration msg){ //sjkang0417
	m_owner->DoRecvRrcConnectionReconfiguration(msg);
}
template <class C>
void 
MemberLteAsSapProvider<C>::NotifySecondaryCellHandover (uint16_t oldRnti, uint16_t newRnti, uint16_t mmWaveCellId, LteRrcSap::RadioResourceConfigDedicated radioResourceConfigDedicated)
{
  m_owner->DoNotifySecondaryCellHandover (oldRnti, newRnti, mmWaveCellId, radioResourceConfigDedicated);
}


/**
 * Template for the implementation of the LteAsSapUser as a member
 * of an owner class of type C to which all methods are forwarded
 * 
 */
template <class C>
class MemberLteAsSapUser : public LteAsSapUser
{
public:
  MemberLteAsSapUser (C* owner);

  // inherited from LteAsSapUser
  virtual void NotifyConnectionSuccessful (uint16_t rnti);
  virtual void NotifyHandoverSuccessful (uint16_t rnti, uint16_t mmWaveCellId);
  virtual void NotifyConnectToMmWave (uint16_t mmWaveCellId, uint16_t mmWaveCellId_2); //sjkang0205
  virtual void SendRrcReconfigurationMessageforHandover(LteRrcSap::RrcConnectionReconfiguration msg); //sjkang0417
  virtual void NotifyConnectionFailed ();
  virtual void RecvData (Ptr<Packet> packet);
  virtual void NotifyConnectionReleased ();
  virtual void NotifySecondaryCellHandoverStarted (uint16_t oldRnti, uint16_t newRnti, uint16_t mmWaveCellId, LteRrcSap::RadioResourceConfigDedicated radioResourceConfigDedicated);

private:
  MemberLteAsSapUser ();
  C* m_owner;
};

template <class C>
MemberLteAsSapUser<C>::MemberLteAsSapUser (C* owner)
  : m_owner (owner)
{
}

template <class C>
MemberLteAsSapUser<C>::MemberLteAsSapUser ()
{
}

template <class C>
void 
MemberLteAsSapUser<C>::NotifyConnectionSuccessful (uint16_t rnti)
{
  m_owner->DoNotifyConnectionSuccessful (rnti);
}

template <class C>
void 
MemberLteAsSapUser<C>::NotifyHandoverSuccessful (uint16_t rnti, uint16_t mmWaveCellId)
{
  m_owner->DoNotifyHandoverSuccessful (rnti, mmWaveCellId);
}

template <class C>
void 
MemberLteAsSapUser<C>::NotifyConnectToMmWave (uint16_t mmWaveCellId, uint16_t mmWaveCellId_2) //sjkang0205
{
  m_owner->DoNotifyConnectToMmWave (mmWaveCellId, mmWaveCellId_2);
}
template <class C>
void 
MemberLteAsSapUser<C>::SendRrcReconfigurationMessageforHandover (LteRrcSap::RrcConnectionReconfiguration msg) //sjkang0205
{
  m_owner->DoRecvRrcConnectionReconfigurationMessageforHandover( msg); //sjkang
}
template <class C>
void
MemberLteAsSapUser<C>::NotifyConnectionFailed ()
{
  m_owner->DoNotifyConnectionFailed ();
}

template <class C>
void 
MemberLteAsSapUser<C>::RecvData (Ptr<Packet> packet)
{
  m_owner->DoRecvData (packet);
}

template <class C>
void 
MemberLteAsSapUser<C>::NotifyConnectionReleased ()
{
  m_owner->DoNotifyConnectionReleased ();
}


template <class C>
void 
MemberLteAsSapUser<C>::NotifySecondaryCellHandoverStarted (uint16_t oldRnti, uint16_t newRnti, uint16_t mmWaveCellId, LteRrcSap::RadioResourceConfigDedicated radioResourceConfigDedicated)
{
  m_owner->DoNotifySecondaryCellHandoverStarted (oldRnti, newRnti, mmWaveCellId, radioResourceConfigDedicated);
}

} // namespace ns3

#endif // LTE_AS_SAP_H
