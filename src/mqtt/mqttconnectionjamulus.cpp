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
// TEST
// #include <QFile>

#include <QMetaMethod>
#include <QMqttTopicName>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QThread>
#include <QTranslator>

#include "protocol.h"
#include "serverlist.h"
#include "util.h"

#include "mqttconnectionjamulus.h"

using namespace mqtt;

namespace
{
QString getTopicString ( CServerListManager* serverListManager, const QString& prefix, const QString& type, const QString& suffix )
{
    QString sport ( "0" );
    if ( serverListManager )
    {
        const CServerListEntry* serverInfo = serverListManager->GetServerInfo();
        sport                              = QString::number ( serverInfo->LHostAddr.iPort );
    }
    QString result = prefix + "/" + sport + "/" + type + "/" + suffix;
    return result;
}
}; // namespace

/* ********************************************************************************************************
 * CMqttConnectionExternal
 * ********************************************************************************************************/

CMqttConnectionJamulus::CMqttConnectionJamulus() : Server ( nullptr ), ServerListManager ( nullptr ), MqttClient ( nullptr ), AutoReconnect ( false )
{}

CMqttConnectionJamulus::~CMqttConnectionJamulus() {}

bool CMqttConnectionJamulus::IsConnected() const
{
    bool result = MqttClient != nullptr && MqttClient->state() == QMqttClient::Connected;
    return result;
}

bool CMqttConnectionJamulus::IsDisconnected() const
{
    bool result = MqttClient != nullptr && MqttClient->state() == QMqttClient::Disconnected;
    return result;
}

QMqttTopicName CMqttConnectionJamulus::GetTopicName ( const QString& prefix, const QString& suffix ) const
{
    QString        topic ( getTopicString ( ServerListManager, prefix, GetType(), suffix ) );
    QMqttTopicName result ( topic );
    return result;
}

QMqttTopicFilter CMqttConnectionJamulus::GetTopicFilter ( const QString& prefix, const QString& suffix ) const
{
    QString          topic ( getTopicString ( ServerListManager, prefix, GetType(), suffix ) );
    QMqttTopicFilter result ( topic );
    return result;
}

void CMqttConnectionJamulus::OnMqttConnectedInternal() {}

QJsonDocument CMqttConnectionJamulus::ParseJsonMessage ( QJsonObject& jsonObject, QMqttMessage message ) const
{
    QJsonParseError parseError;
    QJsonDocument   document = QJsonDocument::fromJson ( message.payload(), &parseError );
    jsonObject["status"]     = parseError.error;
    if ( parseError.error == QJsonParseError::NoError )
    {
        QJsonObject messageJson  = document.object();
        QJsonValue  contextValue = messageJson["context"];
        if ( contextValue.isObject() )
        {
            QJsonObject contextObject = contextValue.toObject();
            jsonObject["context"]     = contextObject;
        }
    }
    else
    {
        jsonObject["message"]       = parseError.errorString();
        jsonObject["contextString"] = QString ( message.payload() );
    }
    return document;
}

void CMqttConnectionJamulus::HostAddressToJsonObject ( QJsonObject& jsonObject, CHostAddress hostAddress )
{
    jsonObject["inetAddress"] = hostAddress.InetAddr.toString();
    jsonObject["port"]        = hostAddress.iPort;
}

void CMqttConnectionJamulus::ChannelInfoVectorToJsonArray ( QJsonArray& jsonArray, CVector<CChannelInfo> vecChanInfo )
{
    for ( const CChannelInfo& channelInfo : vecChanInfo )
    {
        QJsonObject channelObject;
        channelObject["channelID"] = channelInfo.iChanID;
        // [--> CDM 25/05/2021
        // channelObject["ipAddress"] = (qint64)channelInfo.iIpAddr;
        // <--] CDM 25/05/2021
        channelObject["name"]           = channelInfo.strName;
        channelObject["country"]        = QLocale::countryToString ( channelInfo.eCountry );
        channelObject["countryCode"]    = channelInfo.eCountry;
        channelObject["city"]           = channelInfo.strCity;
        channelObject["instrument"]     = CInstPictures::GetName ( channelInfo.iInstrument );
        channelObject["instrumentCode"] = channelInfo.iInstrument;
        channelObject["skillLevel"]     = SkillLevelToString ( channelInfo.eSkillLevel );
        channelObject["skillLevelCode"] = channelInfo.eSkillLevel;
        jsonArray.append ( channelObject );
    }
}

