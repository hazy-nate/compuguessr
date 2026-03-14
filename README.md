
# CompuGuessr

<div style="overflow-x: auto; width: 100%;">

<pre style="width: max-content;"><code>
   (                             (
   )\           )             (  )\ )      (     (          (
 (((_)   (     (     `  )    ))\(()/(     ))\   ))\ (   (   )(
 )\___   )\    )\  ' /(/(   /((_)/(_))_  /((_) /((_))\  )\ (()\
((/ __| ((_) _((_)) ((_)_\ (_))((_)) __|(_))( (_)) ((_)((_) ((_)
 | (__ / _ \| '  \()| '_ \)| || | | (_ || || |/ -_)(_-<(_-<| '_|
  \___|\___/|_|_|_| | .__/  \_,_|  \___| \_,_|\___|/__//__/|_|
                    |_|

</code></pre>
</div>

CompuGuessr is a FastCGI web application that gamifies challenges involving data.

## Building

The build process depends on these programs:

**Required:**

- `clang` or `gcc`
- `nasm`
- `ld` or `mold`
- `make`
- `python`
- `robodoc`
- `sphinx`

```bash
git clone https://github.com/hazy-nate/compuguessr.git
cd compuguessr
make
```

## Running

Since CompuGuessr is a FastCGI application, it's necessary to use a web server that supports FastCGI. The repository provides an example of a working configuration with lighttpd:

```conf
# compuguessr.conf

server.modules          += ( "mod_fastcgi" )
server.port             = 8080
server.document-root    = "/var/www/html"

fastcgi.server = (
    "/" => ((
        "bin-path"    => "<ABSOLUTE-PATH-TO-COMPUGUESSR-BIN>",
        "socket"      => "/tmp/compguessr.sock",
        "max-procs"   => 1,
        "check-local" => "disable"
    ))
)
```

```bash
lighttpd -f compuguessr.conf
```
