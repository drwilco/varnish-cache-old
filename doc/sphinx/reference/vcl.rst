.. _reference-vcl:

===
VCL
===

------------------------------
Varnish Configuration Language
------------------------------

:Author: Dag-Erling Smørgrav
:Author: Poul-Henning Kamp
:Author: Kristian Lyngstøl
:Author: Per Buer
:Date:   2010-06-02
:Version: 1.0
:Manual section: 7

DESCRIPTION
===========

The VCL language is a small domain-specific language designed to be
used to define request handling and document caching policies for
Varnish Cache.

When a new configuration is loaded, the varnishd management process
translates the VCL code to C and compiles it to a shared object which
is then dynamically linked into the server process.

SYNTAX
======

The VCL syntax is very simple, and deliberately similar to C and Perl.
Blocks are delimited by curly braces, statements end with semicolons,
and comments may be written as in C, C++ or Perl according to your own
preferences.

In addition to the C-like assignment (=), comparison (==, !=) and
boolean (!, && and \|\|) operators, VCL supports both regular
expression and ACL matching using the ~ and the !~ operators.

Unlike C and Perl, the backslash (\) character has no special meaning
in strings in VCL, so it can be freely used in regular expressions
without doubling.

Strings are concatenated using the '+' operator. 

Assignments are introduced with the *set* keyword.  There are no
user-defined variables; values can only be assigned to variables
attached to backend, request or document objects.  Most of these are
typed, and the values assigned to them must have a compatible unit
suffix.

You can use the *set* keyword to arbitrary HTTP headers. You can
remove headers with the *remove* or *unset* keywords, which are
synonym.

The ``return(action)`` keyword terminates the subroutine. *action* can be,
depending on context one of

* deliver
* error
* fetch
* hash
* lookup
* pass
* pipe
* restart

Please see the list of subroutines to see what return actions are
available where.

VCL has if tests, but no loops.

You may log arbitrary strings to the shared memory log with the
keyword *log*.

The contents of another VCL file may be inserted at any point in the
code by using the *include* keyword followed by the name of the other
file as a quoted string.

Backend declarations
--------------------

A backend declaration creates and initializes a named backend object:::

  backend www {
    .host = "www.example.com";
    .port = "http";
  }

The backend object can later be used to select a backend at request time:::

  if (req.http.host ~ "(?i)^(www.)?example.com$") {
    set req.backend = www;
  }

To avoid overloading backend servers, .max_connections can be set to
limit the maximum number of concurrent backend connections.

The timeout parameters can be overridden in the backend declaration.
The timeout parameters are .connect_timeout for the time to wait for a
backend connection, .first_byte_timeout for the time to wait for the
first byte from the backend and .between_bytes_timeout for time to
wait between each received byte.

These can be set in the declaration like this:::

  backend www {
    .host = "www.example.com";
    .port = "http";
    .connect_timeout = 1s;
    .first_byte_timeout = 5s;
    .between_bytes_timeout = 2s;
  }

To mark a backend as unhealthy after number of items have been added
to it's saintmode list .saintmode_threshold can be set to the maximum
list size. Setting a value of 0 disables saintmode checking entirely
for that backend.  The value in the backend declaration overrides the
parameter.

Directors
---------

A director is a logical group of backend servers clustered together
for redundancy. The basic role of the director is to let Varnish
choose a backend server amongst several so if one is down another can
be used.

There are several types of directors. The different director types
use different algorithms to choose which backend to use.

Configuring a director may look like this:::

  director b2 random {
    .retries = 5;
    {
      // We can refer to named backends
      .backend = b1;
      .weight  = 7;
    }
    {
      // Or define them inline 
      .backend  = {
        .host = "fs2";
      }
    .weight         = 3;
    }
  } 