QString CMqttConnectionJamulus::SkillLevelToString ( ESkillLevel skillLevel )
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

void CMqttConnectionJamulus::PublishQueuedMessages()
{
    if ( IsConnected() )
    {
        QMutexLocker mutexLocker ( &MutexMessageQueue );
        for ( TMessageQueue::iterator it = MessageQueue.begin(); it != MessageQueue.end(); ++it )
        {
            TMessageQueuePair& pair = *it;
            QJsonDocument      jsonDocument ( pair.second );
            MqttClient->publish ( pair.first, jsonDocument.toJson() );
        }
        MessageQueue.clear();
    }
}

void CMqttConnectionJamulus::PublishOrQueueMessage ( const QMqttTopicName& topicName, const QJsonDocument& jsonDocument )
{
    if ( IsConnected() )
    {
        MqttClient->publish ( topicName, jsonDocument.toJson() );
    }
    else
    {
        QMutexLocker mutexLocker ( &MutexMessageQueue );
        MessageQueue.push_back ( TMessageQueuePair ( topicName, jsonDocument ) );
    }
}

void CMqttConnectionJamulus::PublishOrQueueMessage ( const QMqttTopicName& topicName, const QJsonArray& jsonArray )
{
    CMqttConnectionJamulus::PublishOrQueueMessage ( topicName, QJsonDocument ( jsonArray ) );
}

void CMqttConnectionJamulus::PublishOrQueueMessage ( const QMqttTopicName& topicName, const QJsonObject& jsonObject )
{
    CMqttConnectionJamulus::PublishOrQueueMessage ( topicName, QJsonDocument ( jsonObject ) );
}

void CMqttConnectionJamulus::PublishMessage ( const QMqttTopicName& topicName, const QJsonDocument& jsonDocument )
{
    if ( !IsConnected() )
    {
        throw new std::runtime_error ( ( "Attempt to publish mqtt message while disconnected: " + topicName.name().toStdString() ) );
    }
    MqttClient->publish ( topicName, jsonDocument.toJson() );
}

void CMqttConnectionJamulus::PublishMessage ( const QMqttTopicName& topicName, const QJsonArray& jsonArray )
{
    CMqttConnectionJamulus::PublishMessage ( topicName, QJsonDocument ( jsonArray ) );
}

void CMqttConnectionJamulus::PublishMessage ( const QMqttTopicName& topicName, const QJsonObject& jsonObject )
{
    CMqttConnectionJamulus::PublishMessage ( topicName, QJsonDocument ( jsonObject ) );
}

void CMqttConnectionJamulus::Connect ( const QString& mqttHost, quint16 mqttPort )
{
    if ( MqttClient == nullptr )
    {
        MqttClient = new QMqttClient ( this );

        QObject::connect ( MqttClient, &QMqttClient::connected, this, &CMqttConnectionJamulus::OnMqttConnected );
        QObject::connect ( MqttClient, &QMqttClient::disconnected, this, &CMqttConnectionJamulus::OnMqttDisconnected );
        QObject::connect ( MqttClient, &QMqttClient::messageReceived, this, &CMqttConnectionJamulus::OnMqttMessageReceived );
        QObject::connect ( MqttClient, &QMqttClient::messageStatusChanged, this, &CMqttConnectionJamulus::OnMqttMessageStatusChanged );
        QObject::connect ( MqttClient, &QMqttClient::messageSent, this, &CMqttConnectionJamulus::OnMqttMessageSent );
        QObject::connect ( MqttClient, &QMqttClient::pingResponseReceived, this, &CMqttConnectionJamulus::OnMqttPingResponseReceived );
        QObject::connect ( MqttClient, &QMqttClient::brokerSessionRestored, this, &CMqttConnectionJamulus::OnMqttBrokerSessionRestored );
        qRegisterMetaType<QMqttClient::ClientState> ( "ClientState" );
        QObject::connect ( MqttClient, &QMqttClient::stateChanged, this, &CMqttConnectionJamulus::OnMqttStateChanged );
        qRegisterMetaType<QMqttClient::ClientError> ( "ClientError" );
        QObject::connect ( MqttClient, &QMqttClient::errorChanged, this, &CMqttConnectionJamulus::OnMqttErrorChanged );
        QObject::connect ( MqttClient, &QMqttClient::cleanSessionChanged, this, &CMqttConnectionJamulus::OnMqttCleanSessionChanged );
    }

    if ( !IsDisconnected() )
    {
        AutoReconnect = false;
        MqttClient->disconnectFromHost();
    }

    MqttClient->setHostname ( mqttHost );
    MqttClient->setPort ( mqttPort );

    AutoReconnect = false;
    MqttClient->connectToHost();
}

