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

#include "../server.h"
#include "mqttcontroller.h"
#include "mqttconnection.h"
#include "mqttconnectionrequest.h"

using namespace mqtt;

CMqttController::CMqttController ( CServer* server, const QString& mqttHostpub, quint16 mqttPortPub ) :
    pServer ( server ),
    mqttInetAddress(),
    pthMqttConnection ( nullptr ),
    pMqttConnection ( new CMqttConnection() ),
    pthMqttConnectionRequest ( nullptr ),
    pMqttConnectionRequest ( new CMqttConnectionRequest() )
{
    pthMqttConnection = new QThread();
    pthMqttConnection->setObjectName ( "MqttConnection" );
    pMqttConnection->moveToThread ( pthMqttConnection );

    pthMqttConnectionRequest = new QThread();
    pthMqttConnectionRequest->setObjectName ( "MqttConnectionRequest" );
    pMqttConnectionRequest->moveToThread ( pthMqttConnectionRequest );

    // QT signals
    QObject::connect ( pthMqttConnection, &QThread::finished, pMqttConnection, &QObject::deleteLater );

    QObject::connect ( QCoreApplication::instance(),
                       &QCoreApplication::aboutToQuit,
                       pMqttConnection,
                       &CMqttConnection::OnAboutToQuit,
                       Qt::ConnectionType::BlockingQueuedConnection );

    QObject::connect ( this, &CMqttController::Connect, pMqttConnection, &CMqttConnection::Connect );

    // from the controller to the connection
    QObject::connect ( this,
                       &CMqttController::EndRecorderThread,
                       pMqttConnection,
                       &CMqttConnection::OnEndRecorderThread,
                       Qt::ConnectionType::BlockingQueuedConnection );

    // Proxied from the server to the connection
    QObject::connect ( this, &CMqttController::Started, pMqttConnection, &CMqttConnection::OnStarted );

    QObject::connect ( this, &CMqttController::Stopped, pMqttConnection, &CMqttConnection::OnStopped );

    QObject::connect ( this, &CMqttController::ClientDisconnected, pMqttConnection, &CMqttConnection::OnClientDisconnected );

    qRegisterMetaType<ESvrRegStatus> ( "ESvrRegStatus" );
    QObject::connect ( this, &CMqttController::SvrRegStatusChanged, pMqttConnection, &CMqttConnection::OnSvrRegStatusChanged );

    // qRegisterMetaType<CVector<int16_t>> ( "CVector<int16_t>" );
    // QObject::connect( this, &CMqttController::AudioFrame,
    //     pMqttConnection, &CMqttConnection::OnAudioFrame );

    qRegisterMetaType<COSUtil::EOpSystemType> ( "COSUtil::EOpSystemType" );
    QObject::connect ( this, &CMqttController::CLVersionAndOSReceived, pMqttConnection, &CMqttConnection::OnCLVersionAndOSReceived );

    QObject::connect ( this, &CMqttController::CLPingReceived, pMqttConnection, &mqtt::CMqttConnection::OnCLPingReceived );

    QObject::connect ( this, &CMqttController::RestartRecorder, pMqttConnection, &CMqttConnection::OnRestartRecorder );

    QObject::connect ( this, &CMqttController::StopRecorder, pMqttConnection, &CMqttConnection::OnStopRecorder );

    QObject::connect ( this, &CMqttController::RecordingSessionStarted, pMqttConnection, &CMqttConnection::OnRecordingSessionStarted );

    QObject::connect ( this, &CMqttController::EndRecorderThread, pMqttConnection, &CMqttConnection::OnEndRecorderThread );

    QObject::connect ( this, &CMqttController::ChanInfoHasChanged, pMqttConnection, &CMqttConnection::OnChanInfoHasChanged );

    QObject::connect ( this, &CMqttController::NewConnection, pMqttConnection, &CMqttConnection::OnNewConnection );

    QObject::connect ( this, &CMqttController::ServerFull, pMqttConnection, &CMqttConnection::OnServerFull );

    QObject::connect ( this, &CMqttController::ProtcolCLMessageReceived, pMqttConnection, &CMqttConnection::OnProtcolCLMessageReceived );

    QObject::connect ( this, &CMqttController::ProtcolMessageReceived, pMqttConnection, &CMqttConnection::OnProtcolMessageReceived );

    // from the recorder to the server
    // QObject::connect ( pJamRecorder, &CJamRecorder::RecordingSessionStarted,
    //     this, &CJamController::RecordingSessionStarted );

    QObject::connect ( this, &CMqttController::CLConnClientsListMesReceived, pMqttConnection, &CMqttConnection::OnCLConnClientsListMesReceived );
    qRegisterMetaType<CServer*> ( "CServer" );
    qRegisterMetaType<CServerListManager*> ( "CServerListManager" );
    QObject::connect ( this, &CMqttController::ServerStarted, pMqttConnection, &CMqttConnection::OnServerStarted );
    QObject::connect ( this, &CMqttController::ServerStopped, pMqttConnection, &CMqttConnection::OnServerStopped );

    QObject::connect ( pthMqttConnectionRequest, &QThread::finished, pMqttConnectionRequest, &QObject::deleteLater );

    QObject::connect ( QCoreApplication::instance(),
                       &QCoreApplication::aboutToQuit,
                       pMqttConnectionRequest,
                       &CMqttConnectionRequest::OnAboutToQuit,
                       Qt::ConnectionType::BlockingQueuedConnection );

    QObject::connect ( this, &CMqttController::Connect, pMqttConnectionRequest, &CMqttConnectionRequest::Connect );

    // Proxied from the server to the connection
    QObject::connect ( this, &CMqttController::Started, pMqttConnectionRequest, &CMqttConnectionRequest::OnStarted );
    QObject::connect ( this, &CMqttController::Stopped, pMqttConnectionRequest, &CMqttConnectionRequest::OnStopped );

    QObject::connect ( this, &CMqttController::ServerStarted, pMqttConnectionRequest, &CMqttConnectionRequest::OnServerStarted );
    QObject::connect ( this, &CMqttController::ServerStopped, pMqttConnectionRequest, &CMqttConnectionRequest::OnServerStopped );

    pthMqttConnection->start ( QThread::NormalPriority );
    pthMqttConnectionRequest->start ( QThread::NormalPriority );
    emit Connect ( mqttHostpub, mqttPortPub );
}