Directors can also use other directors as a backend:::

  director cluster1 hash {
    { .backend = { .host = "cl1n1"; } .weight = 1; }
    { .backend = { .host = "cl1n2"; } .weight = 1; }
  }
  director cluster2 hash {
    { .backend = { .host = "cl2n1"; } .weight = 1; }
    { .backend = { .host = "cl2n2"; } .weight = 1; }
  }
  director default random {
    { .backend = cluster1; .weight = 1; }
    { .backend = cluster2; .weight = 1; }
  }

The random director
~~~~~~~~~~~~~~~~~~~

The random director takes one per director option .retries.  This
specifies how many tries it will use to find a working backend.  The
default is the same as the number of backends defined for the
director.

There is also a per-backend option: weight which defines the portion
of traffic to send to the particular backend.

The round-robin director
~~~~~~~~~~~~~~~~~~~~~~~~

The round-robin director does not take any options.


The client director
~~~~~~~~~~~~~~~~~~~

The client director picks a backend based on the clients
*identity*. You can set the VCL variable *client.identity* to identify
the client by picking up the value of a session cookie or similar.

Note: from 2.1.0 to 2.1.3 *client.identity* isn't available and the
director will use automatically set the idenity based on client.ip In
2.1.4 and onwards you can set client.identity to any string available.

The client director takes one option - *retries* which set the number
of retries the director should take in order to find a healthy
backend.



The hash director
~~~~~~~~~~~~~~~~~

The hash director will pick a backend based on the URL hash
value. 

This is useful is you are using Varnish to load balance in front of
other Varnish caches or other web accelerators as objects won't be
duplicated across caches.

The client director takes one option - *retries* which set the number
of retries the director should take in order to find a healthy
backend.

The DNS director
~~~~~~~~~~~~~~~~

The DNS director can use backends in three different ways. Either like the
random or round-robin director or using .list::

  director directorname dns {
          .list = {
                  .host_header = "www.example.com";
                  .port = "80";
                  .connect_timeout = 0.4;
                  "192.168.15.0"/24;
                  "192.168.16.128"/25;
          }
          .ttl = 5m;
          .suffix = "internal.example.net";
  }

This will specify 384 backends, all using port 80 and a connection timeout
of 0.4s. Options must come before the list of IPs in the .list statement.

The .ttl defines the cache duration of the DNS lookups.

The above example will append "internal.example.net" to the incoming Host
header supplied by the client, before looking it up. All settings are
optional.

The fallback director
~~~~~~~~~~~~~~~~~~~~~

The fallback director will pick the first backend that is healthy. It 
considers them in the order in which they are listed in its definition.

The fallback director does not take any options.

An example of a fallback director::

  director b3 fallback {
    { .backend = www1; }
    { .backend = www2; } // will only be used if www1 is unhealthy.
    { .backend = www3; } // will only be used if both www1 and www2
                         // are unhealthy.
  }

Backend probes
--------------

Backends can be probed to see whether they should be considered
healthy or not.  The return status can also be checked by using
req.backend.healthy .window is how many of the latest polls we
examine, while .threshold is how many of those must have succeeded for
us to consider the backend healthy.  .initial is how many of the
probes are considered good when Varnish starts - defaults to the same
amount as the threshold.

A backend with a probe can be defined like this:::

  backend www {
    .host = "www.example.com";
    .port = "http";
    .probe = {
      .url = "/test.jpg";
      .timeout = 0.3 s;
      .window = 8;
      .threshold = 3;
      .initial = 3;
    }
  }

It is also possible to specify the raw HTTP request::

  backend www {
    .host = "www.example.com";
    .port = "http";
    .probe = {
      # NB: \r\n automatically inserted after each string!
      .request =
        "GET / HTTP/1.1"
        "Host: www.foo.bar"
        "Connection: close";
    }
  }

ACLs
----

An ACL declaration creates and initializes a named access control list
which can later be used to match client addresses:::

  acl local {
    "localhost";         // myself
    "192.0.2.0"/24;      // and everyone on the local network
    ! "192.0.2.23";      // except for the dialin router 
  }

