/******************************************************************************\
 * Copyright (c) 2020
 *
 * Author(s):
 *  cdmahoney
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#pragma once

#include <QObject>

#include "../util.h"
#include "../protocol.h"

class CServer;
class CServerListManager;

namespace mqtt
{

class CMqttConnection;
class CMqttConnectionRequest;

class CMqttController : public QObject
{
    Q_OBJECT

public:
    explicit CMqttController ( CServer* server, const QString& mqttpub, quint16 iMqttPortPub );
    virtual ~CMqttController();

    bool IsEnabled() const;

private:
    CServer*                pServer;
    CHostAddress            mqttInetAddress;
    QThread*                pthMqttConnection;
    CMqttConnection*        pMqttConnection;
    QThread*                pthMqttConnectionRequest;
    CMqttConnectionRequest* pMqttConnectionRequest;

signals:
    void EndConnectionThread();

    // MqttConnection control
    void Connect ( const QString& mqttHost, quint16 mqttPort = 1883 );

    // Proxied server signals
    void Started();
    void Stopped();
    void ClientDisconnected ( const int iChID );
    void SvrRegStatusChanged ( bool enabled, ESvrRegStatus regStatus, const QString& strStatus );
    // void AudioFrame ( const int              iChID,
    //                   const QString          stChName,
    //                   const CHostAddress     RecHostAddr,
    //                   const int              iNumAudChan,
    //                   const CVector<int16_t> vecsData );
    void CLVersionAndOSReceived ( CHostAddress InetAddr, COSUtil::EOpSystemType eOSType, QString strVersion );
    void CLPingReceived ( CHostAddress inetAddr, int iMs );
    void RestartRecorder();
    void StopRecorder();
    void RecordingSessionStarted ( QString sessionDir );
    void EndRecorderThread();

    // Proxied socket signals
    // void NewConnection(); // for the client
    void NewConnection ( int          iChID,
                         CHostAddress RecHostAddr ); // for the server
    void ServerFull ( CHostAddress RecHostAddr );

    // Channel events
    void ChanInfoHasChanged ( CVector<CChannelInfo> vecChanInfo );

    // Protocol events - signal linked to server event
    void CLConnClientsListMesReceived ( CHostAddress InetAddr, CVector<CChannelInfo> vecChanInfo );

    // Socket events. Copied from server
    void ProtcolCLMessageReceived ( int iRecID, CVector<uint8_t> vecbyMesBodyData, CHostAddress RecHostAddr );

    void ProtcolMessageReceived ( int iRecCounter, int iRecID, CVector<uint8_t> vecbyMesBodyData, CHostAddress RecHostAddr );
    void ServerStarted ( CServer* server, CServerListManager* serverListManager );
    void ServerStopped ( CServer* server, CServerListManager* serverListManager );

public slots:
    // Server signals
    void OnStarted();
    void OnStopped();
    void OnClientDisconnected ( const int iChID );
    void OnSvrRegStatusChanged();
    // void OnAudioFrame ( const int              iChID,
    //                   const QString          stChName,
    //                   const CHostAddress     RecHostAddr,
    //                   const int              iNumAudChan,
    //                   const CVector<int16_t> vecsData );
    void OnCLVersionAndOSReceived ( CHostAddress InetAddr, COSUtil::EOpSystemType eOSType, QString strVersion );
    void OnCLPingReceived ( CHostAddress inetAddr, int iMs );
    void OnRestartRecorder();
    void OnStopRecorder();
    void OnRecordingSessionStarted ( QString sessionDir );
    void OnEndRecorderThread();

    // Channel events (via server)
    void OnChanInfoHasChanged ( CVector<CChannelInfo> vecChanInfo );

    // Socket signals
    // void OnNewConnection(); // for the client
    void OnNewConnection ( int          iChID,
                           CHostAddress RecHostAddr ); // for the server
    void OnServerFull ( CHostAddress RecHostAddr );
    void OnProtcolCLMessageReceived ( int iRecID, CVector<uint8_t> vecbyMesBodyData, CHostAddress RecHostAddr );

    void OnProtcolMessageReceived ( int iRecCounter, int iRecID, CVector<uint8_t> vecbyMesBodyData, CHostAddress RecHostAddr );
    void OnServerStarted ( CServer* server, CServerListManager* serverListManager );
    void OnServerStopped ( CServer* server, CServerListManager* serverListManager );
};

} // namespace mqtt

// Q_DECLARE_METATYPE(int16_t)
