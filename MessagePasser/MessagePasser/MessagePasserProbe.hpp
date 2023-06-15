inline MessagePasser::ProbeResult MessagePasser::Probe() const {
    MPI_Status status;
    int flag;
    MPI_Iprobe(MPI_ANY_SOURCE,MPI_ANY_TAG,communicator,&flag,&status);

    bool message_exists = flag;
    int source = status.MPI_SOURCE;
    int n;
    MPI_Get_count(&status,MPI_CHAR,&n);
    ProbeResult probe_result(message_exists,source,n);

    return probe_result;
}