If an ACL entry specifies a host name which Varnish is unable to
resolve, it will match any address it is com‐ pared to.  Consequently,
if it is preceded by a negation mark, it will reject any address it is
compared to, which may not be what you intended.  If the entry is
enclosed in parentheses, however, it will simply be ignored.

To match an IP address against an ACL, simply use the match operator:::

  if (client.ip ~ local) {
    return (pipe);
  }

Regular Expressions
-------------------

In Varnish 2.1.0 Varnish switched to using PCRE - Perl-compatible
regular expressions. For a complete description of PCRE please see the
PCRE(3) man page.

To send flags to the PCRE engine, such as to turn on *case
insensitivity* add the flag within parens following a question mark,
like this:::

  if (req.http.host ~ "(?i)example.com$") {
          ...
  }


Functions
---------

The following built-in functions are available:

hash_data(str)
  Adds a string to the hash input. In default.vcl hash_data() is
  called on the host and URL of the *request*.

regsub(str, regex, sub)
  Returns a copy of str with the first occurrence of the regular 
  expression regex replaced with sub. Within sub, \0 (which can 
  also be spelled &) is replaced with the entire matched string, 
  and \n is replaced with the contents of subgroup n in the 
  matched string.

regsuball(str, regex, sub)
  As regsuball() but this replaces all occurrences.

ban_url(regex)
  Ban all objects in cache whose URLs match regex.

Subroutines
~~~~~~~~~~~

A subroutine is used to group code for legibility or reusability:::
  
  sub pipe_if_local {
    if (client.ip ~ local) {
      return (pipe);
    }
  }

Subroutines in VCL do not take arguments, nor do they return values.

To call a subroutine, use the call keyword followed by the subroutine's name:

call pipe_if_local;

There are a number of special subroutines which hook into the Varnish
workflow.  These subroutines may inspect and manipulate HTTP headers
and various other aspects of each request, and to a certain extent
decide how the request should be handled.  Each subroutine terminates
by calling one of a small number of keywords which indicates the
desired outcome.

vcl_recv
  Called at the beginning of a request, after the complete request has
  been received and parsed.  Its purpose is to decide whether or not
  to serve the request, how to do it, and, if applicable, which backend
  to use.
  
  The vcl_recv subroutine may terminate with calling return() on one of
  the following keywords:

  error code [reason]
    Return the specified error code to the client and abandon the request.

  pass    
    Switch to pass mode.  Control will eventually pass to vcl_pass.

  pipe    
    Switch to pipe mode.  Control will eventually pass to vcl_pipe.

  lookup  
    Look up the requested object in the cache.  Control will
    eventually pass to vcl_hit or vcl_miss, depending on whether the
    object is in the cache.

vcl_pipe
  Called upon entering pipe mode.  In this mode, the request is passed
  on to the backend, and any further data from either client or
  backend is passed on unaltered until either end closes the
  connection.
  
  The vcl_pipe subroutine may terminate with calling return() with one of
  the following keywords:

  error code [reason]
    Return the specified error code to the client and abandon the request.

  pipe
    Proceed with pipe mode.

vcl_pass
  Called upon entering pass mode.  In this mode, the request is passed
  on to the backend, and the backend's response is passed on to the
  client, but is not entered into the cache.  Subsequent requests sub‐
  mitted over the same client connection are handled normally.
  
  The vcl_recv subroutine may terminate with calling return() with one of
  the following keywords:
  
  error code [reason]
    Return the specified error code to the client and abandon the request.

  pass
    Proceed with pass mode.

  restart
    Restart the transaction. Increases the restart counter. If the number 
    of restarts is higher than *max_restarts* varnish emits a guru meditation 
    error.

vcl_hash
  You may call hash_data() on the data you would like to add to the hash.
  
  The vcl_hash subroutine may terminate with calling return() with one of
  the following keywords:

  hash
    Proceed.

