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

#include "mqttconnectioncommand.h"

using namespace mqtt;

/* ********************************************************************************************************
 * CMqttConnectionCommand
 * ********************************************************************************************************/

CMqttConnectionCommand::CMqttConnectionCommand() : CMqttConnectionJamulus() {}
CMqttConnectionCommand::~CMqttConnectionCommand() {}

QString CMqttConnectionCommand::GetType() const
{
    static const QString type ( "command" );
    return type;
}

void CMqttConnectionCommand::OnMqttConnectedInternal()
{
    QMqttTopicFilter topicFilter = GetTopicFilter ( "jamulus", "request/#" );
    Subscribe ( topicFilter, &CMqttConnectionCommand::OnMqttSubscriptionMessageReceived, &CMqttConnectionCommand::OnMqttSubscriptionStateChanged );
}

void CMqttConnectionCommand::OnMqttSubscriptionMessageReceived ( const QMqttMessage& msg )
{
    QMqttTopicName topic ( msg.topic() );

    QMqttTopicFilter topicFilterPostRecording = GetTopicFilter ( "jamulus", "request/recording/post/#" );
    if ( topicFilterPostRecording.match ( topic ) )
    {
        if ( Server )
        {
            QJsonObject   jsonObject;
            QJsonDocument messageDocument = ParseJsonMessage ( jsonObject, msg );
            if ( !messageDocument.isNull() )
            {
                QJsonObject messageJson = messageDocument.object();
                QJsonValue  opValue     = messageJson["op"];
                if ( opValue.isString() )
                {
                    QString op = opValue.toString();
                    if ( op == "toggle" )
                    {
                        emit SetEnableRecording ( !Server->GetRecordingEnabled() );
                    }
                    else if ( op == "start" )
                    {
                        emit SetEnableRecording ( true );
                    }
                    else if ( op == "stop" )
                    {
                        emit SetEnableRecording ( false );
                    }
                    else if ( op == "new" )
                    {
                        emit RequestNewRecording();
                    }
                }
            }

            QMqttTopicName topicName = GetTopicName ( "jamulus", "response/recording/post" );
            PublishMessage ( topicName, jsonObject );
        }
    }
}
void CMqttConnectionCommand::OnMqttSubscriptionStateChanged ( QMqttSubscription::SubscriptionState /*state*/ ) {}