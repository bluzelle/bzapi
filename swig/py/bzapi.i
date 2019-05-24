%module bzapi

%include <std_shared_ptr.i>
%{
#include "library/response.hpp"
#include "database/async_database.hpp"
#include "database/database.hpp"
#include "library/library.hpp"

using namespace bzapi;
%}
%typemap(out) std::string {
$result = PyString_FromString($1.c_str());
}
%include std_string.i
%include stdint.i
%shared_ptr(bzapi::response)
%shared_ptr(bzapi::database)
%shared_ptr(bzapi::async_database)

using std::string;
%include "library/response.hpp"
%include "database/async_database.hpp"
%include "database/database.hpp"
%include "library/library.hpp"

