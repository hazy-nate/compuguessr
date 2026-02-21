
# naws

naws stands for Nate's Assembly Web Server. This is an early prototype of a web
server. For now, it isn't intended for production use.

## Building

The server can be compiled with NASM. The build script uses `mold` by default,
which is a newer drop-in replacement for the GNU linker. However, you can
still use the GNU linker.

```
$ sh build.sh
```

## Running

To run the server, simply run it as any other executable (preferably without
superuser privileges and not exposed publicly).

```
$ ./nasm
```

