
#pragma once

namespace YOGA {
enum CellStatus { InCell, OutCell, FringeCell };
enum NodeStatus { InNode, OutNode, FringeNode, Orphan, Unknown, ReceptorCandidate,MandatoryReceptor};


    class StatusKeeper {
    public:
        YOGA::NodeStatus value() const {return current;}
        void transition(NodeStatus s){
            current = s;
        }
        NodeStatus current = Unknown;

        static std::string to_string(NodeStatus s){
            switch (s) {
                case InNode:return "InNode";
                case OutNode:return "OutNode";
                case ReceptorCandidate:return "ReceptorCandidate";
                case MandatoryReceptor:return "MandatoryReceptor";
                case Orphan:return "Orphan";
                case Unknown:return "Unknown";
                case FringeNode:return "FringeNode";
                default: return "Invalid Status";
            }

        }

    };
}