vcl_hit
  Called after a cache lookup if the requested document was found in the cache.
  
  The vcl_hit subroutine may terminate with calling return() with one of
  the following keywords:

  deliver
    Deliver the cached object to the client.  Control will eventually
    pass to vcl_deliver.

  error code [reason]
    Return the specified error code to the client and abandon the request.

  pass
    Switch to pass mode.  Control will eventually pass to vcl_pass.

  restart
    Restart the transaction. Increases the restart counter. If the number 
    of restarts is higher than *max_restarts* varnish emits a guru meditation 
    error.

vcl_miss
  Called after a cache lookup if the requested document was not found
  in the cache.  Its purpose is to decide whether or not to attempt to
  retrieve the document from the backend, and which backend to use.
  
  The vcl_miss subroutine may terminate with calling return() with one of
  the following keywords:

  error code [reason]
    Return the specified error code to the client and abandon the request.

  pass
    Switch to pass mode.  Control will eventually pass to vcl_pass.

  fetch
    Retrieve the requested object from the backend.  Control will
    eventually pass to vcl_fetch.

vcl_fetch
  Called after a document has been successfully retrieved from the backend.
  
  The vcl_fetch subroutine may terminate with calling return() with
  one of the following keywords:

  deliver
    Possibly insert the object into the cache, then deliver it to the
    client.  Control will eventually pass to vcl_deliver.

  error code [reason]
    Return the specified error code to the client and abandon the request.

  pass
    Switch to pass mode.  Control will eventually pass to vcl_pass.

  restart
    Restart the transaction. Increases the restart counter. If the number 
    of restarts is higher than *max_restarts* varnish emits a guru meditation 
    error.

vcl_deliver
  Called before a cached object is delivered to the client.
  
  The vcl_deliver subroutine may terminate with one of the following
  keywords:

  deliver
    Deliver the object to the client.

  error code [reason]
    Return the specified error code to the client and abandon the request.

  restart
    Restart the transaction. Increases the restart counter. If the number 
    of restarts is higher than *max_restarts* varnish emits a guru meditation 
    error.

vcl_error
  Called when we hit an error, either explicitly or implicitly due to 
  backend or internal errors.

  The vcl_error subroutine may terminate by calling return with one of
  the following keywords:
 
  deliver
    Deliver the error object to the client.

  restart
    Restart the transaction. Increases the restart counter. If the number 
    of restarts is higher than *max_restarts* varnish emits a guru meditation 
    error.

If one of these subroutines is left undefined or terminates without
reaching a handling decision, control will be handed over to the
builtin default.  See the EXAMPLES section for a listing of the
default code.

Multiple subroutines
~~~~~~~~~~~~~~~~~~~~
If multiple subroutines with the same name are defined, they are
concatenated in the order in which the appear in the source.

Example:::

	# in file "main.vcl"
	include "backends.vcl";
	include "ban.vcl";

	# in file "backends.vcl"
	sub vcl_recv {
	  if (req.http.host ~ "(?i)example.com") {
	    set req.backend = foo;
	  } elsif (req.http.host ~ "(?i)example.org") {
	    set req.backend = bar;
	  }
	}

	# in file "ban.vcl"
	sub vcl_recv {
	  if (client.ip ~ admin_network) {
	    if (req.http.Cache-Control ~ "no-cache") {
	      ban_url(req.url);
	    }
	  }
	}

The builtin default subroutines are implicitly appended in this way.

Variables
~~~~~~~~~

Although subroutines take no arguments, the necessary information is
made available to the handler subroutines through global variables.

The following variables are always available:

now   
  The current time, in seconds since the epoch.

The following variables are available in backend declarations:

.host
  Host name or IP address of a backend.

.port
  Service name or port number of a backend.

The following variables are available while processing a request:

client.ip
  The client's IP address.

server.hostname
  The host name of the server.

server.identity 
  The identity of the server, as set by the -i
  parameter.  If the -i parameter is not passed to varnishd,
  server.identity will be set to the name of the instance, as
  specified by the -n parameter.

