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
// #include <QFile>

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

namespace
{
// QString getTopicString ( CServerListManager* serverListManager, const QString& prefix, const QString& suffix )
// {
//     QString sport ( "0" );
//     if ( serverListManager )
//     {
//         const CServerListEntry* serverInfo = serverListManager->GetServerInfo();
//         sport                              = QString::number ( serverInfo->LHostAddr.iPort );
//     }
//     QString result = prefix + "/" + sport + "/" + suffix;
//     return result;
// }
// QMqttTopicName getTopicName ( CServerListManager* serverListManager, const QString& prefix, const QString& suffix )
// {
//     QString        topic ( getTopicString ( serverListManager, prefix, suffix ) );
//     QMqttTopicName result ( topic );
//     return result;
// }
// QMqttTopicFilter getTopicFilter(CServerListManager* serverListManager, const QString& prefix, const QString& suffix)
// {
//     QString topic(getTopicString(serverListManager, prefix, suffix));
//     QMqttTopicFilter result(topic);
//     return result;
// }

QString skillLevelToString ( ESkillLevel skillLevel )
{
    QString result;
    switch ( skillLevel )
    {
    case SL_BEGINNER:
        result = QCoreApplication::translate ( "CMusProfDlg", "Beginner" );
        break;
    case SL_INTERMEDIATE:
        result = QCoreApplication::translate ( "CMusProfDlg", "Intermediate" );
        break;
    case SL_PROFESSIONAL:
        result = QCoreApplication::translate ( "CMusProfDlg", "Expert" );
        break;
    case SL_NOT_SET:
        result = QCoreApplication::translate ( "CMusProfDlg", "Unknown" );
        break;
    }
    return result;
}

void channelInfoVectorToJsonArray ( QJsonArray& jsonArray, CVector<CChannelInfo> vecChanInfo )
{
    // See:
    //  https://doc.qt.io/qt-5/qtcore-serialization-savegame-example.html
    for ( const CChannelInfo& channelInfo : vecChanInfo )
    {
        QJsonObject channelObject;
        channelObject["channelID"]      = channelInfo.iChanID;
        channelObject["name"]           = channelInfo.strName;
        channelObject["country"]        = QLocale::countryToString ( channelInfo.eCountry );
        channelObject["countryCode"]    = channelInfo.eCountry;
        channelObject["city"]           = channelInfo.strCity;
        channelObject["instrument"]     = CInstPictures::GetName ( channelInfo.iInstrument );
        channelObject["instrumentCode"] = channelInfo.iInstrument;
        channelObject["skillLevel"]     = skillLevelToString ( channelInfo.eSkillLevel );
        channelObject["skillLevelCode"] = channelInfo.eSkillLevel;
        jsonArray.append ( channelObject );
    }
}
void hostAddressToJsonObject ( QJsonObject& jsonObject, CHostAddress hostAddress )
{
    jsonObject["inetAddress"] = hostAddress.InetAddr.toString();
    jsonObject["port"]        = hostAddress.iPort;
}

// TEST - write to file
// QFile File ( "D:\\CDM\\Projects\\Jamulus\\20201111\\build-Jamulus-Desktop_Qt_5_15_2_MinGW_64_bit-Debug\\debug\\logs\\qtservermqtt.log" );

// QString _CurTimeDatetoLogString()
// {
//     // time and date to string conversion
//     const QDateTime curDateTime = QDateTime::currentDateTime();

//     // format date and time output according to "2006-09-30 11:38:08"
//     return curDateTime.toString ( "yyyy-MM-dd HH:mm:ss" );
// };

// void _WriteString ( /*QFile& file, */ const QString& string )
// {
//     QTextStream out ( &File );
//     // QTextStream out ( &file );
//     out << _CurTimeDatetoLogString() << " " << string << endl;
//     File.flush();
// };
}; // namespace

/* ********************************************************************************************************
 * CMqttConnection
 * ********************************************************************************************************/

CMqttConnection::CMqttConnection() : CMqttConnectionJamulus()
{
    // File.open ( QIODevice::Append | QIODevice::Text );
}

CMqttConnection::~CMqttConnection() {}

