# Edições no kernel Linux 6.6.13 para a syscall `iamroot`

Matrícula `2311292` · número da syscall **692** (= 600 + 92) · driver em `/proc/IDP-cdriver`.

São três arquivos editados na árvore `build/linux-stable` (tag `v6.6.13`).

## 1. `arch/x86/entry/syscalls/syscall_64.tbl`

Linha adicionada logo após a entrada `453 64 map_shadow_stack`:

```
692	common	iamroot			sys_iamroot
```

## 2. `include/linux/syscalls.h`

Protótipo adicionado junto aos demais de `kernel/sys.c` (após `sys_getpriority`):

```c
asmlinkage long sys_iamroot(int flag);
```

## 3. `kernel/sys.c`

Includes adicionados (após `#include <linux/cred.h>`):

```c
#include <linux/timekeeping.h>
#include <linux/time.h>
```

Variável per-CPU + função (inserida antes de `SYSCALL_DEFINE3(setpriority, ...)`):

```c
/* ===== T03 - Chamada de sistema iamroot ===== */
#define IAMROOT_MATRICULA	2311292
#define IAMROOT_DRIVER_PATH	"/proc/IDP-cdriver"

DEFINE_PER_CPU(bool, syscall_iamroot_flag);
EXPORT_PER_CPU_SYMBOL(syscall_iamroot_flag);

SYSCALL_DEFINE1(iamroot, int, flag)
{
	struct file *driver;
	struct timespec64 ts;
	struct tm tm;
	struct cred *new_cred;
	long ret = 0;

	/* 1. Matricula */
	pr_info("iamroot: matricula = %d\n", IAMROOT_MATRICULA);

	/* 2. Hora do sistema (HH:MM:SS, fuso de Brasilia UTC-3) */
	ktime_get_real_ts64(&ts);
	time64_to_tm(ts.tv_sec, -3 * 3600, &tm);
	pr_info("iamroot: hora = %02d:%02d:%02d\n",
		tm.tm_hour, tm.tm_min, tm.tm_sec);

	/* 3a. Marca a flag per-CPU como verdadeira (lida pelo driver) */
	this_cpu_write(syscall_iamroot_flag, true);

	/* 3b. Interage com o driver-atividade.ko via ioctl */
	driver = filp_open(IAMROOT_DRIVER_PATH, O_RDWR, 0);
	if (IS_ERR(driver)) {
		pr_err("iamroot: falha ao abrir %s (%ld)\n",
		       IAMROOT_DRIVER_PATH, PTR_ERR(driver));
	} else {
		if (driver->f_op && driver->f_op->unlocked_ioctl)
			driver->f_op->unlocked_ioctl(driver, IAMROOT_MATRICULA, 0);
		filp_close(driver, NULL);
	}

	/* 3c. Restaura a flag */
	this_cpu_write(syscall_iamroot_flag, false);

	/* 4. Eleva privilegios para root */
	new_cred = prepare_kernel_cred(NULL); /* NULL => init_cred (root) em 6.6 */
	if (new_cred)
		commit_creds(new_cred);
	else
		ret = -ENOMEM;

	return ret;
}
```

> O driver `driver-atividade.ko` importa o símbolo `syscall_iamroot_flag` (confirmado via `nm -u`); por isso o nome e o `EXPORT_PER_CPU_SYMBOL` precisam ser exatamente esses.
