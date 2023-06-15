interface
    subroutine tracer_initialize(id) bind(c)
        use iso_c_binding
        integer(c_int), intent(in), value :: id
    end subroutine
end interface

interface
    subroutine tracer_initialize_with_ranks_to_trace(id, ranks_to_trace, num_ranks_to_trace) bind(c)
        use iso_c_binding
        integer(c_int), intent(in), value :: id, num_ranks_to_trace
        integer(c_int), dimension(num_ranks_to_trace), intent(inout) :: ranks_to_trace
    end subroutine
end interface

interface
    function tracer_is_initialized() bind(c)
        use iso_c_binding
        integer(c_int) :: tracer_is_initialized
    end function
end interface

interface
    subroutine tracer_begin(x) bind(c)
        use iso_c_binding
        character(c_char), intent(inout) :: x(*)
    end subroutine
end interface

interface
    subroutine tracer_end(x) bind(c)
        use iso_c_binding
        character(c_char), intent(inout) :: x(*)
    end subroutine
end interface

interface
    function tracer_get_memory_in_mb() bind(c)
        use iso_c_binding
        integer(c_int) :: tracer_get_memory_in_mb
    end function
end interface

interface
    subroutine tracer_trace_memory() bind(c)
        use iso_c_binding
    end subroutine
end interface

interface
    subroutine tracer_flush() bind(c)
        use iso_c_binding
    end subroutine
end interface

interface
    subroutine tracer_finalize() bind(c)
        use iso_c_binding
    end subroutine
end interface