QString CMqttConnection::GetType() const
{
    static const QString type ( "state" );
    return type;
}

void CMqttConnection::OnMqttConnectedInternal()
{
    // _WriteString ( "CMqttConnection::OnMqttConnectedInternal" );
    QMqttTopicFilter topicFilter = GetTopicFilter ( "jamulus", "request/#" );
    Subscribe ( topicFilter, &CMqttConnection::OnMqttSubscriptionMessageReceived, &CMqttConnection::OnMqttSubscriptionStateChanged );
}

void CMqttConnection::OnMqttSubscriptionMessageReceived ( const QMqttMessage& msg )
{
    // _WriteString ( "CMqttConnection::OnMqttSubscriptionMessageReceived - Topic: " + msg.topic().name() );
    // _WriteString ( "CMqttConnection::OnMqttSubscriptionMessageReceived - Payload: " + msg.payload() );

    QMqttTopicName topic ( msg.topic() );

    QMqttTopicFilter topicFilterGetConfiguration = GetTopicFilter ( "jamulus", "request/configuration/get/#" );
    if ( topicFilterGetConfiguration.match ( topic ) )
    {
        // _WriteString ( "CMqttConnection::OnMqttSubscriptionMessageReceived - topic is configuration/get!" );
        if ( Server && ServerListManager )
        {
            const CServerListEntry* serverInfo = ServerListManager->GetServerInfo();

            QJsonObject jsonObject;
            // CServerListEntry
            jsonObject["upTime"] = serverInfo->RegisterTime.elapsed();

            // CServerInfo
            QJsonObject jsonObjectInternetAddress;
            CMqttConnectionJamulus::HostAddressToJsonObject ( jsonObjectInternetAddress, serverInfo->HostAddr );
            jsonObject["internetAddress"] = jsonObjectInternetAddress;

            QJsonObject jsonObjectInternalAddress;
            CMqttConnectionJamulus::HostAddressToJsonObject ( jsonObjectInternalAddress, serverInfo->LHostAddr );
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

            QMqttTopicName topicName = GetTopicName ( "jamulus", "response/configuration/get" );
            PublishMessage ( topicName, jsonObject );
        }
    }

    QMqttTopicFilter topicFilterGetConnections = GetTopicFilter ( "jamulus", "request/connections/get/#" );
    if ( topicFilterGetConnections.match ( topic ) )
    {
        // _WriteString ( "CMqttConnection::OnMqttSubscriptionMessageReceived - topic is connections/get!" );
        if ( Server )
        {
            CVector<CChannelInfo> vecChanInfo ( Server->CreateChannelList() );

            QJsonObject jsonObject;
            QJsonArray  channelArray;
            ChannelInfoVectorToJsonArray ( channelArray, vecChanInfo );
            jsonObject["connections"] = channelArray;

            QMqttTopicName topicName = GetTopicName ( "jamulus", "response/connections/get" );
            PublishMessage ( topicName, jsonObject );
        }
    }
}

