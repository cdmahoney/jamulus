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
#include <QMqttTopicName>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QTranslator>

#include "protocol.h"
#include "server.h"
#include "serverlist.h"
#include "util.h"

#include "mqttconnection.h"

using namespace mqtt;

/* ********************************************************************************************************
 * CMqttConnection
 * ********************************************************************************************************/

CMqttConnection::CMqttConnection() : CMqttConnectionJamulus() {}

CMqttConnection::~CMqttConnection() {}

QString CMqttConnection::GetType() const
{
    static const QString type ( "state" );
    return type;
}

void CMqttConnection::OnMqttConnectedInternal()
{
    QMqttTopicFilter topicFilter = GetTopicFilter ( "jamulus", "request/#" );
    Subscribe ( topicFilter, &CMqttConnection::OnMqttSubscriptionMessageReceived, &CMqttConnection::OnMqttSubscriptionStateChanged );
}

void CMqttConnection::OnMqttSubscriptionMessageReceived ( const QMqttMessage& msg )
{
    QMqttTopicName topic ( msg.topic() );

    QMqttTopicFilter topicFilterGetConfiguration = GetTopicFilter ( "jamulus", "request/configuration/get/#" );
    if ( topicFilterGetConfiguration.match ( topic ) )
    {
        QJsonObject jsonObjectResult;
        // Parse message payload for json context
        QJsonDocument messageDocument = ParseJsonMessage ( jsonObjectResult, msg );
        if ( Server && ServerListManager && !messageDocument.isNull() )
        {
            const CServerListEntry* serverInfo = ServerListManager->GetServerInfo();

            QJsonObject jsonObject;
            // CServerListEntry
            jsonObject["upTime"] = serverInfo->RegisterTime.elapsed();

            // CServerInfo
            QJsonObject jsonObjectInternetAddress;
            HostAddressToJsonObject ( jsonObjectInternetAddress, serverInfo->HostAddr );
            jsonObject["internetAddress"] = jsonObjectInternetAddress;

            QJsonObject jsonObjectInternalAddress;
            HostAddressToJsonObject ( jsonObjectInternalAddress, serverInfo->LHostAddr );
            jsonObject["internalAddress"] = jsonObjectInternalAddress;

            // CServerCoreInfo
            jsonObject["name"]             = serverInfo->strName;
            jsonObject["city"]             = serverInfo->strCity;
            jsonObject["country"]          = QLocale::countryToString ( serverInfo->eCountry );
            jsonObject["countryCode"]      = serverInfo->eCountry;
            jsonObject["maxNumberClients"] = serverInfo->iMaxNumClients;
            jsonObject["permanent"]        = serverInfo->bPermanentOnline;

            // Recording
            QJsonObject jsonObjectRecording;
            jsonObjectRecording["initialised"]  = Server->GetRecorderInitialised();
            jsonObjectRecording["errorMessage"] = Server->GetRecorderErrMsg();
            jsonObjectRecording["enabled"]      = Server->GetRecordingEnabled();
            jsonObjectRecording["directory"]    = Server->GetRecordingDir();
            jsonObject["recording"]             = jsonObjectRecording;

            jsonObjectResult["configuration"] = jsonObject;
        }
        QMqttTopicName topicName = GetTopicName ( "jamulus", "response/configuration/get" );
        PublishMessage ( topicName, jsonObjectResult );
    }

    QMqttTopicFilter topicFilterGetConnections = GetTopicFilter ( "jamulus", "request/connections/get/#" );
    if ( topicFilterGetConnections.match ( topic ) )
    {
        QJsonObject jsonObjectResult;
        // Parse message payload for json context
        QJsonDocument messageDocument = ParseJsonMessage ( jsonObjectResult, msg );
        if ( Server && !messageDocument.isNull() )
        {
            CVector<CChannelInfo> vecChanInfo ( Server->CreateChannelList() );

            // QJsonObject jsonObject;
            QJsonArray channelArray;
            ChannelInfoVectorToJsonArray ( channelArray, vecChanInfo );
            jsonObjectResult["connections"] = channelArray;
        }
        QMqttTopicName topicName = GetTopicName ( "jamulus", "response/connections/get" );
        PublishMessage ( topicName, jsonObjectResult );
    }
}

