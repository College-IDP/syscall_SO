# Trabalho Prático 03 (T03)

**Instituto Brasileiro de Ensino, Desenvolvimento e Pesquisa — IDP Asa Norte**
Ciência da Computação e Engenharia de Software

## Identificação da Disciplina

| Código da Disciplina | Nome da Disciplina | Professor | Período |
|---|---|---|---|
| CIC_6MA e EGS_6MA | Sistemas Operacionais | Jeremias Moreira Gomes | 2026/1 |

---

## 1. Objetivo

O objetivo deste trabalho é capacitar o aluno a estudar e compreender conceitos envolvendo o desenvolvimento de Sistemas Operacionais. Utilizando o conhecimento teórico adquirido nas aulas e ao longo do curso, o aluno deverá escrever uma **Chamada de Sistema** (syscall) para o Kernel do Linux.

Ao cumprir os objetivos do trabalho, o aluno terá adquirido: (i) uma compreensão prática dos conceitos e características-chave sobre a construção de ambientes de desenvolvimento de Sistemas Operacionais; (ii) maior aprimoramento das suas habilidades de programação, organização de informações e manuseio do ambiente Linux; e (iii) essa experiência proporcionará maior confiança e capacidade para explorar esses conceitos na solução de problemas computacionais.

## 2. Ambiente de Desenvolvimento Linux

O objetivo deste trabalho é escrever uma chamada de sistema em um ambiente de desenvolvimento simplificado para o Kernel do Linux. Para isso, diversas ferramentas serão explicadas e utilizadas ao longo deste documento. As configurações e instalações aqui descritas foram montadas e testadas no ambiente Linux disponibilizado no laboratório Dell do IDP, o qual possui instalado a ferramenta WSL2, Sistema Operacional Ubuntu 24.04 e o Kernel `6.6.87.2-microsoft-standard-WSL2`. Este pode ser replicado para ambientes similares, mas esteja ciente que a montagem em ambientes diferentes pode exigir ajustes nas configurações.

Assim, para a realização deste trabalho, serão necessários os seguintes passos:

- Atualização do Sistema;
- Instalação de Ferramentas Básicas Necessárias;
- Instalação do Emulador QEMU;
- Instalação do Busybox;
- Compilação do Kernel Linux;
- Construção de um Sistema Operacional com Sistema de Arquivos em RAM com Driver de Caractere incluso; e
- Execução com QEMU;
- Manipulação de Drivers de Caractere;
- Escrita da Chamada de Sistema;
- Execução da Chamada de Sistema desenvolvida.

### 2.1. Ferramentas Básicas

Abaixo, segue uma lista de ferramentas que podem ser instaladas via próprio sistema (`apt-get`) e serão necessárias direta ou indiretamente na realização do trabalho:

- `build-essential`
- `libncurses-dev`
- `bison`
- `flex`
- `libssl-dev`
- `libelf-dev`
- `pahole`
- `cpio`
- `colordiff`

### 2.2. Emulador QEMU

O QEMU (Quick Emulator) é um software de código aberto que permite emular sistemas operacionais e arquiteturas de hardware diferentes em um único computador. Ele é um emulador de máquina virtual, que fornece um ambiente virtualizado para possibilitar a execução de um sistema operacional dentro de outro.

Isso permite criar máquinas virtuais que executam sistemas operacionais diferentes, permitindo aos usuários testar novas versões de um sistema operacional sem o instalar diretamente em seu computador principal. Também é possível criar diferentes ambientes de desenvolvimento para diferentes projetos, sem precisar comprar hardware adicional.

Além disso, o QEMU permite emular diferentes arquiteturas de hardware em um único computador, o que pode ser útil para testar software em diferentes plataformas sem a necessidade de ter cada hardware fisicamente disponível.

O QEMU pode ser instalado via `apt-get` e, neste trabalho, utilizaremos apenas a execução em arquiteturas x86-64, que pode ser instalado com o seguinte comando:

```bash
sudo apt-get install qemu-system qemu-utils qemu-system-x86
```

Para testar a instalação, você pode utilizar o parâmetro `version` da versão que será utilizada:

```bash
qemu-system-x86_64 --version
```

### 2.3. Busybox

BusyBox é uma ferramenta única que possui um conjunto típico de ferramentas de um sistema UNIX em um único arquivo executável. Tipicamente, em um sistema Linux, o sistema possui diferentes ferramentas nas pastas `/bin`, `/sbin`, `/usr/bin`, `/usr/sbin`, etc. O BusyBox é um executável que combina várias dessas ferramentas em um único executável e, ao invés de ter todas essas ferramentas em pastas diferentes, com o BusyBox você cria links simbólicos para cada ferramenta que deseja utilizar para esse único executável que se passa por todos eles (um truque para diminuir espaço ocupado por essas ferramentas em sistemas embarcados).

