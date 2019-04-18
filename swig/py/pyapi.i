%module libdb

%include <std_shared_ptr.i>
%{
#include "boost/asio.hpp"
#include "library/library.hpp"
#include "library/response.hpp"
#include "library/database.hpp"

%}
%shared_ptr(response)
%shared_ptr(test)
%typemap(out) std::string {
        $result = PyString_FromString($1.c_str());
}
#include "library/library.hpp"
#include "library/response.hpp"
#include "library/database.hpp"
