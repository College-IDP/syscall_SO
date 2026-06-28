# T03 — Syscall `iamroot` · Runbook de Execução

> Documento de execução para implementar o Trabalho Prático 03 de Sistemas Operacionais (IDP, Prof. Jeremias Moreira Gomes). Reúne **parâmetros**, **regras de execução**, **spec com decisões travadas** e **runbook de comandos por fase**.
>
> **Prazo:** 28/06/2026 23:55 · **Entrega:** Moodle · **Modalidade:** individual.

---

## 0. Parâmetros (preencher UMA vez)

Substitua estes valores em todo o documento antes de executar. Eles são a única coisa que muda entre alunos.

```
MATRICULA        = 2311292        # <-- sua matrícula completa
SYSCALL_NR       = 692             # <-- 600 + dois últimos dígitos da matrícula (ex.: ...11 -> 611)
NPROC            = $(nproc)        # processos de compilação
WORKDIR          = $HOME/t03       # raiz de trabalho
DRIVERS_DIR      = $WORKDIR/drivers # onde estão driver-teste.ko e driver-atividade.ko (baixados do Moodle)
```

> ⚠️ `SYSCALL_NR` é **600 + (matrícula mod 100)**. Ex.: matrícula `22221111` → `611`. Confirme antes de prosseguir; um número errado quebra a tabela de syscalls.

---

## 1. Regras de execução

Siga estas regras ao executar este runbook:

1. **Ambiente-alvo:** a máquina de trabalho é **Arch Linux**. Os comandos default são para Arch; blocos marcados `# WSL2/Ubuntu` são o equivalente do lab Dell. Verifique o host (`/etc/os-release`) e use o conjunto certo.
2. **Checkpoints obrigatórios:** PARE e confirme antes de qualquer passo que (a) compile o kernel, (b) inicie o QEMU, (c) modifique credenciais/root. Esses passos são longos ou sensíveis.
3. **Idempotência:** antes de clonar/baixar, cheque se o diretório já existe. Não re-clone o `linux-stable` se já presente (são vários GB).
4. **Shallow clone:** clonar o kernel sempre com `--depth 1 --branch <tag>` para economizar disco/tempo. O `git diff` do patch continua funcionando contra o HEAD da tag.
5. **Sem decisões em runtime:** todas as opções de `menuconfig`, versões e o código C já estão travados na §3. Não improvise — aplique o que está escrito.
6. **Verificação por fase:** ao fim de cada fase, rode o comando de validação indicado e só avance se passar.
7. **Disco:** verifique ≥10 GB livres antes da Fase 2 (`df -h .`). Aborte com aviso se não houver.
8. **Não compile o kernel duas vezes à toa:** a Fase 3 (teste do kernel limpo) é opcional; se o aluno confiar no ambiente, pule direto para a Fase 4 e compile uma única vez já com a syscall.

---

## 2. Pré-requisitos / Ferramentas

### Arch Linux
```bash
sudo pacman -S --needed base-devel bc cpio ncurses openssl libelf pahole \
    qemu-system-x86 qemu-img git
# colordiff é opcional (visualizar patch); está em extra/colordiff
sudo pacman -S --needed colordiff
```
> `flex` e `bison` já vêm no grupo `base-devel`. Se `libelf` falhar, use `elfutils`.

### WSL2 / Ubuntu 24.04 (lab Dell)
```bash
sudo apt-get update
sudo apt-get install -y build-essential libncurses-dev bison flex libssl-dev \
    libelf-dev pahole cpio colordiff \
    qemu-system qemu-utils qemu-system-x86
```

Validação:
```bash
qemu-system-x86_64 --version
gcc --version && make --version
```

---

## 3. SPEC — decisões travadas

### 3.1 Versões fixas
| Componente | Versão | Tag | Commit |
|---|---|---|---|
| BusyBox | 1.36.1 | `1_36_1` | `1a64f6a20aaf6ea4dbba68bbfa8cc1ab7e5c57c4` |
| Kernel  | 6.6.137 | `v6.6.137` | `62b19b564504d027296502e0e30baf5fe0ef080a` |

### 3.2 Config do BusyBox (`make menuconfig`)
- **Networking Utilities** → desabilitar `tc`.
- **Settings → Build Options** → habilitar **Build static binary (no shared libs)**.

### 3.3 Config do Kernel (`make menuconfig`)
- **General setup** → *Initial RAM filesystem and RAM disk (initramfs/initrd) support*: **ON**
- *64-bit kernel*: **ON**
- **Device Drivers → Generic Driver Options**:
  - *Maintain a devtmpfs filesystem to mount at /dev*: **ON**
  - *Automount devtmpfs at /dev, after the kernel mounted the rootfs*: **ON**
- Desabilitar assinatura de módulos (facilita testes):
  ```bash
  scripts/config --disable SYSTEM_TRUSTED_KEYS
  scripts/config --disable SYSTEM_REVOCATION_KEYS
  ```

### 3.4 Edições no kernel (3 arquivos)

