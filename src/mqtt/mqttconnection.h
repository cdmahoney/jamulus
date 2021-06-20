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

#include "../util.h"
#include "./mqttconnectionjamulus.h"

namespace mqtt
{

using TMessageQueuePair = QPair<QMqttTopicName, QJsonDocument>;
using TMessageQueue     = CVector<TMessageQueuePair>;

class CMqttConnection : public CMqttConnectionJamulus /*QObject*/
{
    Q_OBJECT

public:
    CMqttConnection();
    ~CMqttConnection();

protected:
    virtual QString GetType() const;
    virtual void    OnMqttConnectedInternal();

private:
    QString RecordingDirectory;

signals:

public slots:
    void OnClientDisconnected ( const int iChID );
    void OnSvrRegStatusChanged ( bool enabled, ESvrRegStatus regStatus, const QString& strStatus );
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

    void OnChanInfoHasChanged ( CVector<CChannelInfo> vecChanInfo );

    void OnNewConnection ( int          iChID,
                           CHostAddress RecHostAddr ); // for the server
    void OnServerFull ( CHostAddress RecHostAddr );

    void OnProtcolCLMessageReceived ( int iRecID, CVector<uint8_t> vecbyMesBodyData, CHostAddress RecHostAddr );

    void OnProtcolMessageReceived ( int iRecCounter, int iRecID, CVector<uint8_t> vecbyMesBodyData, CHostAddress RecHostAddr );

    // Protocol events - signal linked to server event
    void OnCLConnClientsListMesReceived ( CHostAddress InetAddr, CVector<CChannelInfo> vecChanInfo );

    void OnMqttSubscriptionMessageReceived ( const QMqttMessage& msg );
    void OnMqttSubscriptionStateChanged ( QMqttSubscription::SubscriptionState state );
};

} // namespace mqtt
