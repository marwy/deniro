What?
====

deniro is basically a web server with requests and responses defined in a file.

An example
----------

given a file rules.txt:

    [req]
    method = GET
    name = any GET request
    [res]
    Authorization = basic
    body = this is a response body

    [req]
    inherit = any GET request
    url = /some/cute/url
    User-Agent = Google Chrome 666
    [res]
    status = 400
    body = Authorization required
    Authorization = basic

we can run deniro like this:

```shell-session
    $ ./deniro --rules_file=rules.txt
    Listening on port 8080
    $ curl localhost:8080/
    this is a response body
    $ curl --user-agent="Google Chrome 666" localhost:8080/some/cute/url
    Authorization required
```


Why?
====

I once needed a standalone web server that could be driven by a simple text file and figured
it'd be a nice opportunity to learn C.

This is only a POCs. It doesn't have any dependencies. It includes a terrible parser
for about 50% of HTTP/1.1 and an INI-like config parser; it uses linked lists
for everything and mallocs like crazy.
