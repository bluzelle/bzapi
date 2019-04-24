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
%shared_ptr(response)
%shared_ptr(test)
%typemap(out) std::string {
        $result = PyString_FromString($1.c_str());
}
