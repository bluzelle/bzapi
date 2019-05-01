%module bzpy

%include <std_shared_ptr.i>
%{
#include "library/response.hpp"
#include "library/library.hpp"
#include "database/database.hpp"
using namespace bzapi;
%}
%typemap(out) std::string {
$result = PyString_FromString($1.c_str());
}
%include std_string.i
using std::string;
%shared_ptr(bzapi::response)
%shared_ptr(bzapi::database)
%include "library/response.hpp"
%include "library/library.hpp"
%include "database/database.hpp"
