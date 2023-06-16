module tracing_wrapper
    implicit none
    private
    public :: begin_event, end_event, initialize_with_rank, initialize_with_ranks_to_trace
    public :: finalize, get_memory, trace_memory, is_initialized
    include "tracer_fortran_interface.inc"
contains

   ! These two subroutines hide the ugliness of
   ! converting fortran strings to c-strings
   ! to make calling tracer nicer in fortran applications
   subroutine begin_event(s)
     use, intrinsic :: iso_c_binding, only : c_null_char
     character(len=*), intent(in) :: s
     character(len=128) :: converted
   continue
     converted = s//c_null_char
     call tracer_begin(converted)
   end subroutine begin_event

   subroutine end_event(s)
     use, intrinsic :: iso_c_binding, only : c_null_char
     character(len=*), intent(in) :: s
     character(len=128) :: converted
   continue
     converted = s//c_null_char
     call tracer_end(converted)
   end subroutine end_event

   subroutine initialize_with_rank(rank)
       integer, intent(in) :: rank
       call tracer_initialize(rank)
   end subroutine initialize_with_rank

  subroutine initialize_with_ranks_to_trace(rank, ranks_to_trace, num_ranks_to_trace)
    integer, intent(in) :: rank, num_ranks_to_trace
    integer, intent(inout), dimension(num_ranks_to_trace) :: ranks_to_trace
    call tracer_initialize_with_ranks_to_trace(rank, ranks_to_trace, num_ranks_to_trace)
  end subroutine initialize_with_ranks_to_trace

  subroutine finalize()
    call tracer_finalize()
  end subroutine finalize

    subroutine trace_memory()
        call tracer_trace_memory()
    end subroutine trace_memory

    function get_memory()
        use, intrinsic :: iso_c_binding, only : c_int
        integer(c_int) :: get_memory
        get_memory = tracer_get_memory_in_mb()
    end function get_memory

  function is_initialized()
    use, intrinsic :: iso_c_binding, only : c_int
    logical :: is_initialized
    integer(c_int) :: temp
    temp = tracer_is_initialized()
    is_initialized = temp == 1
  end function is_initialized

end module
