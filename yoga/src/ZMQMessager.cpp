#include "ZMQMessager.h"
#include "Sleep.h"

namespace YOGA {

ZMQMessager::Client::Client(std::shared_ptr<ZMQServerNameGenerator> server_names) :
  server_names(server_names), context(), socket(context, Socket::Push){}

ZMQMessager::Client::~Client() {
    socket.close();
    context.close();
}

void ZMQMessager::Client::connectToServer(int serverRank, int serverPort) {
    std::string serverAddress =
        "tcp://" + server_names->getServerNameForRank(serverRank) + ":" + std::to_string(serverPort);
    socket.connect(serverAddress);
}

void ZMQMessager::Client::disconnectFromServer(int id, int serverPort) {
    std::string serverAddress = "tcp://" + server_names->getServerNameForRank(id) + ":" + std::to_string(serverPort);
    socket.disconnect(serverAddress);
}

void ZMQMessager::Client::send(const MessagePasser::Message& msg,bool is_more_coming) {
    socket.send(msg,is_more_coming);
}





}
