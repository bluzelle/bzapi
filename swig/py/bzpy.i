%module bzpy

%include <std_shared_ptr.i>
%{
#include "library/response.hpp"
#include "library/library.hpp"
#include "database/database.hpp"
using namespace bzapi;
%}
%shared_ptr(bzapi::response)
%include "library/response.hpp"
%include "library/library.hpp"
%include "database/database.hpp"
%typemap(out) std::string {
        $result = PyString_FromString($1.c_str());
}
