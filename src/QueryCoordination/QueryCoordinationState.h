#pragma once

#include <QueryCoordination/Pipelines/Pipelines.h>
#include <Client/ConnectionPool.h>

namespace DB
{

class Fragment;

using FragmentPtr = std::shared_ptr<Fragment>;
using FragmentPtrs = std::vector<FragmentPtr>;

class QueryCoordinationState {

public:
    FragmentPtrs fragments;
    Pipelines pipelines;
    std::unordered_map<String, IConnectionPool::Entry> remote_host_connection;
    StorageLimitsList storage_limits;
};


}
