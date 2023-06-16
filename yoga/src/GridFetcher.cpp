#include "GridFetcher.h"

namespace YOGA {
GridFetcher::GridFetcher(const std::vector<VoxelFragment>& fragments)
    : fragment_msgs(buildFragmentMessages(fragments)),
      fragment_extents(buildFragmentExtents(fragments))
      {}

MessagePasser::Message GridFetcher::doWork(MessagePasser::Message& msg) {
    Tracer::begin("Process request");
    Parfait::Extent<double> e;
    msg.unpack(e);
    MessagePasser::Message result;
    size_t mb = 1024*1024;
    result.reserve(5*mb);
    Tracer::begin("get overlapping fragment ids");
    auto fragment_ids = getOverlappingFragmentIds(e);
    Tracer::end("get overlapping fragment ids");
    result.pack(int(fragment_ids.size()));
    Tracer::begin("pack");
    Tracer::begin(std::to_string(fragment_ids.size()));
    for(int id:fragment_ids) {
        auto& m = fragment_msgs[id];
        result.pack(m->data(),m->size());
    }
    Tracer::end(std::to_string(fragment_ids.size()));
    Tracer::end("pack");
    Tracer::end("Process request");
    return result;
}

std::vector<std::shared_ptr<MessagePasser::Message>> GridFetcher::buildFragmentMessages(const std::vector<VoxelFragment>& frags){
    std::vector<std::shared_ptr<MessagePasser::Message>> msgs;
    for(auto& frag:frags){
        msgs.push_back(std::make_shared<MessagePasser::Message>());
        auto& msg = msgs.back();
        msg->pack(frag.transferNodes);
        msg->pack(frag.transferTets);
        msg->pack(frag.transferPyramids);
        msg->pack(frag.transferPrisms);
        msg->pack(frag.transferHexs);
    }
    return msgs;
}

std::vector<Parfait::Extent<double>> GridFetcher::buildFragmentExtents(const std::vector<VoxelFragment>& frags){
    std::vector<Parfait::Extent<double>> extents;
    for(auto& frag:frags){
        auto e = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
        for(auto& tn:frag.transferNodes){
            Parfait::ExtentBuilder::add(e,tn.xyz);
        }
        extents.emplace_back(e);
    }
    return extents;
}

std::vector<int> GridFetcher::getOverlappingFragmentIds(const Parfait::Extent<double>& e) {
    std::vector<int> ids;
    for(int i=0;i<long(fragment_extents.size());i++)
        if(e.intersects(fragment_extents[i]))
            ids.push_back(i);
    return ids;
}
}