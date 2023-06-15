#include "NC_PartitionerBase.h"
#include <parfait/Throw.h>
#include "NC_RCBPartitioner.h"

#if HAVE_PARMETIS
#include "NC_ParmetisPartitioner.h"
#endif

std::unique_ptr<NC::Partitioner> NC::Partitioner::getPartitioner(MessagePasser mp, std::string partitioner_name) {
    if (partitioner_name == "parfait" or partitioner_name == "default") {
        return std::make_unique<NC::RCBPartitioner>();
    }
#if HAVE_PARMETIS
    if (partitioner_name == "parmetis") {
        return std::make_unique<NC::ParmetisPartitioner>();
    }
#else
    mp_rootprint("Could not use parmetis as a partitioner.  Wasn't compiled with parmetis")
#endif
    mp_rootprint("Could not load a NC partitioner named : %s\n", partitioner_name.c_str());
    return std::make_unique<NC::RCBPartitioner>();
}