void CMqttConnectionJamulus::Disconnect()
{
    AutoReconnect = false;
    if ( MqttClient != nullptr )
    {
        MqttClient->disconnectFromHost();
    }
}

void CMqttConnectionJamulus::OnServerStarted ( CServer* server, CServerListManager* serverListManager )
{
    Server            = server;
    ServerListManager = serverListManager;
}
void CMqttConnectionJamulus::OnServerStopped ( CServer* /*server*/, CServerListManager* /*serverListManager*/ ) {}
void CMqttConnectionJamulus::OnStarted() {}
void CMqttConnectionJamulus::OnStopped() {}

/**
 * @brief CMqttConnectionJamulus::OnAboutToQuit End any recording and exit thread
 */
void CMqttConnectionJamulus::OnAboutToQuit()
{
    if ( IsConnected() )
    {
        Disconnect();
    }

    QThread::currentThread()->exit();
}

void CMqttConnectionJamulus::OnMqttConnected()
{
    QMqttTopicFilter   topicFilterPing ( "jamulus/ping" );
    QMqttSubscription* subscriptionPing = MqttClient->subscribe ( topicFilterPing );

    connect ( subscriptionPing, &QMqttSubscription::messageReceived, this, &CMqttConnectionJamulus::OnMqttSubscriptionMessageReceived );
    connect ( subscriptionPing, &QMqttSubscription::stateChanged, this, &CMqttConnectionJamulus::OnMqttSubscriptionStateChanged );
    OnMqttConnectedInternal();
}

void CMqttConnectionJamulus::OnMqttDisconnected()
{
    if ( AutoReconnect )
    {
        MqttClient->connectToHost();
    }
}

void CMqttConnectionJamulus::OnMqttMessageReceived ( const QByteArray& /*message*/, const QMqttTopicName& /*topic*/ ) {}

void CMqttConnectionJamulus::OnMqttMessageStatusChanged ( qint32 /*id*/,
                                                          QMqtt::MessageStatus /*s*/,
                                                          const QMqttMessageStatusProperties& /*properties*/ )
{}

void CMqttConnectionJamulus::OnMqttMessageSent ( qint32 /*id*/ ) {}

void CMqttConnectionJamulus::OnMqttPingResponseReceived() {}

void CMqttConnectionJamulus::OnMqttBrokerSessionRestored() {}

void CMqttConnectionJamulus::OnMqttStateChanged ( QMqttClient::ClientState /*state*/ ) {}

void CMqttConnectionJamulus::OnMqttErrorChanged ( QMqttClient::ClientError /*error*/ ) {}

void CMqttConnectionJamulus::OnMqttCleanSessionChanged ( bool /*cleanSession*/ ) {}

void CMqttConnectionJamulus::OnMqttSubscriptionMessageReceived ( const QMqttMessage& msg )
{
    QMqttTopicFilter topicFilterPing ( "jamulus/ping" );
    if ( topicFilterPing.match ( msg.topic() ) )
    {
        QJsonObject jsonObjectResult;
        // Parse message payload for json context
        QJsonDocument messageDocument = ParseJsonMessage ( jsonObjectResult, msg );
        if ( ServerListManager && !messageDocument.isNull() )
        {
            const CServerListEntry* serverInfo = ServerListManager->GetServerInfo();
            QString                 sport      = QString::number ( serverInfo->LHostAddr.iPort );

            QJsonObject jsonObject;
            jsonObject["port"]    = sport;
            jsonObject["type"]    = GetType();
            jsonObject["name"]    = ServerListManager->GetServerName();
            jsonObject["city"]    = ServerListManager->GetServerCity();
            jsonObject["country"] = QLocale::countryToString ( ServerListManager->GetServerCountry() );

            jsonObjectResult["server"] = jsonObject;
        }
        QMqttTopicName topicName ( "jamulus/pingack" );
        PublishMessage ( topicName, jsonObjectResult );
    }
}

void CMqttConnectionJamulus::OnMqttSubscriptionStateChanged ( QMqttSubscription::SubscriptionState /*state*/ ) {}