CMqttController::~CMqttController() {}

bool CMqttController::IsEnabled() const { return true; }

void CMqttController::OnStarted() { emit Started(); }

void CMqttController::OnStopped() { emit Stopped(); }

void CMqttController::OnClientDisconnected ( const int iChID ) { emit ClientDisconnected ( iChID ); }

void CMqttController::OnSvrRegStatusChanged()
{
    bool          bCurSerListEnabled = pServer->GetServerListEnabled();
    ESvrRegStatus eSvrRegStatus      = pServer->GetSvrRegStatus();
    QString       strStatus          = svrRegStatusToString ( eSvrRegStatus );

    emit SvrRegStatusChanged ( bCurSerListEnabled, eSvrRegStatus, strStatus );
}

// void CMqttController::OnAudioFrame (
//     const int              iChID,
//     const QString          stChName,
//     const CHostAddress     RecHostAddr,
//     const int              iNumAudChan,
//     const CVector<int16_t> vecsData )
// {
//     emit AudioFrame(iChID, stChName, RecHostAddr, iNumAudChan, vecsData);
// }

void CMqttController::OnCLVersionAndOSReceived ( CHostAddress inetAddr, COSUtil::EOpSystemType eOSType, QString strVersion )
{
    emit CLVersionAndOSReceived ( inetAddr, eOSType, strVersion );
}

void CMqttController::OnCLPingReceived ( CHostAddress inetAddr, int iMs ) { emit CLPingReceived ( inetAddr, iMs ); }

void CMqttController::OnRestartRecorder() { emit RestartRecorder(); }

void CMqttController::OnStopRecorder() { emit StopRecorder(); }

void CMqttController::OnRecordingSessionStarted ( QString sessionDir ) { emit RecordingSessionStarted ( sessionDir ); }

void CMqttController::OnEndRecorderThread() { emit EndRecorderThread(); }

void CMqttController::OnChanInfoHasChanged ( CVector<CChannelInfo> vecChanInfo ) { emit ChanInfoHasChanged ( vecChanInfo ); }

void CMqttController::OnNewConnection ( int iChID, CHostAddress recHostAddr ) { emit NewConnection ( iChID, recHostAddr ); }

void CMqttController::OnServerFull ( CHostAddress recHostAddr ) { emit ServerFull ( recHostAddr ); }

void CMqttController::OnProtcolCLMessageReceived ( int iRecID, CVector<uint8_t> vecbyMesBodyData, CHostAddress recHostAddr )
{
    emit ProtcolCLMessageReceived ( iRecID, vecbyMesBodyData, recHostAddr );
}

void CMqttController::OnProtcolMessageReceived ( int iRecCounter, int iRecID, CVector<uint8_t> vecbyMesBodyData, CHostAddress recHostAddr )
{
    emit ProtcolMessageReceived ( iRecCounter, iRecID, vecbyMesBodyData, recHostAddr );
}

void CMqttController::OnServerStarted ( CServer* server, CServerListManager* serverListManager ) { emit ServerStarted ( server, serverListManager ); }

void CMqttController::OnServerStopped ( CServer* server, CServerListManager* serverListManager ) { emit ServerStopped ( server, serverListManager ); }
