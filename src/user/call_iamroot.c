/*
 * call_iamroot.c — programa de usuário que invoca a syscall iamroot (nº 692).
 * Compilar estático (o initramfs não tem libc dinâmica):
 *     gcc -static -O2 -o call_iamroot call_iamroot.c
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define SYS_iamroot 692   /* 600 + (matricula % 100); matricula 2311292 -> 692 */

int main(void)
{
	printf("uid antes  : %d\n", getuid());

	long r = syscall(SYS_iamroot, 0);
	printf("retorno    : %ld\n", r);

	printf("euid depois: %d\n", geteuid());
	if (geteuid() == 0)
		printf(">> Agora sou root! Veja o codigo no dmesg.\n");

	return 0;
}
