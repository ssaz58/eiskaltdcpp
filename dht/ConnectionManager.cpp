/*
 * Copyright (C) 2009 Big Muscle, http://strongdc.sf.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdafx.h"

#include "Constants.h"
#include "ConnectionManager.h"
#include "DHT.h"

#include "../dcpp/ConnectionManager.h"
#include "../dcpp/CryptoManager.h"

namespace dht
{

    ConnectionManager::ConnectionManager(void)
    {
    }

    ConnectionManager::~ConnectionManager(void)
    {
    }

    /*
     * Sends Connect To Me request to online node
     */
    void ConnectionManager::connect(const OnlineUser& ou, const string& token)
    {
        connect(ou, token, CryptoManager::getInstance()->TLSOk() && ou.getUser()->isSet(User::TLS));
    }

    void ConnectionManager::connect(const OnlineUser& ou, const string& token, bool secure)
    {
        bool active = ClientManager::getInstance()->isActive();

        // if I am not active, send reverse connect to me request
        AdcCommand cmd(active ? AdcCommand::CMD_CTM : AdcCommand::CMD_RCM, AdcCommand::TYPE_UDP);
        cmd.addParam(secure ? SECURE_CLIENT_PROTOCOL_TEST : CLIENT_PROTOCOL);

        if(active)
        {
            uint16_t port = secure ? dcpp::ConnectionManager::getInstance()->getSecurePort() : dcpp::ConnectionManager::getInstance()->getPort();
            cmd.addParam(Util::toString(port));
        }

        cmd.addParam(token);

        DHT::getInstance()->send(cmd, ou.getIdentity().getIp(), static_cast<uint16_t>(Util::toInt(ou.getIdentity().getUdpPort())));
    }

    /*
     * Creates connection to specified node
     */
    void ConnectionManager::connectToMe(const Node::Ptr& node, const AdcCommand& cmd)
    {
        const string& protocol = cmd.getParam(1);
        const string& port = cmd.getParam(2);
        const string& token = cmd.getParam(3);

        bool secure = false;
        if(protocol == CLIENT_PROTOCOL)
        {
            // Nothing special
        }
        else if(protocol == SECURE_CLIENT_PROTOCOL_TEST && CryptoManager::getInstance()->TLSOk())
        {
            secure = true;
        }
        else
        {
            AdcCommand cmd(AdcCommand::SEV_FATAL, AdcCommand::ERROR_PROTOCOL_UNSUPPORTED, "Protocol unknown", AdcCommand::TYPE_UDP);
            cmd.addParam("PR", protocol);
            cmd.addParam("TO", token);

            DHT::getInstance()->send(cmd, node->getIdentity().getIp(), static_cast<uint16_t>(Util::toInt(node->getIdentity().getUdpPort())));
            return;
        }

        if(!node->getIdentity().isTcpActive())
        {
            AdcCommand err(AdcCommand::SEV_FATAL, AdcCommand::ERROR_PROTOCOL_GENERIC, "IP unknown", AdcCommand::TYPE_UDP);
            DHT::getInstance()->send(err, node->getIdentity().getIp(), static_cast<uint16_t>(Util::toInt(node->getIdentity().getUdpPort())));
            return;
        }

        dcpp::ConnectionManager::getInstance()->adcConnect(*node, static_cast<uint16_t>(Util::toInt(port)), token, secure);
    }

    /*
     * Sends request to create connection with me
     */
    void ConnectionManager::revConnectToMe(const Node::Ptr& node, const AdcCommand& cmd)
    {
        // this is valid for active-passive connections only
        if(!ClientManager::getInstance()->isActive())
            return;

        const string& protocol = cmd.getParam(0);
        const string& token = cmd.getParam(1);

        bool secure;
        if(protocol == CLIENT_PROTOCOL)
        {
            secure = false;
        }
        else if(protocol == SECURE_CLIENT_PROTOCOL_TEST && CryptoManager::getInstance()->TLSOk())
        {
            secure = true;
        }
        else
        {
            AdcCommand sta(AdcCommand::SEV_FATAL, AdcCommand::ERROR_PROTOCOL_UNSUPPORTED, "Protocol unknown", AdcCommand::TYPE_UDP);
            sta.addParam("PR", protocol);
            sta.addParam("TO", token);

            DHT::getInstance()->send(sta, node->getIdentity().getIp(), static_cast<uint16_t>(Util::toInt(node->getIdentity().getUdpPort())));
            return;
        }

        connect(*node, token, secure);
    }

}