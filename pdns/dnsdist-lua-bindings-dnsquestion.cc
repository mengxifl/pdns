/*
 * This file is part of PowerDNS or dnsdist.
 * Copyright -- PowerDNS.COM B.V. and its contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * In addition, for the avoidance of any doubt, permission is granted to
 * link this program with OpenSSL and to (re)distribute the binaries
 * produced as the result of such linking.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "dnsdist.hh"
#include "dnsdist-ecs.hh"
#include "dnsdist-lua.hh"
#include "dnsparser.hh"

void setupLuaBindingsDNSQuestion(LuaContext& luaCtx)
{
  /* DNSQuestion */
  /* PowerDNS DNSQuestion compat */
  luaCtx.registerMember<const ComboAddress (DNSQuestion::*)>("localaddr", [](const DNSQuestion& dq) -> const ComboAddress { return *dq.local; }, [](DNSQuestion& dq, const ComboAddress newLocal) { (void) newLocal; });
  luaCtx.registerMember<const DNSName (DNSQuestion::*)>("qname", [](const DNSQuestion& dq) -> const DNSName { return *dq.qname; }, [](DNSQuestion& dq, const DNSName newName) { (void) newName; });
  luaCtx.registerMember<uint16_t (DNSQuestion::*)>("qtype", [](const DNSQuestion& dq) -> uint16_t { return dq.qtype; }, [](DNSQuestion& dq, uint16_t newType) { (void) newType; });
  luaCtx.registerMember<uint16_t (DNSQuestion::*)>("qclass", [](const DNSQuestion& dq) -> uint16_t { return dq.qclass; }, [](DNSQuestion& dq, uint16_t newClass) { (void) newClass; });
  luaCtx.registerMember<int (DNSQuestion::*)>("rcode", [](const DNSQuestion& dq) -> int { return dq.dh->rcode; }, [](DNSQuestion& dq, int newRCode) { dq.dh->rcode = newRCode; });
  luaCtx.registerMember<const ComboAddress (DNSQuestion::*)>("remoteaddr", [](const DNSQuestion& dq) -> const ComboAddress { return *dq.remote; }, [](DNSQuestion& dq, const ComboAddress newRemote) { (void) newRemote; });
  /* DNSDist DNSQuestion */
  luaCtx.registerMember("dh", &DNSQuestion::dh);
  luaCtx.registerMember<uint16_t (DNSQuestion::*)>("len", [](const DNSQuestion& dq) -> uint16_t { return dq.len; }, [](DNSQuestion& dq, uint16_t newlen) { dq.len = newlen; });
  luaCtx.registerMember<uint8_t (DNSQuestion::*)>("opcode", [](const DNSQuestion& dq) -> uint8_t { return dq.dh->opcode; }, [](DNSQuestion& dq, uint8_t newOpcode) { (void) newOpcode; });
  luaCtx.registerMember<size_t (DNSQuestion::*)>("size", [](const DNSQuestion& dq) -> size_t { return dq.size; }, [](DNSQuestion& dq, size_t newSize) { (void) newSize; });
  luaCtx.registerMember<bool (DNSQuestion::*)>("tcp", [](const DNSQuestion& dq) -> bool { return dq.tcp; }, [](DNSQuestion& dq, bool newTcp) { (void) newTcp; });
  luaCtx.registerMember<bool (DNSQuestion::*)>("skipCache", [](const DNSQuestion& dq) -> bool { return dq.skipCache; }, [](DNSQuestion& dq, bool newSkipCache) { dq.skipCache = newSkipCache; });
  luaCtx.registerMember<bool (DNSQuestion::*)>("useECS", [](const DNSQuestion& dq) -> bool { return dq.useECS; }, [](DNSQuestion& dq, bool useECS) { dq.useECS = useECS; });
  luaCtx.registerMember<bool (DNSQuestion::*)>("ecsOverride", [](const DNSQuestion& dq) -> bool { return dq.ecsOverride; }, [](DNSQuestion& dq, bool ecsOverride) { dq.ecsOverride = ecsOverride; });
  luaCtx.registerMember<uint16_t (DNSQuestion::*)>("ecsPrefixLength", [](const DNSQuestion& dq) -> uint16_t { return dq.ecsPrefixLength; }, [](DNSQuestion& dq, uint16_t newPrefixLength) { dq.ecsPrefixLength = newPrefixLength; });
  luaCtx.registerMember<boost::optional<uint32_t> (DNSQuestion::*)>("tempFailureTTL",
      [](const DNSQuestion& dq) -> boost::optional<uint32_t> {
        return dq.tempFailureTTL;
      },
      [](DNSQuestion& dq, boost::optional<uint32_t> newValue) {
        dq.tempFailureTTL = newValue;
      }
    );
  luaCtx.registerFunction<bool(DNSQuestion::*)()const>("getDO", [](const DNSQuestion& dq) {
      return getEDNSZ(dq) & EDNS_HEADER_FLAG_DO;
    });

  luaCtx.registerFunction<std::map<uint16_t, EDNSOptionView>(DNSQuestion::*)()const>("getEDNSOptions", [](const DNSQuestion& dq) {
      if (dq.ednsOptions == nullptr) {
        parseEDNSOptions(dq);
      }

      return *dq.ednsOptions;
    });
  luaCtx.registerFunction<std::string(DNSQuestion::*)(void)const>("getTrailingData", [](const DNSQuestion& dq) {
      return dq.getTrailingData();
    });
  luaCtx.registerFunction<bool(DNSQuestion::*)(std::string)>("setTrailingData", [](DNSQuestion& dq, const std::string& tail) {
      return dq.setTrailingData(tail);
    });

  luaCtx.registerFunction<std::string(DNSQuestion::*)()const>("getServerNameIndication", [](const DNSQuestion& dq) {
      return dq.sni;
    });

  luaCtx.registerFunction<void(DNSQuestion::*)(std::string)>("sendTrap", [](const DNSQuestion& dq, boost::optional<std::string> reason) {
#ifdef HAVE_NET_SNMP
      if (g_snmpAgent && g_snmpTrapsEnabled) {
        g_snmpAgent->sendDNSTrap(dq, reason ? *reason : "");
      }
#endif /* HAVE_NET_SNMP */
    });

  luaCtx.registerFunction<void(DNSQuestion::*)(std::string, std::string)>("setTag", [](DNSQuestion& dq, const std::string& strLabel, const std::string& strValue) {
      if(dq.qTag == nullptr) {
        dq.qTag = std::make_shared<QTag>();
      }
      dq.qTag->insert({strLabel, strValue});
    });
  luaCtx.registerFunction<void(DNSQuestion::*)(vector<pair<string, string>>)>("setTagArray", [](DNSQuestion& dq, const vector<pair<string, string>>&tags) {
      if (!dq.qTag) {
        dq.qTag = std::make_shared<QTag>();
      }

      for (const auto& tag : tags) {
        dq.qTag->insert({tag.first, tag.second});
      }
    });
  luaCtx.registerFunction<string(DNSQuestion::*)(std::string)const>("getTag", [](const DNSQuestion& dq, const std::string& strLabel) {
      if (!dq.qTag) {
        return string();
      }

      std::string strValue;
      const auto it = dq.qTag->find(strLabel);
      if (it == dq.qTag->cend()) {
        return string();
      }
      return it->second;
    });
  luaCtx.registerFunction<QTag(DNSQuestion::*)(void)const>("getTagArray", [](const DNSQuestion& dq) {
      if (!dq.qTag) {
        QTag empty;
        return empty;
      }

      return *dq.qTag;
    });

  luaCtx.registerFunction<void(DNSQuestion::*)(std::vector<std::pair<uint8_t, std::string>>)>("setProxyProtocolValues", [](DNSQuestion& dq, const std::vector<std::pair<uint8_t, std::string>>& values) {
      if (!dq.proxyProtocolValues) {
        dq.proxyProtocolValues = make_unique<std::vector<ProxyProtocolValue>>();
      }

      dq.proxyProtocolValues->clear();
      dq.proxyProtocolValues->reserve(values.size());
      for (const auto& value : values) {
        dq.proxyProtocolValues->push_back({value.second, value.first});
      }
    });

  /* LuaWrapper doesn't support inheritance */
  luaCtx.registerMember<const ComboAddress (DNSResponse::*)>("localaddr", [](const DNSResponse& dq) -> const ComboAddress { return *dq.local; }, [](DNSResponse& dq, const ComboAddress newLocal) { (void) newLocal; });
  luaCtx.registerMember<const DNSName (DNSResponse::*)>("qname", [](const DNSResponse& dq) -> const DNSName { return *dq.qname; }, [](DNSResponse& dq, const DNSName newName) { (void) newName; });
  luaCtx.registerMember<uint16_t (DNSResponse::*)>("qtype", [](const DNSResponse& dq) -> uint16_t { return dq.qtype; }, [](DNSResponse& dq, uint16_t newType) { (void) newType; });
  luaCtx.registerMember<uint16_t (DNSResponse::*)>("qclass", [](const DNSResponse& dq) -> uint16_t { return dq.qclass; }, [](DNSResponse& dq, uint16_t newClass) { (void) newClass; });
  luaCtx.registerMember<int (DNSResponse::*)>("rcode", [](const DNSResponse& dq) -> int { return dq.dh->rcode; }, [](DNSResponse& dq, int newRCode) { dq.dh->rcode = newRCode; });
  luaCtx.registerMember<const ComboAddress (DNSResponse::*)>("remoteaddr", [](const DNSResponse& dq) -> const ComboAddress { return *dq.remote; }, [](DNSResponse& dq, const ComboAddress newRemote) { (void) newRemote; });
  luaCtx.registerMember<dnsheader* (DNSResponse::*)>("dh", [](const DNSResponse& dr) -> dnsheader* { return dr.dh; }, [](DNSResponse& dr, dnsheader * newdh) { dr.dh = newdh; });
  luaCtx.registerMember<uint16_t (DNSResponse::*)>("len", [](const DNSResponse& dq) -> uint16_t { return dq.len; }, [](DNSResponse& dq, uint16_t newlen) { dq.len = newlen; });
  luaCtx.registerMember<uint8_t (DNSResponse::*)>("opcode", [](const DNSResponse& dq) -> uint8_t { return dq.dh->opcode; }, [](DNSResponse& dq, uint8_t newOpcode) { (void) newOpcode; });
  luaCtx.registerMember<size_t (DNSResponse::*)>("size", [](const DNSResponse& dq) -> size_t { return dq.size; }, [](DNSResponse& dq, size_t newSize) { (void) newSize; });
  luaCtx.registerMember<bool (DNSResponse::*)>("tcp", [](const DNSResponse& dq) -> bool { return dq.tcp; }, [](DNSResponse& dq, bool newTcp) { (void) newTcp; });
  luaCtx.registerMember<bool (DNSResponse::*)>("skipCache", [](const DNSResponse& dq) -> bool { return dq.skipCache; }, [](DNSResponse& dq, bool newSkipCache) { dq.skipCache = newSkipCache; });
  luaCtx.registerFunction<void(DNSResponse::*)(std::function<uint32_t(uint8_t section, uint16_t qclass, uint16_t qtype, uint32_t ttl)> editFunc)>("editTTLs", [](const DNSResponse& dr, std::function<uint32_t(uint8_t section, uint16_t qclass, uint16_t qtype, uint32_t ttl)> editFunc) {
        editDNSPacketTTL((char*) dr.dh, dr.len, editFunc);
      });
  luaCtx.registerFunction<std::string(DNSResponse::*)(void)const>("getTrailingData", [](const DNSResponse& dq) {
      return dq.getTrailingData();
    });
  luaCtx.registerFunction<bool(DNSResponse::*)(std::string)>("setTrailingData", [](DNSResponse& dq, const std::string& tail) {
      return dq.setTrailingData(tail);
    });

  luaCtx.registerFunction<void(DNSResponse::*)(std::string, std::string)>("setTag", [](DNSResponse& dr, const std::string& strLabel, const std::string& strValue) {
      if(dr.qTag == nullptr) {
        dr.qTag = std::make_shared<QTag>();
      }
      dr.qTag->insert({strLabel, strValue});
    });

  luaCtx.registerFunction<void(DNSResponse::*)(vector<pair<string, string>>)>("setTagArray", [](DNSResponse& dr, const vector<pair<string, string>>&tags) {
      if (!dr.qTag) {
        dr.qTag = std::make_shared<QTag>();
      }

      for (const auto& tag : tags) {
        dr.qTag->insert({tag.first, tag.second});
      }
    });
  luaCtx.registerFunction<string(DNSResponse::*)(std::string)const>("getTag", [](const DNSResponse& dr, const std::string& strLabel) {
      if (!dr.qTag) {
        return string();
      }

      std::string strValue;
      const auto it = dr.qTag->find(strLabel);
      if (it == dr.qTag->cend()) {
        return string();
      }
      return it->second;
    });
  luaCtx.registerFunction<QTag(DNSResponse::*)(void)const>("getTagArray", [](const DNSResponse& dr) {
      if (!dr.qTag) {
        QTag empty;
        return empty;
      }

      return *dr.qTag;
    });

  luaCtx.registerFunction<void(DNSResponse::*)(std::string)>("sendTrap", [](const DNSResponse& dr, boost::optional<std::string> reason) {
#ifdef HAVE_NET_SNMP
      if (g_snmpAgent && g_snmpTrapsEnabled) {
        g_snmpAgent->sendDNSTrap(dr, reason ? *reason : "");
      }
#endif /* HAVE_NET_SNMP */
    });