server.ip
  The IP address of the socket on which the client connection was received.

server.port
  The port number of the socket on which the client connection was received.

req.request
  The request type (e.g. "GET", "HEAD").

req.url
  The requested URL.

req.proto
  The HTTP protocol version used by the client.

req.backend
  The backend to use to service the request.

req.backend.healthy
  Whether the backend is healthy or not. Requires an active probe to be set
  on the backend.

req.http.header
  The corresponding HTTP header.

req.hash_always_miss
  Force a cache miss for this request. If set to true Varnish will disregard
  any existing objects and always (re)fetch from the backend.

req.hash_ignore_busy
  Ignore any busy object during cache lookup. You would want to do 
  this if you have two server looking up content from each other to 
  avoid potential deadlocks.

req.can_gzip
  Does the client accept the gzip transfer encoding.

The following variables are available while preparing a backend
request (either for a cache miss or for pass or pipe mode):

bereq.request
  The request type (e.g. "GET", "HEAD").

bereq.url
  The requested URL.

bereq.proto
  The HTTP protocol version used to talk to the server.

bereq.http.header
  The corresponding HTTP header.

bereq.connect_timeout
  The time in seconds to wait for a backend connection.

bereq.first_byte_timeout
  The time in seconds to wait for the first byte from the backend.  Not
  available in pipe mode.

bereq.between_bytes_timeout
  The time in seconds to wait between each received byte from the
  backend.  Not available in pipe mode.

The following variables are available after the requested object has
been retrieved from the backend, before it is entered into the cache. In
other words, they are available in vcl_fetch:

beresp.do_esi
  Boolean. ESI-process the object after fetching it. Defaults to false. Set it
  to true to parse the object for ESI directives.

beresp.do_gzip
  Boolean. Gzip the object before storing it. Defaults to true. 

beresp.do_gunzip
  Boolean. Unzip the object before storing it in the cache.  Defaults
  to false.

beresp.proto
  The HTTP protocol version used the backend replied with.

beresp.status
  The HTTP status code returned by the server.

beresp.response
  The HTTP status message returned by the server.

beresp.cacheable
  True if the request resulted in a cacheable response. A response is
  considered cacheable if HTTP status code is 200, 203, 300, 301, 302,
  404 or 410 and pass wasn't called in vcl_recv. If however, both the
  TTL and the grace time for the response are 0 beresp.cacheable will
  be 0.
  
  beresp.cacheable is writable.

beresp.ttl
  The object's remaining time to live, in seconds. beresp.ttl is writable.

After the object is entered into the cache, the following (mostly
read-only) variables are available when the object has been located in
cache, typically in vcl_hit and vcl_deliver.

obj.proto
  The HTTP protocol version used when the object was retrieved.

obj.status
  The HTTP status code returned by the server.

obj.response
  The HTTP status message returned by the server.

obj.cacheable
  True if the object had beresp.cacheable. Unless you've forced delivery
  in your VCL obj.cacheable will always be true.

obj.ttl
  The object's remaining time to live, in seconds. obj.ttl is writable.

obj.lastuse
  The approximate time elapsed since the object was last requests, in
  seconds.

obj.hits
  The approximate number of times the object has been delivered. A value 
  of 0 indicates a cache miss.

The following variables are available while determining the hash key
of an object:

req.hash
  The hash key used to refer to an object in the cache.  Used when
  both reading from and writing to the cache.

The following variables are available while preparing a response to the client:

resp.proto
  The HTTP protocol version to use for the response.

resp.status
  The HTTP status code that will be returned.

resp.response
  The HTTP status message that will be returned.

resp.http.header
  The corresponding HTTP header.

Values may be assigned to variables using the set keyword:::

  sub vcl_recv {
    # Normalize the Host: header
    if (req.http.host ~ "(?i)^(www.)?example.com$") {
      set req.http.host = "www.example.com";
    }
  }

