#pragma once
#include <MessagePasser/MessagePasser.h>
#include <mpi.h>

namespace inf {
void initializeMPI();
MPI_Comm getCommunicator();
void finalizeMPI();
}