#ifdef HAVE_DNS_OVER_HTTPS
    luaCtx.registerFunction<std::string(DNSQuestion::*)(void)const>("getHTTPPath", [](const DNSQuestion& dq) {
      if (dq.du == nullptr) {
        return std::string();
      }
      return dq.du->getHTTPPath();
    });

    luaCtx.registerFunction<std::string(DNSQuestion::*)(void)const>("getHTTPQueryString", [](const DNSQuestion& dq) {
      if (dq.du == nullptr) {
        return std::string();
      }
      return dq.du->getHTTPQueryString();
    });

    luaCtx.registerFunction<std::string(DNSQuestion::*)(void)const>("getHTTPHost", [](const DNSQuestion& dq) {
      if (dq.du == nullptr) {
        return std::string();
      }
      return dq.du->getHTTPHost();
    });

    luaCtx.registerFunction<std::string(DNSQuestion::*)(void)const>("getHTTPScheme", [](const DNSQuestion& dq) {
      if (dq.du == nullptr) {
        return std::string();
      }
      return dq.du->getHTTPScheme();
    });

    luaCtx.registerFunction<std::unordered_map<std::string, std::string>(DNSQuestion::*)(void)const>("getHTTPHeaders", [](const DNSQuestion& dq) {
      if (dq.du == nullptr) {
        return std::unordered_map<std::string, std::string>();
      }
      return dq.du->getHTTPHeaders();
    });

    luaCtx.registerFunction<void(DNSQuestion::*)(uint16_t statusCode, const std::string& body, const boost::optional<std::string> contentType)>("setHTTPResponse", [](DNSQuestion& dq, uint16_t statusCode, const std::string& body, const boost::optional<std::string> contentType) {
      if (dq.du == nullptr) {
        return;
      }
      dq.du->setHTTPResponse(statusCode, body, contentType ? *contentType : "");
    });
#endif /* HAVE_DNS_OVER_HTTPS */

  luaCtx.registerFunction<bool(DNSQuestion::*)(bool nxd, const std::string& zone, uint32_t ttl, const std::string& mname, const std::string& rname, uint32_t serial, uint32_t refresh, uint32_t retry, uint32_t expire, uint32_t minimum)>("setNegativeAndAdditionalSOA", [](DNSQuestion& dq, bool nxd, const std::string& zone, uint32_t ttl, const std::string& mname, const std::string& rname, uint32_t serial, uint32_t refresh, uint32_t retry, uint32_t expire, uint32_t minimum) {
      return setNegativeAndAdditionalSOA(dq, nxd, DNSName(zone), ttl, DNSName(mname), DNSName(rname), serial, refresh, retry, expire, minimum);
    });
}