A instalação do BusyBox será específica para a atividade, assim, esta deverá ser compilada a partir do código fonte. O link para clonar o repositório é `https://github.com/mirror/busybox.git`.

Após baixar, você deverá selecionar a tag que contém o commit que será utilizado. A versão é a **1.36.1** (commit `1a64f6a20aaf6ea4dbba68bbfa8cc1ab7e5c57c4`) e a tag é `1_36_1` (utilize o comando `git checkout <tag>` para alternar).

O projeto utiliza a ferramenta `make` e, para ajustar as configurações, você deverá utilizar o comando `make menuconfig`. Com as opções de configurações abertas, ajuste as seguintes opções:

1. **Networking Utilities**: desabilitar a ferramenta `tc`, que não é mais suportada na versão do Kernel que iremos utilizar.
2. **Settings → Build Options**: habilitar "Build static binary", para incluir durante a compilação todas as bibliotecas necessárias dentro do executável.

Após salvar as configurações, compile com o comando `make -j <PROCESSOS>`, onde `<PROCESSOS>` é o número de processos que deseja utilizar para compilar o BusyBox.

Terminada a compilação, o comando para instalar é `make install`, que irá criar uma pasta chamada `_install` com os binários do BusyBox, que serão utilizados posteriormente.

### 2.4. Kernel Linux

O Kernel Linux é o núcleo do sistema operacional Linux, responsável por gerenciar os recursos do sistema, como memória, processos, dispositivos de entrada e saída, entre outros.

O Kernel Linux é um software de código aberto, mantido por uma comunidade de desenvolvedores de todo o mundo. O código-fonte do Kernel do Linux encontra-se em diversos locais, porém o recomendado é acessar via `https://kernel.org/`, que mantém a versão estável do sistema em `git://git.kernel.org/`, além de diversas variações utilizadas para outros ambientes e propósitos.

Para a realização deste trabalho, será utilizada a versão mais próxima daquela disponível no Laboratório Dell, que é a utilizada pela Microsoft no ambiente WSL2: versão **6.6.137**. Para isso, clone o repositório no seguinte link: `git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git`.

Ao acessar a pasta clonada, alterne para a versão que possui a tag `v6.6.137` (`git checkout <tag>`), cujo commit é:

```
62b19b564504d027296502e0e30baf5fe0ef080a
```

Para configurar a compilação do kernel, utilize o comando `make menuconfig` e garanta as seguintes opções:

- **Initial RAM filesystem and RAM disk (initramfs/initrd) support**: opção habilitada
- **64-bit kernel**: opção habilitada
- **Device Drivers → Generic Driver Options**:
  - **Maintain a devtmpfs filesystem to mount at /dev**: opção habilitada
  - **Automount devtmpfs at /dev, after the kernel mounted the rootfs**: opção habilitada

#### 2.4.1. Assinatura de Drivers

Em alguns casos, por motivos de segurança, o Kernel pode exigir que os drivers sejam assinados para que possam ser carregados durante a utilização do sistema. Para desabilitar essa verificação (facilitar o desenvolvimento e testes), você pode utilizar o seguinte:

```bash
scripts/config --disable SYSTEM_TRUSTED_KEYS
scripts/config --disable SYSTEM_REVOCATION_KEYS
```

Com isso, o Kernel já está pronto para ser compilado, utilizando o comando `make -j <PROCESSOS> bzImage`, onde `<PROCESSOS>` é o número de processos e `bzImage` é o arquivo que será gerado com o Kernel pronto para ser utilizado.

### 2.5. Construção de um Sistema Operacional com Sistema de Arquivos em RAM

De posse do Kernel compilado, Busybox e QEMU instalados, agora você pode construir um sistema operacional de maneira simplificada. Um último item que precisa ser gerado é o sistema de arquivos ao qual o Kernel irá acessar, porém neste trabalho tudo irá ser executado diretamente em memória RAM.

Para isso, crie em um local separado uma pasta chamada `initramfs`. Dentro dessa pasta, crie a seguinte estrutura de diretórios:

1. `bin`
2. `sbin`
3. `etc`
4. `proc`
5. `sys`
6. `dev`
7. `usr/bin`
8. `usr/sbin`

Em seguida, copie os arquivos gerados pelo Busybox na pasta `_install` do BusyBox para a pasta `initramfs`, da seguinte maneira:

```bash
cp -a busybox/_install/* initramfs/
```