HTTP headers can be removed entirely using the remove keyword:::

  sub vcl_fetch {
    # Don't cache cookies
    remove beresp.http.Set-Cookie;
  }

Grace and saint mode
--------------------

If the backend takes a long time to generate an object there is a risk
of a thread pile up.  In order to prevent this you can enable *grace*.
This allows varnish to serve an expired version of the object while a
fresh object is being generated by the backend.

The following vcl code will make Varnish serve expired objects.  All
object will be kept up to two minutes past their expiration time or a
fresh object is generated.::

  sub vcl_recv {
    set req.grace = 2m;
  }
  sub vcl_fetch {
    set beresp.grace = 2m;
  }

Saint mode is similar to grace mode and relies on the same
infrastructure but functions differently. You can add VCL code to
vcl_fetch to see whether or not you *like* the response coming from
the backend. If you find that the response is not appropriate you can
set beresp.saintmode to a time limit and call *restart*. Varnish will
then retry other backends to try to fetch the object again. 

If there are no more backends or if you hit *max_restarts* and we have
an object that is younger than what you set beresp.saintmode to be
Varnish will serve the object, even if it is stale.

EXAMPLES
========

The following code is the equivalent of the default configuration with
the backend address set to "backend.example.com" and no backend port
specified:::

  backend default {
   .host = "backend.example.com";
   .port = "http";
  }

.. include:: ../../../bin/varnishd/default.vcl
  :literal:

The following example shows how to support multiple sites running on
separate backends in the same Varnish instance, by selecting backends
based on the request URL:::

  backend www {
    .host = "www.example.com";
    .port = "80";
  }
  
  backend images {
    .host = "images.example.com";
    .port = "80";
  }
  
  sub vcl_recv {
    if (req.http.host ~ "(?i)^(www.)?example.com$") {
      set req.http.host = "www.example.com";
      set req.backend = www;
    } elsif (req.http.host ~ "(?i)^images.example.com$") {
      set req.backend = images;
    } else {
      error 404 "Unknown virtual host";
    }
  }

  The following snippet demonstrates how to force a minimum TTL for
  all documents.  Note that this is not the same as setting the
  default_ttl run-time parameter, as that only affects document for
  which the backend did not specify a TTL:::
  
  sub vcl_fetch {
    if (beresp.ttl < 120s) {
      log "Adjusting TTL";
      set beresp.ttl = 120s;
    }
  }

The following snippet demonstrates how to force Varnish to cache
documents even when cookies are present:::

  sub vcl_recv {
    if (req.request == "GET" && req.http.cookie) {
       return(lookup);
    }
  }
  
  sub vcl_fetch {
    if (beresp.http.Set-Cookie) {
       return(deliver);
   }
  }

The following code implements the HTTP PURGE method as used by Squid
for object invalidation:::

  acl purge {
    "localhost";
    "192.0.2.1"/24;
  }

  sub vcl_recv {
    if (req.request == "PURGE") {
      if (!client.ip ~ purge) {
        error 405 "Not allowed.";
      }
      return(lookup);
    }
  }

  sub vcl_hit {
    if (req.request == "PURGE") {
      purge;
      error 200 "Purged.";
    }
  }

  sub vcl_miss {
    if (req.request == "PURGE") {
      purge;
      error 200 "Purged.";
    }
  }

SEE ALSO
========

* varnishd(1)

HISTORY
=======

The VCL language was developed by Poul-Henning Kamp in cooperation
with Verdens Gang AS, Linpro AS and Varnish Software.  This manual
page was written by Dag-Erling Smørgrav and later edited by
Poul-Henning Kamp and Per Buer.

COPYRIGHT
=========

This document is licensed under the same licence as Varnish
itself. See LICENCE for details.

* Copyright (c) 2006 Verdens Gang AS
* Copyright (c) 2006-2008 Linpro AS
* Copyright (c) 2008-2010 Redpill Linpro AS
* Copyright (c) 2010 Varnish Software AS
