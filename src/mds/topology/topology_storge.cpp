/*
 * Project:
 * Created Date: Mon Sep 03 2018
 * Author: xuchaojie
 * Copyright (c) 2018 netease
 */

#include "src/mds/topology/topology_storge.h"

#include <unordered_map>
#include <vector>
#include <map>
#include <set>
#include <string>

#include "src/repo/repo.h"
#include "src/repo/repoItem.h"
#include "src/repo/dataBase.h"
#include "json/json.h"


namespace curve {
namespace mds {
namespace topology {

using ::curve::repo::ExecUpdateOK;
using ::curve::repo::QueryOK;
using ::curve::repo::ConnectOK;
using ::curve::repo::LogicalPoolRepo;
using ::curve::repo::PhysicalPoolRepo;
using ::curve::repo::LogicalPoolRepo;
using ::curve::repo::ZoneRepo;
using ::curve::repo::ServerRepo;
using ::curve::repo::ChunkServerRepo;
using ::curve::repo::CopySetRepo;


bool DefaultTopologyStorage::init(const std::string &dbName,
    const std::string &user,
    const std::string &url,
    const std::string &password) {
    if (repo_->connectDB(dbName, user, url, password) != ConnectOK) {
        return false;
    } else if (repo_->createDatabase() != ExecUpdateOK) {
        return false;
    } else if (repo_->useDataBase() != ExecUpdateOK) {
        return false;
    } else if (repo_->createAllTables() != ExecUpdateOK) {
        return false;
    }
    return true;
}

bool DefaultTopologyStorage::LoadLogicalPool(
    std::unordered_map<PoolIdType,
    LogicalPool> *logicalPoolMap,
    PoolIdType *maxLogicalPoolId) {
    std::vector<LogicalPoolRepo> logicalPoolRepos;
    if (repo_->LoadLogicalPoolRepos(&logicalPoolRepos) != QueryOK) {
        return false;
    }
    logicalPoolMap->clear();
    *maxLogicalPoolId = 0;
    for (LogicalPoolRepo& rp : logicalPoolRepos) {
        LogicalPool::RedundanceAndPlaceMentPolicy rap;
        LogicalPool::UserPolicy policy;

        Json::Reader reader;
        Json::Value rapJson;
        if (!reader.parse(rp.redundanceAndPlacementPolicy, rapJson)) {
            return false;
        }

        switch (rp.type) {
            case LogicalPoolType::PAGEFILE: {
                if (!rapJson["replicaNum"].isNull()) {
                    rap.pageFileRAP.replicaNum =
                        rapJson["replicaNum"].asInt();
                } else {
                    return false;
                }
                if (!rapJson["copysetNum"].isNull()) {
                    rap.pageFileRAP.copysetNum =
                        rapJson["copysetNum"].asInt();
                } else {
                    return false;
                }
                if (!rapJson["zoneNum"].isNull()) {
                    rap.pageFileRAP.zoneNum = rapJson["zoneNum"].asInt();
                } else {
                    return false;
                }
                break;
            }
            case LogicalPoolType::APPENDFILE: {
                // TODO(xuchaojie): it is not done.
                return false;
                break;
            }
            case LogicalPoolType::APPENDECFILE: {
                // TODO(xuchaojie): it is not done.
                return false;
                break;
            }
            default: {
                return false;
                break;
            }
        }


        // TODO(xuchaojie): parse JSON String to fill policy objects
        LogicalPool pool(rp.logicalPoolID,
                rp.logicalPoolName,
                rp.physicalPoolID,
                static_cast<LogicalPoolType>(rp.type),
                rap,
                policy,
                rp.createTime);
        logicalPoolMap->emplace(std::make_pair(rp.logicalPoolID, pool));

        if (rp.logicalPoolID > *maxLogicalPoolId) {
            *maxLogicalPoolId = rp.logicalPoolID;
        }
    }
    return true;
}

bool DefaultTopologyStorage::LoadPhysicalPool(
    std::unordered_map<PoolIdType, PhysicalPool> *physicalPoolMap,
    PoolIdType *maxPhysicalPoolId) {
    std::vector<PhysicalPoolRepo> physicalPoolRepos;
    if (repo_->LoadPhysicalPoolRepos(&physicalPoolRepos) != QueryOK) {
        return false;
    }
    physicalPoolMap->clear();
    *maxPhysicalPoolId = 0;
    for (PhysicalPoolRepo& rp : physicalPoolRepos) {
        PhysicalPool pool(rp.physicalPoolID,
                rp.physicalPoolName,
                rp.desc);
        physicalPoolMap->emplace(std::make_pair(rp.physicalPoolID, pool));
        if (rp.physicalPoolID > *maxPhysicalPoolId) {
            *maxPhysicalPoolId = rp.physicalPoolID;
        }
    }
    return true;
}

bool DefaultTopologyStorage::LoadZone(
    std::unordered_map<ZoneIdType, Zone> *zoneMap,
    ZoneIdType *maxZoneId) {
    std::vector<ZoneRepo> zoneRepos;
    if (repo_->LoadZoneRepos(&zoneRepos) != QueryOK) {
        return false;
    }
    zoneMap->clear();
    *maxZoneId = 0;
    for (ZoneRepo& rp : zoneRepos) {
        Zone zone(rp.zoneID,
                rp.zoneName,
                rp.poolID,
                rp.desc);
        zoneMap->emplace(std::make_pair(rp.zoneID, zone));
        if (rp.zoneID > *maxZoneId) {
            *maxZoneId = rp.zoneID;
        }
    }
    return true;
}

bool DefaultTopologyStorage::LoadServer(
    std::unordered_map<ServerIdType, Server> *serverMap,
    ServerIdType *maxServerId) {
    std::vector<ServerRepo> ServerRepos;
    if (repo_->LoadServerRepos(&ServerRepos) != QueryOK) {
        return false;
    }
    serverMap->clear();
    *maxServerId = 0;
    for (ServerRepo& rp : ServerRepos) {
        Server server(rp.serverID,
                rp.hostName,
                rp.internalHostIP,
                rp.externalHostIP,
                rp.zoneID,
                rp.poolID,
                rp.desc);
        serverMap->emplace(std::make_pair(rp.serverID, server));
        if (rp.serverID > *maxServerId) {
            *maxServerId = rp.serverID;
        }
    }
    return true;
}

bool DefaultTopologyStorage::LoadChunkServer(
    std::unordered_map<ChunkServerIdType, ChunkServer> *chunkServerMap,
    ChunkServerIdType *maxChunkServerId) {
    std::vector<ChunkServerRepo> chunkServerRepos;
    if (repo_->LoadChunkServerRepos(&chunkServerRepos) != QueryOK) {
        return false;
    }
    chunkServerMap->clear();
    *maxChunkServerId = 0;
    for (ChunkServerRepo& rp : chunkServerRepos) {
        ChunkServer cs(rp.chunkServerID,
                rp.token,
                rp.diskType,
                rp.serverID,
                rp.internalHostIP,
                rp.port,
                rp.mountPoint,
                static_cast<ChunkServerStatus>(rp.rwstatus));
        ChunkServerState csState;
        csState.SetDiskState(static_cast<DiskState>(rp.diskState));
        csState.SetOnlineState(static_cast<OnlineState>(rp.onlineState));
        csState.SetDiskCapacity(rp.capacity);
        csState.SetDiskUsed(rp.used);
        cs.SetChunkServerState(csState);
        chunkServerMap->emplace(std::make_pair(rp.chunkServerID, cs));
        if (rp.chunkServerID > *maxChunkServerId) {
            *maxChunkServerId = rp.chunkServerID;
        }
    }
    return true;
}

bool DefaultTopologyStorage::LoadCopySet(
    std::map<CopySetKey, CopySetInfo> *copySetMap,
    std::map<PoolIdType, CopySetIdType> *copySetIdMaxMap) {
    std::vector<CopySetRepo> copySetRepos;
    if (repo_->LoadCopySetRepos(&copySetRepos) != QueryOK) {
        return false;
    }
    copySetMap->clear();
    copySetIdMaxMap->clear();
    for (CopySetRepo& rp : copySetRepos) {
        CopySetInfo copyset(rp.logicalPoolID, rp.copySetID);
        std::set<ChunkServerIdType> idList;
        Json::Reader reader;
        Json::Value copysetMemJson;
        if (!reader.parse(rp.chunkServerIDList, copysetMemJson)) {
            return false;
        }
        for (int i = 0; i < copysetMemJson.size(); i++) {
            idList.insert(copysetMemJson[i].asInt());
        }
        copyset.SetCopySetMembers(idList);
        if ((*copySetIdMaxMap)[rp.logicalPoolID] < rp.copySetID) {
            (*copySetIdMaxMap)[rp.logicalPoolID] = rp.copySetID;
        }
    }
    return true;
}

bool DefaultTopologyStorage::StorageLogicalPool(const LogicalPool &data) {
    LogicalPool::RedundanceAndPlaceMentPolicy rap =
        data.GetRedundanceAndPlaceMentPolicy();
    LogicalPool::UserPolicy policy = data.GetUserPolicy();
    Json::Value rapJson;
    switch (data.GetLogicalPoolType()) {
        case LogicalPoolType::PAGEFILE : {
            rapJson["replicaNum"] = rap.pageFileRAP.replicaNum;
            rapJson["copysetNum"] = rap.pageFileRAP.copysetNum;
            rapJson["zoneNum"] = rap.pageFileRAP.zoneNum;
            break;
        }
        case LogicalPoolType::APPENDFILE : {
            // TODO(xuchaojie): fix it
            return false;
            break;
        }
        case LogicalPoolType::APPENDECFILE : {
            // TODO(xuchaojie): fix it
            return false;
            break;
        }
        default:
            return false;
            break;
    }
    std::string rapStr = rapJson.toStyledString();

    // TODO(xuchaojie) Parse policy to JSON string
    LogicalPoolRepo rp(data.GetId(),
        data.GetName(),
        data.GetPhysicalPoolId(),
        data.GetLogicalPoolType(),
        data.GetCreateTime(),
        data.GetStatus(),
        rapStr,
        "");
    if (repo_->InsertLogicalPoolRepo(rp) != ExecUpdateOK) {
        return false;
    }
    return true;
}

bool DefaultTopologyStorage::StoragePhysicalPool(const PhysicalPool &data) {
    PhysicalPoolRepo rp(data.GetId(),
         data.GetName(),
         data.GetDesc());
    if (repo_->InsertPhysicalPoolRepo(rp) != ExecUpdateOK) {
        return false;
    }
    return true;
}

bool DefaultTopologyStorage::StorageZone(const Zone &data) {
    ZoneRepo rp(data.GetId(),
        data.GetName(),
        data.GetPhysicalPoolId(),
        data.GetDesc());
    if (repo_->InsertZoneRepo(rp) != ExecUpdateOK) {
        return false;
    }
    return true;
}

bool DefaultTopologyStorage::StorageServer(const Server &data) {
    ServerRepo rp(data.GetId(),
        data.GetHostName(),
        data.GetInternalHostIp(),
        data.GetExternalHostIp(),
        data.GetZoneId(),
        data.GetPhysicalPoolId(),
        data.GetDesc());
    if (repo_->InsertServerRepo(rp) != ExecUpdateOK) {
        return false;
    }
    return true;
}

bool DefaultTopologyStorage::StorageChunkServer(const ChunkServer &data) {
    ChunkServerState csState = data.GetChunkServerState();
    ChunkServerRepo rp(data.GetId(),
            data.GetToken(),
            data.GetDiskType(),
            data.GetHostIp(),
            data.GetPort(),
            data.GetServerId(),
            data.GetStatus(),
            csState.GetDiskState(),
            csState.GetOnlineState(),
            data.GetMountPoint(),
            csState.GetDiskCapacity(),
            csState.GetDiskUsed());

    if (repo_->InsertChunkServerRepo(rp) != ExecUpdateOK) {
        return false;
    }
    return true;
}

bool DefaultTopologyStorage::StorageCopySet(const CopySetInfo &data) {
    Json::Value copysetMemJson;
    for (ChunkServerIdType id : data.GetCopySetMembers()) {
        copysetMemJson.append(id);
    }
    std::string chunkServerListStr = copysetMemJson.toStyledString();

    CopySetRepo rp(data.GetId(),
        data.GetLogicalPoolId(),
        chunkServerListStr);
    if (repo_->InsertCopySetRepo(rp) != ExecUpdateOK) {
        return false;
    }
    return true;
}

bool DefaultTopologyStorage::DeleteLogicalPool(PoolIdType id) {
    if (repo_->DeleteLogicalPoolRepo(id) != ExecUpdateOK) {
        return false;
    }
    return true;
}

bool DefaultTopologyStorage::DeletePhysicalPool(PoolIdType id) {
    if (repo_->DeletePhysicalPoolRepo(id) != ExecUpdateOK) {
        return false;
    }
    return true;
}

bool DefaultTopologyStorage::DeleteZone(ZoneIdType id) {
    if (repo_->DeleteZoneRepo(id) != ExecUpdateOK) {
        return false;
    }
    return true;
}

bool DefaultTopologyStorage::DeleteServer(ServerIdType id) {
    if (repo_->DeleteServerRepo(id) != ExecUpdateOK) {
        return false;
    }
    return true;
}

bool DefaultTopologyStorage::DeleteChunkServer(ChunkServerIdType id) {
    if (repo_->DeleteChunkServerRepo(id) != ExecUpdateOK) {
        return false;
    }
    return true;
}

bool DefaultTopologyStorage::DeleteCopySet(CopySetKey key) {
    if (repo_->DeleteCopySetRepo(key.second, key.first) != ExecUpdateOK) {
        return false;
    }
    return true;
}

bool DefaultTopologyStorage::UpdateLogicalPool(const LogicalPool &data) {
    LogicalPool::RedundanceAndPlaceMentPolicy rap =
        data.GetRedundanceAndPlaceMentPolicy();
    LogicalPool::UserPolicy policy = data.GetUserPolicy();
    Json::Value rapJson;
    switch (data.GetLogicalPoolType()) {
        case LogicalPoolType::PAGEFILE : {
            rapJson["replicaNum"] = rap.pageFileRAP.replicaNum;
            rapJson["copysetNum"] = rap.pageFileRAP.copysetNum;
            rapJson["zoneNum"] = rap.pageFileRAP.zoneNum;
            break;
        }
        case LogicalPoolType::APPENDFILE : {
            // TODO(xuchaojie): fix it
            return false;
            break;
        }
        case LogicalPoolType::APPENDECFILE : {
            // TODO(xuchaojie): fix it
            return false;
            break;
        }
        default:
            return false;
            break;
    }
    std::string rapStr = rapJson.toStyledString();

    // TODO(xuchaojie) Parse policy to JSON string
    LogicalPoolRepo rp(data.GetId(),
        data.GetName(),
        data.GetPhysicalPoolId(),
        data.GetLogicalPoolType(),
        data.GetCreateTime(),
        data.GetStatus(),
        rapStr,
        "");
    if (repo_->UpdateLogicalPoolRepo(rp) != ExecUpdateOK) {
        return false;
    }
    return true;
}

bool DefaultTopologyStorage::UpdatePhysicalPool(const PhysicalPool &data) {
    PhysicalPoolRepo rp(data.GetId(),
         data.GetName(),
         data.GetDesc());
    if (repo_->UpdatePhysicalPoolRepo(rp) != ExecUpdateOK) {
        return false;
    }
    return true;
}

bool DefaultTopologyStorage::UpdateZone(const Zone &data) {
    ZoneRepo rp(data.GetId(),
        data.GetName(),
        data.GetPhysicalPoolId(),
        data.GetDesc());
    if (repo_->UpdateZoneRepo(rp) != ExecUpdateOK) {
        return false;
    }
    return true;
}

bool DefaultTopologyStorage::UpdateServer(const Server &data) {
    ServerRepo rp(data.GetId(),
        data.GetHostName(),
        data.GetInternalHostIp(),
        data.GetExternalHostIp(),
        data.GetZoneId(),
        data.GetPhysicalPoolId(),
        data.GetDesc());
    if (repo_->UpdateServerRepo(rp) != ExecUpdateOK) {
        return false;
    }
    return true;
}

bool DefaultTopologyStorage::UpdateChunkServer(const ChunkServer &data) {
    ChunkServerState csState = data.GetChunkServerState();
    ChunkServerRepo rp(data.GetId(),
            data.GetToken(),
            data.GetDiskType(),
            data.GetHostIp(),
            data.GetPort(),
            data.GetServerId(),
            data.GetStatus(),
            csState.GetDiskState(),
            csState.GetOnlineState(),
            data.GetMountPoint(),
            csState.GetDiskCapacity(),
            csState.GetDiskUsed());

    if (repo_->UpdateChunkServerRepo(rp) != ExecUpdateOK) {
        return false;
    }
    return true;
}

bool DefaultTopologyStorage::UpdateCopySet(const CopySetInfo &data) {
    Json::Value copysetMemJson;
    for (ChunkServerIdType id : data.GetCopySetMembers()) {
        copysetMemJson.append(id);
    }
    std::string chunkServerListStr = copysetMemJson.toStyledString();

    CopySetRepo rp(data.GetId(),
        data.GetLogicalPoolId(),
        chunkServerListStr);
    if (repo_->UpdateCopySetRepo(rp) != ExecUpdateOK) {
        return false;
    }
    return true;
}


}  // namespace topology
}  // namespace mds
}  // namespace curve