Com isso, a estrutura de diretórios do sistema operacional está pronta. Faltam agora dois passos: (1) indicar o que será executado quando o sistema inicializar e (2) criar alguns dispositivos especiais para o sistema operacional.

#### 2.5.1. Init

O sistema operacional precisa de um programa que será executado quando o sistema inicializar. Este programa é chamado de `init` e, assim que o sistema estiver totalmente carregado e testes realizados, o `init` será executado. Assim, crie o arquivo `init` dentro da pasta `initramfs`, e este deverá conter o seguinte:

```sh
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
mknod -m 666 /dev/ttyS0 c 4 64

mdev -s

insmod /driver-teste.ko

echo -e "\nBem-vindo ao IDP Linux!\n\n"
echo -e "Boot levou $(cut -d' ' -f1 /proc/uptime) segundos!\n\n"

setsid cttyhack setuidgid 1000 sh

poweroff -d 0 -f

exec /bin/sh
```

Repare que há algumas diferenças nesse arquivo em relação ao Trabalho 01. A primeira delas é o usuário que será utilizado para executar o sistema operacional, que será o usuário com o ID 1000, significando que o mesmo não possui privilégios de administrador[^1]. A segunda é a inserção de um módulo de driver, por meio do comando `insmod`. A atividade disponibiliza dois módulos de driver: o primeiro, com o nome `driver-teste.ko`, deverá ser utilizado em um Kernel sem modificações, para fins de teste. Já o segundo, com o nome `driver-atividade.ko`, deverá ser utilizado após a escrita da chamada de sistema descrita no próximo tópico.

Após criar o arquivo `init`, você deverá torná-lo executável com o comando `chmod +x init`.

[^1]: O ID do usuário root é 0.

#### 2.5.2. Dispositivos Especiais

O sistema operacional precisa de alguns dispositivos especiais para funcionar corretamente. Para isso, acesse a pasta `initramfs` e execute o seguinte (com permissões de administrador):

```bash
mount -n -t tmpfs none dev 2>/dev/null
mknod -m 622 dev/console c 5 1 2>/dev/null
mknod -m 666 dev/null c 1 3 2>/dev/null
mknod -m 666 dev/zero c 1 5 2>/dev/null
mknod -m 666 dev/ptmx c 5 2 2>/dev/null
mknod -m 666 dev/tty c 5 0 2>/dev/null
mknod -m 444 dev/random c 1 8 2>/dev/null
mknod -m 444 dev/urandom c 1 9 2>/dev/null
chown root:tty dev/{console,ptmx,tty} 2>/dev/null
```

Esses dispositivos servem para que o sistema operacional possa interagir com o usuário, com o hardware e com o próprio sistema operacional.

### 2.6. Compressão de Arquivos para Utilização

Com o sistema operacional pronto, você deverá comprimir a pasta `initramfs` para um arquivo chamado `initramfs.cpio.gz`, que será utilizado pelo Kernel para carregar o sistema operacional. Para isso, utilize o seguinte comando:

```bash
find . -print0 | cpio --null -ov --format=newc | gzip -9 > \
    ../initramfs.cpio.gz
```

Esse arquivo `initramfs.cpio.gz` será utilizado em conjunto com o Kernel compilado na hora de executar o QEMU.

### 2.7. QEMU

Agora que você tem todas as ferramentas necessárias, para executar o sistema utilizando o QEMU você pode digitar:

```bash
qemu-system-x86_64 -kernel bzImage -initrd initramfs.cpio.gz \
    -nographic -append "console=ttyS0 loglevel=3"
```

O que irá iniciar o sistema operacional que você acabou de criar.

### 2.8. Manipulação de Drivers de Caractere

Um dispositivo de caractere é um dispositivo que pode ser acessado como um fluxo de caracteres. Exemplos de dispositivos de caractere incluem terminais, impressoras e dispositivos de comunicação. Os dispositivos de caractere são acessados por meio de arquivos especiais no sistema de arquivos, localizados normalmente na pasta `/dev` ou na pasta `/proc`.

No caso desta atividade, os drivers serão listados na pasta `/proc`. O driver de teste irá aparecer com o nome `/proc/IDP-cdriver-teste`, caso o mesmo tenha sido inserido corretamente (liste com o comando `ls /proc`).

A interação com drivers é feita por meio de chamadas de sistema, onde o dispositivo é manipulado basicamente da mesma forma que um arquivo. Um exemplo de interação com o driver de teste é o seguinte:

```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>

int main() {
    int fd = open("/proc/IDP-cdriver-teste", O_RDWR);
    if (fd < 0) return 1;

    ioctl(fd, 123456, 0);

    close(fd);
    return 0;
}
```

