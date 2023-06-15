
program main
    use tracing_wrapper, only : initialize_with_rank, finalize, begin_event, end_event, get_memory, trace_memory
    use tracing_wrapper, only : initialize_with_ranks_to_trace, is_initialized
    implicit none
    integer :: mb
    integer :: ranks_to_trace(2)
    ranks_to_trace(1) = 0
    ranks_to_trace(2) = 1
!    call initialize_with_rank(0)
    call initialize_with_ranks_to_trace(0, ranks_to_trace, 2)
    if(is_initialized()) then
        write(*,*) "Tracer is initialized correctly"
    end if
    call begin_event("Outer loop")
    call begin_event("Inner loop")
    mb = get_memory()
    call trace_memory()
    write(*,*) 'Using',mb,'mb of memory according to tracer'
    call end_event("Inner loop")
    call end_event("Outer loop")
    call finalize()

end program main

