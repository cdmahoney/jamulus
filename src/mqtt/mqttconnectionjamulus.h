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
#include <QMqttClient>
#include <QMutex>

#include "../util.h"

class QMqttSubscription;

class CServer;
class CServerListManager;

namespace mqtt
{

// Container class for messages stored awaiting connection
using TMessageQueuePair = QPair<QMqttTopicName, QJsonDocument>;
using TMessageQueue     = CVector<TMessageQueuePair>;

// Base class for Jamulus MQTT connection classes
class CMqttConnectionJamulus : public QObject
{
    Q_OBJECT

public:
    CMqttConnectionJamulus();
    virtual ~CMqttConnectionJamulus();

    bool IsConnected() const;
    bool IsDisconnected() const;

protected:
    QMqttTopicName   GetTopicName ( const QString& prefix, const QString& suffix ) const;
    QMqttTopicFilter GetTopicFilter ( const QString& prefix, const QString& suffix ) const;

    static void    HostAddressToJsonObject ( QJsonObject& jsonObject, CHostAddress hostAddress );
    static void    ChannelInfoVectorToJsonArray ( QJsonArray& jsonArray, CVector<CChannelInfo> vecChanInfo );
    static QString SkillLevelToString ( ESkillLevel skillLevel );

    void PublishQueuedMessages();
    void PublishOrQueueMessage ( const QMqttTopicName& topicName, const QJsonDocument& jsonDocument );
    void PublishOrQueueMessage ( const QMqttTopicName& topicName, const QJsonArray& jsonArray );
    void PublishOrQueueMessage ( const QMqttTopicName& topicName, const QJsonObject& jsonObject );

    void PublishMessage ( const QMqttTopicName& topicName, const QJsonDocument& jsonDocument );
    void PublishMessage ( const QMqttTopicName& topicName, const QJsonArray& jsonArray );
    void PublishMessage ( const QMqttTopicName& topicName, const QJsonObject& jsonObject );

    // See:
    //  https://stackoverflow.com/questions/60000/c-inheritance-and-member-function-pointers
    template<typename T>
    QMqttSubscription* Subscribe ( const QMqttTopicFilter& topicFilter,
                                   void ( T::*onMessageReceived ) ( const QMqttMessage& msg ),
                                   void ( T::*onMessageStatusChanged ) ( QMqttSubscription::SubscriptionState state ) ) const
    {
        QMqttSubscription* result = MqttClient->subscribe ( topicFilter );

        connect ( result, &QMqttSubscription::messageReceived, (T*) this, onMessageReceived );
        connect ( result, &QMqttSubscription::stateChanged, (T*) this, onMessageStatusChanged );

        return result;
    }

    virtual QString GetType() const = 0;
    virtual void    OnMqttConnectedInternal();

protected:
    // External resources not managed by CMqttConnectionJamulus instance
    CServer*            Server;
    CServerListManager* ServerListManager;

private:
    QMqttClient* MqttClient;
    bool         AutoReconnect;

    TMessageQueue MessageQueue;
    QMutex        MutexMessageQueue;

signals:

public slots:
    // MqttController control signals
    // Connect slot allows correct cross-thread connection to mqtt broker (calling Connect from
    // a different thread to that used by MqttConnection provokes problems with pings not getting
    // sent to broker)
    void Connect ( const QString& mqttHost, quint16 mqttPort = 1883 );
    void Disconnect();

    // MqttController signals
    void OnStarted();
    void OnStopped();

    void OnServerStarted ( CServer* server, CServerListManager* serverListManager );
    void OnServerStopped ( CServer* server, CServerListManager* serverListManager );

    /**
     * @brief Handle application stopping
     */
    void OnAboutToQuit();

    // QMqttClient signals
    //  https://doc.qt.io/QtMQTT/qmqttclient.html#signals
    void OnMqttConnected();
    void OnMqttDisconnected();
    void OnMqttMessageReceived ( const QByteArray& message, const QMqttTopicName& topic );
    void OnMqttMessageStatusChanged ( qint32 id, QMqtt::MessageStatus s, const QMqttMessageStatusProperties& properties );
    void OnMqttMessageSent ( qint32 id );
    void OnMqttPingResponseReceived();
    void OnMqttBrokerSessionRestored();

    void OnMqttSubscriptionMessageReceived ( const QMqttMessage& msg );
    void OnMqttSubscriptionStateChanged ( QMqttSubscription::SubscriptionState state );

    void OnMqttStateChanged ( QMqttClient::ClientState state );
    void OnMqttErrorChanged ( QMqttClient::ClientError error );
    void OnMqttCleanSessionChanged ( bool cleanSession );
};

} // namespace mqtt