Nesse caso, ao invés de syscalls de leitura e escrita, o driver de caractere é manipulado por meio de `ioctl`, que é uma chamada de sistema genérica para manipulação de dispositivos. Ela recebe três argumentos: o descritor do arquivo, um valor que representa uma operação (`unsigned int`) e um valor que representa um argumento para a operação (`unsigned long`).

Ao testar esse código, o dispositivo irá imprimir uma mensagem nos logs do Kernel (visualize com o comando `dmesg`) informando que a operação foi acionada com sucesso.

## 3. Chamadas de Sistema

Aqui de fato entra a parte que você deverá desenvolver (a parte "legal" do trabalho). Você deverá escrever uma chamada de sistema chamada `iamroot`. Essa chamada irá executar as seguintes tarefas:

1. Imprimir a sua matrícula;
2. Imprimir a hora do sistema;
3. Interagir com o driver de caractere `driver-atividade.ko`, que irá realizar uma operação de escrita no `dmesg`; e
4. Elevar os privilégios do usuário para o nível de administrador (root).

### 3.1. Adicionando uma Nova Chamada de Sistema

A seção a seguir é baseada na documentação oficial (`https://www.kernel.org/doc/html/v6.6/process/adding-syscalls.html`), caso tenha alguma dúvida ou queira informações adicionais.

Para escrever uma chamada de sistema, você deverá seguir os passos a seguir.

#### 3.1.1. Adicionando a Chamada de Sistema

Primeiramente, você deverá alterar o arquivo `arch/x86/entry/syscalls/syscall_64.tbl`, incluindo o número da chamada de sistema, o tipo da chamada de sistema (que será `common`), o nome da chamada de sistema (que será `iamroot`) e o nome da função de referência para a chamada de sistema, que possui o padrão `sys_iamroot`.

O número da chamada de sistema deverá seguir a seguinte regra:

- Deverá ser um inteiro positivo `6XY`, onde `XY` são os dois últimos dígitos da sua matrícula. Por exemplo, se a matrícula for `22221111`, o número da chamada de sistema será `611`.

#### 3.1.2. Incluindo o Protótipo da Função

Em seguida, o arquivo `include/linux/syscalls.h` deverá ser alterado incluindo a assinatura da função `sys_iamroot`, que deverá receber um parâmetro do tipo inteiro, deverá retornar um inteiro longo, e o seu protótipo terá o rótulo `asmlinkage` (especificação para trabalhar apenas na pilha e sem registradores). O local para incluir pode ser em quase qualquer lugar, mas para referência, procure os protótipos referentes ao arquivo `kernel/sys.c`.

#### 3.1.3. Escrevendo a Função

A função `sys_iamroot` deverá ser escrita no arquivo `kernel/sys.c`.

Um detalhe importante é que a adição de funções no Kernel do Linux é feita utilizando macros[^2]. Aqui, você deverá concluir três passos:

1. Definir uma variável booleana chamada `syscall_iamroot_flag`.
2. Exportar a variável `syscall_iamroot_flag` para que possa ser utilizada pelo driver de caractere da atividade.
3. Escrever a função `sys_iamroot` com um parâmetro, por meio da macro `SYSCALL_DEFINE1` (utilize alguma outra chamada de sistema como referência, para entender o funcionamento da macro).

Para definir a variável booleana, utilize a macro `DEFINE_PER_CPU`, que deverá estar da seguinte forma:

```c
DEFINE_PER_CPU(bool, syscall_iamroot_flag);
```

O driver da atividade (`driver-atividade.ko`) irá verificar a existência dessa variável, para devolver o conteúdo correto na resposta. Assim, após a criação, esta precisa ser exportada para o contexto de todo o kernel.

Para exportar a variável, utilize a macro `EXPORT_PER_CPU_SYMBOL`, da seguinte forma:

```c
EXPORT_PER_CPU_SYMBOL(syscall_iamroot_flag);
```

Por último, mas não menos importante, a função `sys_iamroot` deverá executar o seguinte:

- Registrar no arquivo de logs do Kernel a sua matrícula. Para imprimir uma mensagem no log do Kernel, existem algumas opções válidas:
  - A função `printk`, que é a mais comum; ou
  - A função `pr_*`, que é uma alternativa que inclui no nome da função o tipo de mensagem que será impresso como, por exemplo, `pr_info`.