void CMqttConnection::OnMqttSubscriptionStateChanged ( QMqttSubscription::SubscriptionState /*state*/ ) {}

void CMqttConnection::OnClientDisconnected ( const int iChID )
{
    // _WriteString ( "CMqttConnection::OnClientDisconnected" );
    QJsonObject jsonObject;
    jsonObject["channelID"] = iChID;

    QMqttTopicName topicName = GetTopicName ( "jamulus", "client/disconnected" );
    PublishOrQueueMessage ( topicName, jsonObject );
}

void CMqttConnection::OnSvrRegStatusChanged ( bool enabled, ESvrRegStatus regStatus, const QString& strStatus )
{
    QJsonObject jsonObject;
    jsonObject["enabled"]    = enabled;
    jsonObject["statusCode"] = regStatus;
    jsonObject["status"]     = strStatus;

    QMqttTopicName topicName = GetTopicName ( "jamulus", "registration/status" );
    PublishOrQueueMessage ( topicName, jsonObject );
}
void CMqttConnection::OnCLVersionAndOSReceived ( CHostAddress inetAddr, COSUtil::EOpSystemType eOSType, QString strVersion )
{
    QJsonObject jsonObjectHostAddress;
    HostAddressToJsonObject ( jsonObjectHostAddress, inetAddr );

    QJsonObject jsonObject;
    jsonObject["hostAddress"] = jsonObjectHostAddress;
    jsonObject["osType"]      = eOSType;
    jsonObject["os"]          = COSUtil::GetOperatingSystemString ( eOSType );
    jsonObject["version"]     = strVersion;

    QMqttTopicName topicName = GetTopicName ( "jamulus", "cl/versionandos/receive" );
    PublishOrQueueMessage ( topicName, jsonObject );
}
void CMqttConnection::OnCLPingReceived ( CHostAddress inetAddr, int iMs )
{
    static int previousMs = -1;

    // TODO - Remove this message? (triggers abround 2 messages/second)
    if ( previousMs != -1 )
    {
        QJsonObject jsonObjectHostAddress;
        HostAddressToJsonObject ( jsonObjectHostAddress, inetAddr );

        QJsonObject jsonObject;
        jsonObject["hostAddress"] = jsonObjectHostAddress;
        jsonObject["ms"]          = iMs - previousMs;

        QMqttTopicName topicName = GetTopicName ( "jamulus", "cl/ping/receive" );
        // Uncomment following line to get mqtt messages for cl ping events - this will
        // provoke a lot of messages!
        // PublishOrQueueMessage ( topicName, jsonObject );
    }
    previousMs = iMs;
}

void CMqttConnection::OnRestartRecorder()
{
    QJsonObject jsonObject;

    QMqttTopicName topicName = GetTopicName ( "jamulus", "recording/restart" );
    PublishOrQueueMessage ( topicName, jsonObject );
}

void CMqttConnection::OnStopRecorder()
{
    QJsonObject jsonObject;
    jsonObject["directory"] = RecordingDirectory;

    QMqttTopicName topicName = GetTopicName ( "jamulus", "recording/stop" );
    PublishOrQueueMessage ( topicName, jsonObject );
    RecordingDirectory.clear();
}

void CMqttConnection::OnRecordingSessionStarted ( QString sessionDir )
{
    RecordingDirectory = sessionDir;
    QJsonObject jsonObject;
    jsonObject["directory"] = RecordingDirectory;

    QMqttTopicName topicName = GetTopicName ( "jamulus", "recording/start" );
    PublishOrQueueMessage ( topicName, jsonObject );
}

void CMqttConnection::OnEndRecorderThread()
{
    QJsonObject jsonObject;
    jsonObject["directory"] = RecordingDirectory;

    QMqttTopicName topicName = GetTopicName ( "jamulus", "recording/endthread" );
    PublishOrQueueMessage ( topicName, jsonObject );
}

void CMqttConnection::OnChanInfoHasChanged ( CVector<CChannelInfo> vecChanInfo )
{
    QJsonArray channelArray;
    ChannelInfoVectorToJsonArray ( channelArray, vecChanInfo );

    QMqttTopicName topicName = GetTopicName ( "jamulus", "channelinfo" );
    PublishOrQueueMessage ( topicName, channelArray );
}

