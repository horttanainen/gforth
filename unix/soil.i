%module soil
%insert("include")
%{
#include <SOIL.h>
#ifdef __gnu_linux__
#undef stderr
extern struct _IO_FILE *stderr;
#endif
%}

%apply SWIGTYPE * { unsigned char const *const };

%include <SOIL.h>