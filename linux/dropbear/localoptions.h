/* See ../crypt_stub.c -- DROPBEAR_SVR_PASSWORD_AUTH stays at its default
 * (enabled) so the -B (blank password) option compiles in; the crypt()
 * it needs to link against is our stub, not a real libcrypt. */
