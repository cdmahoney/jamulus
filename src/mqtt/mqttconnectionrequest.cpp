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

#include "mqttconnectionrequest.h"

using namespace mqtt;

namespace
{
// // TEST - write to file
// QFile File ( "D:\\CDM\\Projects\\Jamulus\\20201111\\build-Jamulus-Desktop_Qt_5_15_2_MinGW_64_bit-Debug\\debug\\logs\\qtservermqttexternal.log" );

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
 * CMqttConnectionRequest
 * ********************************************************************************************************/

CMqttConnectionRequest::CMqttConnectionRequest() : CMqttConnectionJamulus()
{
    // File.open ( QIODevice::Append | QIODevice::Text );
}
CMqttConnectionRequest::~CMqttConnectionRequest() {}

QString CMqttConnectionRequest::GetType() const
{
    static const QString type ( "request" );
    return type;
}

void CMqttConnectionRequest::OnMqttConnectedInternal()
{
    // _WriteString ( "CMqttConnectionRequest::OnMqttConnectedInternal" );
    QMqttTopicFilter topicFilter = GetTopicFilter ( "jamulus", "request/#" );
    Subscribe ( topicFilter, &CMqttConnectionRequest::OnMqttSubscriptionMessageReceived, &CMqttConnectionRequest::OnMqttSubscriptionStateChanged );
    // QMqttSubscription* subscription = Subscribe ( topicFilter,
    //                                               &CMqttConnectionRequest::OnMqttSubscriptionMessageReceived,
    //                                               &CMqttConnectionRequest::OnMqttSubscriptionStateChanged );
    // connect ( subscription, &QMqttSubscription::qosChanged, [this] ( quint8 qos ) {
    //     _WriteString ( "CMqttConnectionExternal::OnMqttConnected: " + QString::number ( qos ) );
    // } );
}

void CMqttConnectionRequest::OnMqttSubscriptionMessageReceived ( const QMqttMessage& msg )
{
    // _WriteString ( "CMqttConnectionRequest::OnMqttSubscriptionMessageReceived - Topic: " + msg.topic().name() );
    // _WriteString ( "CMqttConnectionRequest::OnMqttSubscriptionMessageReceived - Payload: " + msg.payload() );

    QMqttTopicName topic ( msg.topic() );

    QMqttTopicFilter topicFilterGetConfiguration = GetTopicFilter ( "jamulus", "request/configuration/get/#" );
    if ( topicFilterGetConfiguration.match ( topic ) )
    {
        // _WriteString ( "CMqttConnectionRequest::OnMqttSubscriptionMessageReceived - topic is configuration/get!" );
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
        // _WriteString ( "CMqttConnectionRequest::OnMqttSubscriptionMessageReceived - topic is connections/get!" );
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
void CMqttConnectionRequest::OnMqttSubscriptionStateChanged ( QMqttSubscription::SubscriptionState /*state*/ )
{
    // _WriteString ( "CMqttConnectionRequest::OnMqttSubscriptionStateChanged: " + QString::number ( state ) );
}