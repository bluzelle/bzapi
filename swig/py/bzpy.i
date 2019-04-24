%module bzpy

%include <std_shared_ptr.i>
%{
#include "library/response.hpp"
#include "library/library.hpp"
#include "database/database.hpp"
%}
%include "library/response.hpp"
%include "library/library.hpp"
%include "database/database.hpp"
%shared_ptr(bzapi::response)
%typemap(out) std::string {
        $result = PyString_FromString($1.c_str());
}
