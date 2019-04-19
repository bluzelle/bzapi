%module bzpy

%include <std_shared_ptr.i>
%{
#include "boost/asio.hpp"
#include "library/library.hpp"
#include "library/response.hpp"
#include "database/database.hpp"

%}
%shared_ptr(response)
%typemap(out) std::string {
        $result = PyString_FromString($1.c_str());
}
#include "library/library.hpp"
#include "library/response.hpp"
#include "database/database.hpp"