void CMqttConnection::OnNewConnection ( int iChID, CHostAddress recHostAddr )
{
    QJsonObject jsonObjectHostAddress;
    HostAddressToJsonObject ( jsonObjectHostAddress, recHostAddr );

    QJsonObject jsonObject;
    jsonObject["channelID"]   = iChID;
    jsonObject["hostAddress"] = jsonObjectHostAddress;

    QMqttTopicName topicName = GetTopicName ( "jamulus", "newconnection" );
    PublishOrQueueMessage ( topicName, jsonObject );
}

void CMqttConnection::OnServerFull ( CHostAddress recHostAddr )
{
    QJsonObject jsonObjectHostAddress;
    HostAddressToJsonObject ( jsonObjectHostAddress, recHostAddr );

    QJsonObject jsonObject;
    jsonObject["hostAddress"] = jsonObjectHostAddress;

    QMqttTopicName topicName = GetTopicName ( "jamulus", "serverfull" );
    PublishOrQueueMessage ( topicName, jsonObject );
}

void CMqttConnection::OnProtcolCLMessageReceived ( int iRecID, CVector<uint8_t> /*vecbyMesBodyData*/, CHostAddress /*recHostAddr*/ )
{
    QString message;
    bool    writeMessage = true;
    switch ( iRecID )
    {
    case PROTMESSID_CLM_PING_MS:
        writeMessage = false;
        break;
    case PROTMESSID_CLM_PING_MS_WITHNUMCLIENTS:
        message = QString ( "PROTMESSID_CLM_PING_MS_WITHNUMCLIENTS (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CLM_SERVER_FULL:
        message = QString ( "PROTMESSID_CLM_SERVER_FULL (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CLM_REGISTER_SERVER:
        message = QString ( "PROTMESSID_CLM_REGISTER_SERVER (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CLM_UNREGISTER_SERVER:
        message = QString ( "PROTMESSID_CLM_UNREGISTER_SERVER (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CLM_SERVER_LIST:
        message = QString ( "PROTMESSID_CLM_SERVER_LIST (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CLM_REQ_SERVER_LIST:
        message = QString ( "PROTMESSID_CLM_REQ_SERVER_LIST (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CLM_SEND_EMPTY_MESSAGE:
        message = QString ( "PROTMESSID_CLM_SEND_EMPTY_MESSAGE (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CLM_EMPTY_MESSAGE:
        message = QString ( "PROTMESSID_CLM_EMPTY_MESSAGE (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CLM_DISCONNECTION:
        message = QString ( "PROTMESSID_CLM_DISCONNECTION (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CLM_VERSION_AND_OS:
        message = QString ( "PROTMESSID_CLM_VERSION_AND_OS (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CLM_REQ_VERSION_AND_OS:
        message = QString ( "PROTMESSID_CLM_REQ_VERSION_AND_OS (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CLM_CONN_CLIENTS_LIST:
        message = QString ( "PROTMESSID_CLM_CONN_CLIENTS_LIST (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CLM_REQ_CONN_CLIENTS_LIST:
        message = QString ( "PROTMESSID_CLM_REQ_CONN_CLIENTS_LIST (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CLM_CHANNEL_LEVEL_LIST:
        message = QString ( "PROTMESSID_CLM_CHANNEL_LEVEL_LIST (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CLM_REGISTER_SERVER_RESP:
        message = QString ( "PROTMESSID_CLM_REGISTER_SERVER_RESP (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CLM_REGISTER_SERVER_EX:
        message = QString ( "PROTMESSID_CLM_REGISTER_SERVER_EX (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CLM_RED_SERVER_LIST:
        message = QString ( "PROTMESSID_CLM_RED_SERVER_LIST (" + QString::number ( iRecID ) + ")" );
        break;
    }
    if ( writeMessage )
    {
        // _WriteString ( "CMqttConnection::OnProtcolCLMessageReceived: " + message );
    }
}

void CMqttConnection::OnProtcolMessageReceived ( int /*iRecCounter*/,
                                                 int iRecID,
                                                 CVector<uint8_t> /*vecbyMesBodyData*/,
                                                 CHostAddress /*recHostAddr*/ )
{
    QString message;
    bool    writeMessage = true;
    switch ( iRecID )
    {
    case PROTMESSID_ILLEGAL:
        message = QString ( "PROTMESSID_ILLEGAL (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_ACKN:
        writeMessage = false;
        break;
    case PROTMESSID_JITT_BUF_SIZE:
        message = QString ( "PROTMESSID_JITT_BUF_SIZE (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_REQ_JITT_BUF_SIZE:
        message = QString ( "PROTMESSID_REQ_JITT_BUF_SIZE (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_NET_BLSI_FACTOR:
        message = QString ( "PROTMESSID_NET_BLSI_FACTOR (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CHANNEL_GAIN:
        message = QString ( "PROTMESSID_CHANNEL_GAIN (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CONN_CLIENTS_LIST_NAME:
        message = QString ( "PROTMESSID_CONN_CLIENTS_LIST_NAME (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_SERVER_FULL:
        message = QString ( "PROTMESSID_SERVER_FULL (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_REQ_CONN_CLIENTS_LIST:
        message = QString ( "PROTMESSID_REQ_CONN_CLIENTS_LIST (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CHANNEL_NAME:
        message = QString ( "PROTMESSID_CHANNEL_NAME (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CHAT_TEXT:
        message = QString ( "PROTMESSID_CHAT_TEXT (" + QString::number ( iRecID ) + ")" );
        break;
    // case PROTMESSID_PING_MS:
    //     message = QString("PROTMESSID_PING_MS (" + QString::number(iRecID) + ")");
    //     break;
    case PROTMESSID_NETW_TRANSPORT_PROPS:
        message = QString ( "PROTMESSID_NETW_TRANSPORT_PROPS (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_REQ_NETW_TRANSPORT_PROPS:
        message = QString ( "PROTMESSID_REQ_NETW_TRANSPORT_PROPS (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_DISCONNECTION:
        message = QString ( "PROTMESSID_DISCONNECTION (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_REQ_CHANNEL_INFOS:
        message = QString ( "PROTMESSID_REQ_CHANNEL_INFOS (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CONN_CLIENTS_LIST:
        message = QString ( "PROTMESSID_CONN_CLIENTS_LIST (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CHANNEL_INFOS:
        message = QString ( "PROTMESSID_CHANNEL_INFOS (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_OPUS_SUPPORTED:
        message = QString ( "PROTMESSID_OPUS_SUPPORTED (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_LICENCE_REQUIRED:
        message = QString ( "PROTMESSID_LICENCE_REQUIRED (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_REQ_CHANNEL_LEVEL_LIST:
        message = QString ( "PROTMESSID_REQ_CHANNEL_LEVEL_LIST (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_VERSION_AND_OS:
        message = QString ( "PROTMESSID_VERSION_AND_OS (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CHANNEL_PAN:
        message = QString ( "PROTMESSID_CHANNEL_PAN (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_MUTE_STATE_CHANGED:
        message = QString ( "PROTMESSID_MUTE_STATE_CHANGED (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_CLIENT_ID:
        message = QString ( "PROTMESSID_CLIENT_ID (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_RECORDER_STATE:
        message = QString ( "PROTMESSID_RECORDER_STATE (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_REQ_SPLIT_MESS_SUPPORT:
        message = QString ( "PROTMESSID_REQ_SPLIT_MESS_SUPPORT (" + QString::number ( iRecID ) + ")" );
        break;
    case PROTMESSID_SPLIT_MESS_SUPPORTED:
        message = QString ( "PROTMESSID_SPLIT_MESS_SUPPORTED (" + QString::number ( iRecID ) + ")" );
        break;
    }
    if ( writeMessage )
    {
        // _WriteString ( "CMqttConnection::OnProtcolMessageReceived: " + message );
    }
}

void CMqttConnection::OnCLConnClientsListMesReceived ( CHostAddress inetAddr, CVector<CChannelInfo> vecChanInfo )
{
    QJsonArray channelArray;
    ChannelInfoVectorToJsonArray ( channelArray, vecChanInfo );

    QJsonObject jsonObjectHostAddress;
    HostAddressToJsonObject ( jsonObjectHostAddress, inetAddr );

    QJsonObject jsonObject;
    jsonObject["hostAddress"] = jsonObjectHostAddress;
    jsonObject["channels"]    = channelArray;

    QMqttTopicName topicName = GetTopicName ( "jamulus", "cl/clientslist/receive" );
    PublishOrQueueMessage ( topicName, jsonObject );
}