void CMqttConnection::OnMqttSubscriptionStateChanged ( QMqttSubscription::SubscriptionState /*state*/ )
{
    // _WriteString ( "CMqttConnection::OnMqttSubscriptionStateChanged: " + QString::number ( state ) );
}

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
    // _WriteString ( "CMqttConnection::OnSvrRegStatusChanged" );
    QJsonObject jsonObject;
    jsonObject["enabled"]    = enabled;
    jsonObject["statusCode"] = regStatus;
    jsonObject["status"]     = strStatus;

    QMqttTopicName topicName = GetTopicName ( "jamulus", "registration/status" );
    PublishOrQueueMessage ( topicName, jsonObject );
}
// void CMqttConnection::OnAudioFrame (
//     const int              iChID,
//     const QString          stChName,
//     const CHostAddress     RecHostAddr,
//     const int              iNumAudChan,
//     const CVector<int16_t> vecsData )
// {
//     // _WriteString("CMqttConnection::OnAudioFrame");
// }
void CMqttConnection::OnCLVersionAndOSReceived ( CHostAddress inetAddr, COSUtil::EOpSystemType eOSType, QString strVersion )
{
    // _WriteString ( "CMqttConnection::OnCLVersionAndOSReceived" );
    QJsonObject jsonObjectHostAddress;
    hostAddressToJsonObject ( jsonObjectHostAddress, inetAddr );

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
    // _WriteString ( "CMqttConnection::OnCLPingReceived" );
    static int previousMs = -1;

    // TODO - Remove this message? (triggers abround 2 messages/second)
    if ( previousMs != -1 )
    {
        QJsonObject jsonObjectHostAddress;
        hostAddressToJsonObject ( jsonObjectHostAddress, inetAddr );

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
    // _WriteString ( "CMqttConnection::OnRestartRecorder" );
    QJsonObject jsonObject;

    QMqttTopicName topicName = GetTopicName ( "jamulus", "recording/restart" );
    PublishOrQueueMessage ( topicName, jsonObject );
}

void CMqttConnection::OnStopRecorder()
{
    // _WriteString ( "CMqttConnection::OnStopRecorder" );
    QJsonObject jsonObject;
    jsonObject["directory"] = RecordingDirectory;

    QMqttTopicName topicName = GetTopicName ( "jamulus", "recording/stop" );
    PublishOrQueueMessage ( topicName, jsonObject );
    RecordingDirectory.clear();
}

void CMqttConnection::OnRecordingSessionStarted ( QString sessionDir )
{
    // _WriteString ( QString ( "CMqttConnection::OnRecordingSessionStarted: " + sessionDir ) );
    RecordingDirectory = sessionDir;
    QJsonObject jsonObject;
    jsonObject["directory"] = RecordingDirectory;

    QMqttTopicName topicName = GetTopicName ( "jamulus", "recording/start" );
    PublishOrQueueMessage ( topicName, jsonObject );
}

void CMqttConnection::OnEndRecorderThread()
{
    // _WriteString ( "CMqttConnection::OnEndRecorderThread" );
    QJsonObject jsonObject;
    jsonObject["directory"] = RecordingDirectory;

    QMqttTopicName topicName = GetTopicName ( "jamulus", "recording/endthread" );
    PublishOrQueueMessage ( topicName, jsonObject );
}

void CMqttConnection::OnChanInfoHasChanged ( CVector<CChannelInfo> vecChanInfo )
{
    // _WriteString ( "CMqttConnection::OnChanInfoHasChanged" );
    QJsonArray channelArray;
    channelInfoVectorToJsonArray ( channelArray, vecChanInfo );

    QMqttTopicName topicName = GetTopicName ( "jamulus", "channelinfo" );
    PublishOrQueueMessage ( topicName, channelArray );
}

void CMqttConnection::OnNewConnection ( int iChID, CHostAddress recHostAddr )
{
    // _WriteString ( "CMqttConnection::OnNewConnection" );
    QJsonObject jsonObjectHostAddress;
    hostAddressToJsonObject ( jsonObjectHostAddress, recHostAddr );

    QJsonObject jsonObject;
    jsonObject["channelID"]   = iChID;
    jsonObject["hostAddress"] = jsonObjectHostAddress;

    QMqttTopicName topicName = GetTopicName ( "jamulus", "newconnection" );
    PublishOrQueueMessage ( topicName, jsonObject );
}

void CMqttConnection::OnServerFull ( CHostAddress recHostAddr )
{
    QJsonObject jsonObjectHostAddress;
    hostAddressToJsonObject ( jsonObjectHostAddress, recHostAddr );

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
    // _WriteString ( "CMqttConnection::OnCLConnClientsListMesReceived" );
    // See:
    //  https://doc.qt.io/qt-5/qtcore-serialization-savegame-example.html
    QJsonArray channelArray;
    channelInfoVectorToJsonArray ( channelArray, vecChanInfo );

    QJsonObject jsonObjectHostAddress;
    hostAddressToJsonObject ( jsonObjectHostAddress, inetAddr );

    QJsonObject jsonObject;
    jsonObject["hostAddress"] = jsonObjectHostAddress;
    jsonObject["channels"]    = channelArray;

    QMqttTopicName topicName = GetTopicName ( "jamulus", "cl/clientslist/receive" );
    PublishOrQueueMessage ( topicName, jsonObject );
}