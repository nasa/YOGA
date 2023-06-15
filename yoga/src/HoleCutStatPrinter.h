#pragma once
#include <MessagePasser/MessagePasser.h>
#include <RootPrinter.h>
#include <vector>
#include "YogaMesh.h"
#include "YogaStatuses.h"
#include "DruyorTypeAssignment.h"

namespace YOGA {
DruyorTypeAssignment::StatusCounts printStats(const YogaMesh& mesh,
                const std::vector<NodeStatus>& node_statuses,
                const RootPrinter& rootPrinter,
                MessagePasser mp);
}