**(a) `arch/x86/entry/syscalls/syscall_64.tbl`** — adicionar a linha (respeitando colunas/tabs, e ordenando pelo número):
```
SYSCALL_NR	common	iamroot	sys_iamroot
```

**(b) `include/linux/syscalls.h`** — adicionar o protótipo perto dos demais de `kernel/sys.c`:
```c
asmlinkage long sys_iamroot(int flag);
```

**(c) `kernel/sys.c`** — incluir headers (se ainda não existirem no arquivo) e a função.

Headers (topo do arquivo, após os includes existentes):
```c
#include <linux/cred.h>
#include <linux/fs.h>
#include <linux/timekeeping.h>
#include <linux/time.h>
```

Variável per-CPU + função (adicionar junto às outras `SYSCALL_DEFINE` do arquivo):
```c
DEFINE_PER_CPU(bool, syscall_iamroot_flag);
EXPORT_PER_CPU_SYMBOL(syscall_iamroot_flag);

SYSCALL_DEFINE1(iamroot, int, flag)
{
	struct file *driver;
	struct timespec64 ts;
	struct tm tm;
	struct cred *new_cred;
	long ret = 0;

	/* 1. Matrícula */
	pr_info("iamroot: matricula = %d\n", MATRICULA);

	/* 2. Hora do sistema (HH:MM:SS, fuso de Brasilia UTC-3) */
	ktime_get_real_ts64(&ts);
	time64_to_tm(ts.tv_sec, -3 * 3600, &tm);
	pr_info("iamroot: hora = %02d:%02d:%02d\n",
		tm.tm_hour, tm.tm_min, tm.tm_sec);

	/* 3a. Marca a flag per-CPU como verdadeira */
	this_cpu_write(syscall_iamroot_flag, true);

	/* 3b. Interage com o driver-atividade.ko via ioctl */
	driver = filp_open(DRIVER_PATH, O_RDWR, 0);
	if (IS_ERR(driver)) {
		pr_err("iamroot: falha ao abrir %s (%ld)\n",
		       DRIVER_PATH, PTR_ERR(driver));
	} else {
		if (driver->f_op && driver->f_op->unlocked_ioctl)
			driver->f_op->unlocked_ioctl(driver, MATRICULA, 0);
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

> **Defina `MATRICULA`, `SYSCALL_NR` e `DRIVER_PATH`** substituindo no código acima. Onde o enunciado é ambíguo:
> - `DRIVER_PATH`: o texto cita tanto `/proc/IDP-cdriver` quanto `/proc/proc/IDP-cdriver`. **Confirme o nome real dentro do QEMU com `ls /proc`** após `insmod driver-atividade.ko` e use exatamente o que aparecer (provavelmente `/proc/IDP-cdriver`).

### 3.5 Programa de usuário (`call_iamroot.c`)
Compilar **estático** (o initramfs não tem libc dinâmica) e copiar para o initramfs.
```c
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define SYS_iamroot SYSCALL_NR   /* mesmo numero da §0 */

int main(void)
{
	printf("uid antes : %d\n", getuid());
	long r = syscall(SYS_iamroot, 0);
	printf("retorno   : %ld\n", r);
	printf("euid depois: %d\n", geteuid());
	if (geteuid() == 0)
		printf(">> Agora sou root! Veja o codigo no dmesg.\n");
	return 0;
}
```
```bash
gcc -static -O2 -o call_iamroot call_iamroot.c
```

---

## 4. Runbook por fase

### Fase 1 — BusyBox
```bash
mkdir -p "$WORKDIR" && cd "$WORKDIR"
git clone https://github.com/mirror/busybox.git
cd busybox
git checkout 1_36_1
make menuconfig          # aplicar §3.2 (desabilitar tc; static binary ON)
make -j"$NPROC"
make install             # gera ./_install
```
**Validação:** `file _install/bin/busybox | grep -q 'statically linked' && echo OK`

### Fase 2 — Kernel (clone + checkout)
```bash
cd "$WORKDIR"
df -h . | tail -1        # checar >=10GB
git clone --depth 1 --branch v6.6.137 \
  git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
cd linux-stable
git rev-parse HEAD       # deve bater com 62b19b56...
```
> Se `git://` falhar (firewall), troque por `https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git`.

### Fase 3 — (Opcional) Validar kernel limpo + `driver-teste.ko`
Compila o kernel **sem** modificações para testar o ambiente com o driver de teste. Pule se confiar no setup (compila o kernel só uma vez na Fase 5).
```bash
cd "$WORKDIR/linux-stable"
make menuconfig          # aplicar §3.3
scripts/config --disable SYSTEM_TRUSTED_KEYS
scripts/config --disable SYSTEM_REVOCATION_KEYS
make -j"$NPROC" bzImage  # CHECKPOINT: compilação longa
```
Depois monte o initramfs (Fase 6) usando `driver-teste.ko` no `init` e rode o QEMU (Fase 7). `ls /proc` deve mostrar `/proc/IDP-cdriver-teste`.

