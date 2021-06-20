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
#include "mqttconnectioncommand.h"

using namespace mqtt;

CMqttController::CMqttController ( CServer*       server,
                                   const QString& mqttHostPub,
                                   quint16        iMqttPortPub,
                                   const QString& mqttHostSub,
                                   quint16        iMqttPortSub ) :
    pServer ( server ),
    mqttInetAddress(),
    pthMqttConnection ( nullptr ),
    pMqttConnection ( nullptr ),
    // pMqttConnection ( new CMqttConnection() ),
    pthMqttConnectionCommand ( nullptr ),
    pMqttConnectionCommand ( nullptr )
// pMqttConnectionCommand ( new CMqttConnectionCommand() )
{
    if ( !mqttHostPub.isEmpty() )
    {
        pMqttConnection   = new CMqttConnection();
        pthMqttConnection = new QThread();
        pthMqttConnection->setObjectName ( "MqttConnection" );
        pMqttConnection->moveToThread ( pthMqttConnection );

        // QT signals
        QObject::connect ( pthMqttConnection, &QThread::finished, pMqttConnection, &QObject::deleteLater );

        QObject::connect ( QCoreApplication::instance(),
                           &QCoreApplication::aboutToQuit,
                           pMqttConnection,
                           &CMqttConnection::OnAboutToQuit,
                           Qt::ConnectionType::BlockingQueuedConnection );

        QObject::connect ( this, &CMqttController::ConnectPub, pMqttConnection, &CMqttConnection::Connect );
        // QObject::connect ( this, &CMqttController::Connect, pMqttConnection, &CMqttConnection::Connect );

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

        pthMqttConnection->start ( QThread::NormalPriority );
        emit ConnectPub ( mqttHostPub, iMqttPortPub );
    }

    if ( !mqttHostSub.isEmpty() )
    {
        pMqttConnectionCommand   = new CMqttConnectionCommand();
        pthMqttConnectionCommand = new QThread();
        pthMqttConnectionCommand->setObjectName ( "MqttConnectionCommand" );
        pMqttConnectionCommand->moveToThread ( pthMqttConnectionCommand );

        QObject::connect ( pthMqttConnectionCommand, &QThread::finished, pMqttConnectionCommand, &QObject::deleteLater );

        QObject::connect ( QCoreApplication::instance(),
                           &QCoreApplication::aboutToQuit,
                           pMqttConnectionCommand,
                           &CMqttConnectionCommand::OnAboutToQuit,
                           Qt::ConnectionType::BlockingQueuedConnection );

        QObject::connect ( this, &CMqttController::ConnectSub, pMqttConnectionCommand, &CMqttConnectionCommand::Connect );

        // Proxied from the server to the connection
        QObject::connect ( this, &CMqttController::Started, pMqttConnectionCommand, &CMqttConnectionCommand::OnStarted );
        QObject::connect ( this, &CMqttController::Stopped, pMqttConnectionCommand, &CMqttConnectionCommand::OnStopped );

        QObject::connect ( this, &CMqttController::ServerStarted, pMqttConnectionCommand, &CMqttConnectionCommand::OnServerStarted );
        QObject::connect ( this, &CMqttController::ServerStopped, pMqttConnectionCommand, &CMqttConnectionCommand::OnServerStopped );

        QObject::connect ( pMqttConnectionCommand, &CMqttConnectionCommand::SetEnableRecording, this, &CMqttController::SetEnableRecording );
        QObject::connect ( pMqttConnectionCommand, &CMqttConnectionCommand::RequestNewRecording, this, &CMqttController::RequestNewRecording );

        pthMqttConnectionCommand->start ( QThread::NormalPriority );

        emit ConnectSub ( mqttHostSub, iMqttPortSub );
    }
}

CMqttController::~CMqttController()
{
    delete pMqttConnection;
    delete pthMqttConnection;
    delete pMqttConnectionCommand;
    delete pthMqttConnectionCommand;
}

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

void CMqttController::SetEnableRecording ( bool bNewEnableRecording ) const { pServer->SetEnableRecording ( bNewEnableRecording ); }
void CMqttController::RequestNewRecording() const { pServer->RequestNewRecording(); }
