#pragma once
#include <MessagePasser/MessagePasser.h>
#include <parfait/Throw.h>
#include <t-infinity/GlobalToLocal.h>
#include <t-infinity/MeshConnectivity.h>
#include <t-infinity/TinfMesh.h>

#include <memory>

namespace inf {

void setNodeOwners(MessagePasser mp, std::shared_ptr<inf::TinfMesh> mesh);

}