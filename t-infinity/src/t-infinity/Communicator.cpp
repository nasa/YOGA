#include "Communicator.h"
#include <mpi.h>

void inf::initializeMPI() { MessagePasser::Init(); }

MPI_Comm inf::getCommunicator() { return MPI_COMM_WORLD; }

void inf::finalizeMPI() { MessagePasser::Finalize(); }
