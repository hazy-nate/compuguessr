
%macro ENV_KEYS 0
    ENV_KEY C, 'C', "COMPUGUE", "SSR_SOCK", 8, compuguessr_sock, 18, "/var/run/compuguessr/compuguessr.sock"
    ENV_KEY V, 'V', "VALKEY_S", "SOCK=",   7, valkey_sock,      12, "/var/run/valkey/valkey.sock"
%endmacro