- Registrar no arquivo de logs do Kernel (`dmesg`) o horário em que a chamada de sistema foi executada, utilizando a função.
  - Para coletar a hora do sistema, existem algumas alternativas que podem ser utilizadas, por exemplo a função `ktime_get_real_ts64`.
  - Não precisa obrigatoriamente utilizar esta função, mas o objetivo é mostrar essa informação em horas, minutos e segundos.
- Chamar a função `this_cpu_write` para alterar o valor da variável para `true`;
- Abrir o driver de caractere `driver-atividade.ko`, que irá aparecer como `/proc/proc/IDP-cdriver`;
  - Utilize a função `filp_open` para abrir o driver; esta retorna um ponteiro para a estrutura `struct file`;
  - Acione a operação do driver de caractere, que é feita por meio da função `ioctl`, da seguinte forma: `file->f_op->unlocked_ioctl(file, <matricula>, 0);`, onde `<matricula>` é o número da sua matrícula;
  - Fechar o driver de caractere com a função `filp_close`.
- Chamar a função `this_cpu_write` para alterar o valor da variável para `false`;
- Alterar as credenciais do usuário para o nível de administrador.
  - Para verificar as permissões atuais, utilize a chamada de sistema `getuid()` ou `geteuid()` (isso no espaço de usuário);
  - Para modificar o usuário no nível de kernel, o processo executando possui uma estrutura chamada `task_struct` que possui todas as informações sobre aquele processo, e para alterar algo você pode utilizar as funções `commit_creds` e `prepare_kernel_cred` (pesquise a sintaxe).

Terminadas essas alterações, você deverá compilar o Kernel e, agora, ao construir o sistema operacional, incluir o driver de caractere `driver-atividade.ko`[^3]. Ao executar o sistema operacional, você deverá escrever um programa de usuário que chame a sua chamada de sistema. Caso a execução seja bem sucedida, você deverá ver uma mensagem com o seu nome no `dmesg` e o código, de aproximadamente 16 caracteres, que é a solução para a atividade.

[^2]: É uma forma de conseguir manter compatibilidade entre diferentes arquiteturas e versões do Kernel.

[^3]: Lembre-se que os dois drivers disponibilizados não são compatíveis no mesmo sistema. O de testes somente funciona no kernel sem modificações, enquanto o segundo funciona no kernel modificado pelo menos uma vez. Para verificar informações sobre o driver, você pode utilizar o comando `modinfo <driver>.ko`.

## 4. Sumário

Abaixo, seguem as diretrizes para a execução do trabalho.

### 4.1. Restrições deste Trabalho

Esta atividade deverá ser realizada de maneira **individual**.

### 4.2. Datas Importantes

Esse trabalho poderá ser submetido até o dia **28/06/2026 (domingo) — 23:55h**.

### 4.3. Por onde será a entrega do Trabalho?

O trabalho deverá ser entregue via Moodle na disciplina de Sistemas Operacionais, em uma atividade chamada "Trabalho Prático 03", para submissão da resposta.

### 4.4. O que deverá ser entregue, referente ao trabalho?

Deverá ser entregue a resposta final coletada a partir da execução da syscall `iamroot`, com o valor da matrícula correta, interagindo com o dispositivo de caractere `driver-atividade.ko`. O driver irá escrever a resposta no buffer de mensagens do Kernel, que pode ser consultado a partir do comando `dmesg`.

Além disso, deverá ser submetido também um patch com as alterações realizadas no kernel, para a criação da chamada de sistema. Para gerar o patch, você deve utilizar o comando `git diff`, redirecionando a saída para um arquivo. Exemplo:

```bash
git diff > iamroot-syscall.patch
```

Se quiser visualizar o conteúdo do patch gerado, você pode executar o seguinte:

```bash
cat iamroot-syscall.patch | colordiff
```

### 4.5. Observações importantes

- **Não deixe para fazer o trabalho de última hora**.
- Você precisará de pelo menos 10GB de espaço livre em disco para realizar o trabalho (sim, o Kernel do Linux é muito grande).
- **Não deixe para fazer o trabalho de última hora**.
- Caso não faça o trabalho no laboratório da disciplina (Dell), fique atento para a versão de todos os softwares utilizados.
- **Não deixe para fazer o trabalho de última hora**.

## 5. Bibliografia

1. TANENBAUM, Andrew S. *Sistemas Operacionais Modernos*. Terceira edição. 2010.
2. SALZMAN, Peter Jay et al. *The Linux Kernel Module Programming Guide*. 2001.
3. Linux Kernel Teaching. `https://linux-kernel-labs.github.io/refs/heads/master/index.html`, acesso em 2026.
4. The Linux Kernel. `https://www.kernel.org/doc/html/v6.6/process/adding-syscalls.html`, acesso em 2026.