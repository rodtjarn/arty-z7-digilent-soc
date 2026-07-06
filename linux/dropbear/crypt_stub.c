/* Arty Z7 initramfs: the cross toolchain's sysroot has no libcrypt (no
 * crypt.h/libcrypt.a), so this stub satisfies the linker for
 * DROPBEAR_SVR_PASSWORD_AUTH. It's only ever reached for a real (non-blank)
 * password attempt -- returning NULL makes dropbear's existing "invalid
 * salt" handling reject those cleanly. Blank-password login (dropbear -B)
 * never calls crypt() at all, so this doesn't affect that path.
 */
char *crypt(const char *key, const char *salt) {
	(void)key;
	(void)salt;
	return (char *)0;
}
