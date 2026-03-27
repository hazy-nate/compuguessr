
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

CompuGuessr is a FastCGI web application that gamifies challenges involving
data.

## Building

The build process can be done within a Docker container. The container can be
built from the root of the project using the `make docker-build` command.

### Docker

```bash
sudo emerge -av app-containers/docker \
  app-containers/docker-cli \
  app-containers/docker-compose
```

```bash
git clone git@natewilliams.xyz:compuguessr.git
cd compuguessr
make docker-build-dev
make docker-make
```

### Manual

The build process for the CompuGuessr executable depends on these programs:

**Required:**

- **C preprocessor/compiler:** `clang` or `gcc`
- **Assembler:** `nasm`
- **Linker:** `ld` or `mold`
- **Builder:** `make`

```bash
git clone git@natewilliams.xyz:compuguessr.git
cd compuguessr
make
```

## Running

Since CompuGuessr is a FastCGI application, it's necessary to use a web server
that supports FastCGI. The repository provides an example of a working
configuration with **lighttpd**:

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
