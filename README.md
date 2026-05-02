
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

CompuGuessr is a FastCGI web application that gamifies challenges involving code
and data.

## Building

The easiest way to build the project is within a Docker container. The
development container can be built from the root of the project using the `make
docker-build-native` command.

### Docker

First, ensure that Docker, Docker CLI, and Docker Compose are installed. For
example, those packages can be installed on Gentoo with the following commands:

```bash
sudo emerge -av app-containers/docker \
  app-containers/docker-cli \
  app-containers/docker-compose
```

Then, clone the repository and recursively initialize every submodule. This can
be done like so:

```bash
git clone --recurse-submodules git@natewilliams.xyz:compuguessr.git
cd compuguessr
make docker-build-native
make docker-shell-native
make [debug]|<release>
```

### Manual

The build process for the CompuGuessr executable depends on these programs:

**Required:**

- **C preprocessor/compiler:** `clang` or `gcc`
- **Assembler:** `nasm`
- **Linker:** `ld` or `mold`
- **Builder:** `make`

```bash
git clone --recurse-submodules git@natewilliams.xyz:compuguessr.git
cd compuguessr
make [debug]|<release>
```

## Running

Since CompuGuessr is a FastCGI application, it's necessary to use a web server
that supports FastCGI. The repository provides an example of a working
configuration with **lighttpd**:

```conf
server.document-root = var.CWD + "/public"
server.port = 8080

server.modules += ( "mod_fastcgi", "mod_dirlisting" )

index-file.names = ( "index.html" )

fastcgi.server = (
    "/api/" => ((
        "socket" => "/tmp/compuguessr.sock",
        "check-local" => "disable"
    ))
)

mimetype.assign = (
    ".html" => "text/html",
    ".css"  => "text/css",
    ".js"   => "application/javascript",
    ".json" => "application/json",
    ".png"  => "image/png",
    ".jpg"  => "image/jpeg"
)
```

To start CompuGuessr and lighttpd, simply run the executable and then start
lighttpd with the provided script:

```bash
bin/release/compuguessr &
scripts/lighttpd_start.sh &
```

To stop CompuGuessr and lighttpd, use the `pkill` command:

```bash
pkill compuguessr
pkill lighttpd
```

CompuGuessr creates three files in the `/tmp/` directory:

1. `/tmp/compuguessr.sock` - The socket file connected to by lighttpd.
2. `/tmp/cg_sessions.db` - A file that's mapped in CompuGuessr's memory for
   storing session information.
3. `/tmp/cg_users.db` - A file that's mapped in CompuGuessr's memory for storing
   user information.
