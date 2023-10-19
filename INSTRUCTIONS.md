Mersive Linux Interview Project
===================

Welcome! Thanks for interviewing with Mersive Technologies!

We're going to work with some HTTP today. Our goal with this exercise isn't to "stump" anyone with tricky business, but to give candidates a fair and uniform way to express themselves in code on a realistic-ish problem.

In a couple of hours, we expect everyone will get some of the way through this problem, but no one will get all the way through. If invited to a technical interview, you'll pair with us wherever you left off, because we'd much rather see how you work on a problem you're familiar with and have had time to think about than on some clever whiteboard puzzle we spring on you.

At this stage, we care most about:
* Interpreting written specifications (following instructions)
* General familiarity with sockets, the C/C++ language, multi-threading, network I/O
* Legible code

When in doubt, do less, better. 😀

# The Problem

Write a minimal HTTP server using TCP sockets.  It should support GET,
POST, and DELETE requests for arbitrary URIs.  Each time a client
POSTs data, a mapping for the specified URI should be created or
updated such that the URI is associated with the body and MIME type of
that POST.  Subsequent GET requests should retrieve that content,
whereas DELETE requests should remove the mapping for that URI so that
subsequent requests return 404.


## Details

The server does NOT need handle HTTPS, nor any request verb besides GET,
POST, or DELETE, nor any headers except Content-Type and
Content-Length.  However, it should be able to skip past headers it
does not recognize.  It need not support chunked encoding.

The server should respond with "404 Not Found" to any request for an
uninitialized or deleted URI.  It should respond with "200 OK" in all
other cases.

Please use nothing higher-level than TCP for networking.  Do not use
an existing web server library or HTTP parser to handle requests or
construct responses.

If you need further clarification about anything, please let us know.

## Documentation

Please include instructions for building and running your solution.
Also, please summarize your design, including any tradeoffs, possible
improvements, or performance concerns which may apply.

## Extra Credit

Please refer the bottom of this document for some extra credit.

## Submission

Please refer to the `README.md` for submission instructions.

# Helpful Reference

## Introduction to HTTP

In case you're not already familiar with the HTTP protocol, here's
what you need to know.

Requests take the following forms:

```
GET /foo HTTP/1.1
User-Agent: curl/7.35.0
Host: example.com
Accept: */*

```

```
POST /foo HTTP/1.1
User-Agent: curl/7.35.0
Host: example.com
Accept: */*
Content-Length: 3
Content-Type: text/plain

foo
```

```
DELETE /foo HTTP/1.1
User-Agent: curl/7.35.0
Host: example.com
Accept: */*

```

The first line contains three tokens: the verb, the URI, and the HTTP
version.  All lines end in an ASCII carriage return followed by an
ASCII line feed.  The first line is followed by zero or more headers,
followed by an empty line.  If there is a request body, it will follow
that empty line for as many bytes as indicated by the Content-Length
header.  If there is no request body (e.g. a GET or DELETE), the empty
line ends the request.

Responses take the following form:

```
HTTP/1.1 200 OK
Content-Type: text/plain
Content-Length: 4

haha
```

The first line contains the protocol version, response code, and a
string describing the response code.  Again, each line is followed by
a carriage return and line feed, and there is an empty line between
the headers and the body, if present.


## Examples

```{bash}
# By default, the server should return "404 Not Found" for all URIs
$ curl -i http://localhost:8000/foo
HTTP/1.1 404 Not Found
...

# The server should accept data of any type (e.g. JSON, PNG, text, etc.)
$ curl -i -H "Content-Type: application/json" -X POST -d '{ "secret": 42 }' http://localhost:8000/foo
HTTP/1.1 200 OK
...

$ curl -i http://localhost:8000/foo
HTTP/1.1 200 OK
Content-Type: application/json
...
{ "secret": 42 }

$ curl -i -H "Content-Type: text/plain" -X POST -d 'squeamish ossifrage' http://localhost:8000/yo/yo/yo
HTTP/1.1 200 OK
...

$ curl -i http://localhost:8000/yo/yo/yo
HTTP/1.1 200 OK
Content-Type: text/plain
...
squeamish ossifrage

# Deleting a URI removes the mapping
$ curl -i -X DELETE http://localhost:8000/yo/yo/yo
HTTP/1.1 200 OK
...

$ curl -i http://localhost:8000/yo/yo/yo
HTTP/1.1 404 Not Found
...
```

# Extra Credit

This part is optional, so feel free to skip it.

Modify your server to support non-destructive updates.  Each POST or
DELETE should cause a new version of the data store to be created and
assigned an incrementing version number.  The number for the
newly-created version should be returned in a custom response header
named X-Data-Version.  GET requests should accept an optional version
parameter indicating which version to query.  If that parameter is not
specified, the latest version is queried.

For example:

```{bash}
$ curl -i -X POST -d 'hello' http://localhost:8000/foo
HTTP/1.1 200 OK
X-Data-Version: 1
...

$ curl -i http://localhost:8000/foo
HTTP/1.1 200 OK
...
hello

$ curl -i -X POST -d 'hola' http://localhost:8000/foo
HTTP/1.1 200 OK
X-Data-Version: 2
...

$ curl -i http://localhost:8000/foo
HTTP/1.1 200 OK
...
hola

$ curl -i http://localhost:8000/foo?version=1
HTTP/1.1 200 OK
...
hello

$ curl -i http://localhost:8000/foo?version=2
HTTP/1.1 200 OK
...
hola

$ curl -i -X DELETE http://localhost:8000/foo
HTTP/1.1 200 OK
X-Data-Version: 3
...

$ curl -i http://localhost:8000/foo
HTTP/1.1 404 Not Found
...

$ curl -i http://localhost:8000/foo?version=2
HTTP/1.1 200 OK
...
hola
```