### Fase 4 — Implementar a syscall
Aplicar §3.4 nos três arquivos. Depois conferir:
```bash
cd "$WORKDIR/linux-stable"
grep -n "iamroot" arch/x86/entry/syscalls/syscall_64.tbl
grep -n "sys_iamroot" include/linux/syscalls.h
grep -n "SYSCALL_DEFINE1(iamroot" kernel/sys.c
```

### Fase 5 — Recompilar kernel (com a syscall)
```bash
cd "$WORKDIR/linux-stable"
make -j"$NPROC" bzImage   # CHECKPOINT: compilação longa
ls -lh arch/x86/boot/bzImage
```
> Erros comuns: include faltando (`implicit declaration of ...`) → revise §3.4(c); símbolo `prepare_kernel_cred` indisponível → garanta que está em `kernel/sys.c` (tem acesso a símbolos internos).

### Fase 6 — Montar o initramfs (com `driver-atividade.ko`)
```bash
cd "$WORKDIR"
rm -rf initramfs && mkdir -p initramfs/{bin,sbin,etc,proc,sys,dev,usr/bin,usr/sbin}
cp -a busybox/_install/* initramfs/

# driver da ATIVIDADE (kernel modificado) + programa de usuario
cp "$DRIVERS_DIR/driver-atividade.ko" initramfs/
cp call_iamroot initramfs/bin/        # ja compilado estatico (§3.5)
```
Criar `initramfs/init` (note: `insmod /driver-atividade.ko`):
```sh
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
mknod -m 666 /dev/ttyS0 c 4 64

mdev -s

insmod /driver-atividade.ko

echo -e "\nBem-vindo ao IDP Linux!\n\n"
echo -e "Boot levou $(cut -d' ' -f1 /proc/uptime) segundos!\n\n"

setsid cttyhack setuidgid 1000 sh

poweroff -d 0 -f

exec /bin/sh
```
```bash
chmod +x initramfs/init
```
Dispositivos especiais (root):
```bash
cd "$WORKDIR/initramfs"
sudo sh -c '
mount -n -t tmpfs none dev 2>/dev/null
mknod -m 622 dev/console c 5 1 2>/dev/null
mknod -m 666 dev/null    c 1 3 2>/dev/null
mknod -m 666 dev/zero    c 1 5 2>/dev/null
mknod -m 666 dev/ptmx    c 5 2 2>/dev/null
mknod -m 666 dev/tty     c 5 0 2>/dev/null
mknod -m 444 dev/random  c 1 8 2>/dev/null
mknod -m 444 dev/urandom c 1 9 2>/dev/null
chown root:tty dev/{console,ptmx,tty} 2>/dev/null
'
```
Empacotar:
```bash
cd "$WORKDIR/initramfs"
find . -print0 | cpio --null -ov --format=newc | gzip -9 > ../initramfs.cpio.gz
```

### Fase 7 — Executar no QEMU e chamar a syscall
```bash
cd "$WORKDIR"
qemu-system-x86_64 \
  -kernel linux-stable/arch/x86/boot/bzImage \
  -initrd initramfs.cpio.gz \
  -nographic -append "console=ttyS0 loglevel=3"   # CHECKPOINT
```
Dentro do sistema (shell uid 1000):
```sh
ls /proc                 # confirmar o nome do driver -> ajustar DRIVER_PATH se preciso
id                       # uid=1000 antes
call_iamroot             # chama a syscall
id                       # deve virar uid=0 (root)
dmesg | tail -n 20       # ver matricula, hora e o CODIGO (~16 chars) = RESPOSTA
```
Sair do QEMU: `Ctrl-A` depois `X`.

> Se o `call_iamroot` não estiver no PATH, rode `/bin/call_iamroot`.

### Fase 8 — Entregáveis
```bash
cd "$WORKDIR/linux-stable"
git diff > "$WORKDIR/iamroot-syscall.patch"
cat "$WORKDIR/iamroot-syscall.patch" | colordiff   # conferir
```

---

## 5. Checklist de entrega (Moodle · até 28/06 23:55)

- [ ] **Resposta final** (código de ~16 caracteres escrito pelo `driver-atividade.ko` no `dmesg`), capturada com matrícula correta.
- [ ] **`iamroot-syscall.patch`** gerado via `git diff` (edições em `syscall_64.tbl`, `syscalls.h`, `sys.c`).
- [ ] `SYSCALL_NR` = 600 + (matrícula mod 100) conferido.
- [ ] Driver usado = `driver-atividade.ko` (não o `-teste`), em kernel já modificado.
- [ ] `id` confirmou elevação para uid=0 dentro do QEMU.

## 6. Armadilhas conhecidas

- **Driver errado:** `driver-teste.ko` só roda em kernel limpo; `driver-atividade.ko` só em kernel modificado ≥1 vez. Cheque com `modinfo <driver>.ko`.
- **`DRIVER_PATH`:** valide o nome real com `ls /proc` antes de fixar no `sys.c`.
- **Static linking do user program:** sem `-static` o binário não roda no initramfs.
- **`prepare_kernel_cred(NULL)`:** correto no 6.6 (retorna `init_cred`/root). Não passar `current`.
- **Espaço em disco:** kernel ocupa muito; mantenha ≥10 GB livres.