%module bzapi

%include <std_shared_ptr.i>
%{
#include "include/response.hpp"
#include "include/async_database.hpp"
#include "include/database.hpp"
#include "include/bzapi.hpp"

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
%include "include/response.hpp"
%include "include/async_database.hpp"
%include "include/database.hpp"
%include "include/bzapi.hpp"

