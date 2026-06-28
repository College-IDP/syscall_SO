# T03 — Orquestração `iamroot` 🧭

> **Markdown principal de organização e execução.** Coordena todas as etapas do Trabalho Prático 03 (Syscall `iamroot`, Sistemas Operacionais — IDP). Cada step referencia o runbook detalhado: [`executor.md`](./executor.md).
>
> **Prazo:** 28/06/2026 23:55 · **Entrega:** Moodle · **Individual**

---

## Visão geral dos steps

| Step | Nome | Saída-chave | Commit |
|------|------|-------------|--------|
| **0** | Ambiente, arquitetura de pastas e GitHub | repo versionado na nuvem | `chore: 🎉 scaffold inicial` |
| **1** | BusyBox | `_install/` estático | `build: 📦 busybox 1.36.1` |
| **2** | Kernel base | `linux-stable` na tag v6.6.137 | `chore: ⬇️ kernel base` |
| **3** | Implementar syscall | 3 arquivos editados | `feat: ✨ syscall iamroot` |
| **4** | Compilar kernel | `bzImage` | `build: 🔨 kernel + syscall` |
| **5** | Montar initramfs | `initramfs.cpio.gz` | `build: 🗜️ initramfs + driver` |
| **6** | Executar e coletar resposta | `resposta.txt` (código ~16 chars) | `docs: 📝 resposta dmesg` |
| **7** | Patch e entrega | `iamroot-syscall.patch` | `chore: 🚀 entregaveis` |

> **Regra de ouro:** cada step só inicia quando a **Validação** do anterior passa. Os steps pesados (4 e 6) exigem confirmação antes de rodar.

---

## Step 0 — Ambiente, arquitetura e GitHub 🏗️

**Objetivo:** deixar o ambiente pronto, a estrutura de pastas criada e o repositório versionado na nuvem antes de tocar em qualquer código.

### 0.1 Dependências do sistema
Instale as ferramentas conforme o host (detalhes e equivalência WSL2 no runbook §2):
```bash
# Arch Linux (daily driver)
sudo pacman -S --needed base-devel bc cpio ncurses openssl libelf pahole \
    qemu-system-x86 qemu-img git colordiff github-cli
```
Validação: `qemu-system-x86_64 --version && gh --version`

### 0.2 Arquitetura de pastas
```bash
export WORKDIR="$HOME/t03"
mkdir -p "$WORKDIR"/{docs,src/kernel,src/user,src/initramfs,scripts,build,drivers,deliver}
cd "$WORKDIR"
```
Estrutura resultante:
```
t03/
├── README.md
├── .gitignore
├── docs/                  # enunciado + runbook (este orquestrador também)
├── src/
│   ├── kernel/            # snippets de referência das edições (sys.c, .tbl, syscalls.h)
│   ├── user/              # call_iamroot.c
│   └── initramfs/         # init
├── scripts/               # 00-setup.sh ... 07-deliver.sh
├── build/      (ignorado) # linux-stable, busybox, initramfs, bzImage, *.cpio.gz
├── drivers/    (ignorado) # driver-teste.ko, driver-atividade.ko
└── deliver/               # iamroot-syscall.patch, resposta.txt
```
> **Por que separar `build/` e `drivers/`?** O kernel + busybox passam de vários GB e os `.ko` são binários da disciplina — nada disso vai pro Git. Só o que é seu (código, scripts, docs, patch, resposta) é versionado.

Mova os artefatos já existentes para dentro da estrutura:
```bash
cp /caminho/executor.md     docs/
cp /caminho/requirements.md docs/
# os .ko baixados do Moodle:
cp /caminho/driver-*.ko drivers/
```

### 0.3 `.gitignore`
```bash
cat > .gitignore <<'EOF'
# Artefatos pesados de build (não versionar)
build/
drivers/*.ko
*.cpio.gz
bzImage
*.o
*.ko

# Resposta final fica em deliver/, mas binários não
call_iamroot

# Sistema
.DS_Store
*.swp
EOF
```

### 0.4 README inicial
```bash
cat > README.md <<'EOF'
# T03 — Syscall `iamroot`

Implementação de uma chamada de sistema no Kernel Linux 6.6.137 que imprime
matrícula e hora, interage com um char driver e eleva privilégios para root.
Disciplina de Sistemas Operacionais — IDP, 2026/1.

## Organização
- `docs/` — enunciado e runbook de execução
- `src/`  — código (syscall, programa de usuário, init)
- `scripts/` — automação por step
- `deliver/` — patch e resposta final

Execução guiada pelo orquestrador em `docs/orchestrator.md`.
EOF
```

### 0.5 Inicializar Git + fluxo (Conventional Commits + gitmoji)
```bash
git init -b main
git add .
git commit -m "chore: 🎉 scaffold inicial do T03 (ambiente, pastas, docs)"

# branch de trabalho (gitflow)
git branch develop
git checkout develop
```

### 0.6 Subir para o GitHub
```bash
gh auth status || gh auth login        # se ainda não autenticado
gh repo create t03-syscall-iamroot --private --source=. --remote=origin
git push -u origin main
git push -u origin develop
```
> Repo **privado** (é entrega acadêmica individual). Trabalhe em `develop` e faça commit ao fim de cada step; faça merge em `main` só ao final.

**Validação do Step 0:** `gh repo view --web` abre o repositório com o scaffold versionado. ✅

---

## Step 1 — BusyBox 📦
**Depende de:** Step 0 · **Runbook:** Fase 1
**Ações:** clonar mirror, `checkout 1_36_1`, `menuconfig` (desabilitar `tc`, *static binary* ON), `make && make install` em `build/busybox`.
**Validação:** `file build/busybox/_install/bin/busybox | grep -q 'statically linked'`
**Commit:** `build: 📦 busybox 1.36.1 estatico`

---

## Step 2 — Kernel base ⬇️
**Depende de:** Step 1 · **Runbook:** Fase 2
**Ações:** checar ≥10 GB livres; `git clone --depth 1 --branch v6.6.137 ... build/linux-stable`; conferir commit `62b19b56…`.
**Validação:** `git -C build/linux-stable rev-parse HEAD` bate com o commit da spec.
**Commit:** `chore: ⬇️ kernel linux-stable v6.6.137`
> O `linux-stable` está em `build/` (ignorado) — o commit aqui só registra scripts/notas, não o código do kernel.

---

## Step 3 — Implementar a syscall ✨
**Depende de:** Step 2 · **Runbook:** Fase 4 + Spec §3.4
**Ações:** editar os 3 arquivos do kernel (`syscall_64.tbl`, `syscalls.h`, `sys.c`) com `MATRICULA`, `SYSCALL_NR` e `DRIVER_PATH` preenchidos. Salvar cópias de referência em `src/kernel/`.
**Validação:** os três `grep` da Fase 4 retornam as inserções.
**Commit:** `feat: ✨ adiciona syscall iamroot (nr SYSCALL_NR)`

---

## Step 4 — Compilar o kernel 🔨
**Depende de:** Step 3 · **Runbook:** Fase 5 · ⚠️ **CHECKPOINT (compilação longa)**
**Ações:** `scripts/config` para desabilitar trusted/revocation keys; `make -j bzImage`.
**Validação:** `ls -lh build/linux-stable/arch/x86/boot/bzImage`
**Commit:** `build: 🔨 compila kernel com syscall`

---

## Step 5 — Montar o initramfs 🗜️
**Depende de:** Step 4 · **Runbook:** Fase 6
**Ações:** criar estrutura, copiar busybox, `call_iamroot` (estático), `driver-atividade.ko`; escrever `init` com `insmod /driver-atividade.ko`; criar dispositivos especiais; empacotar `initramfs.cpio.gz`.
**Validação:** `initramfs.cpio.gz` gerado em `build/`.
**Commit:** `build: 🗜️ initramfs com driver-atividade`

---

## Step 6 — Executar e coletar a resposta 📝
**Depende de:** Step 5 · **Runbook:** Fase 7 · ⚠️ **CHECKPOINT (QEMU + elevação root)**
**Ações:** subir QEMU; dentro: `ls /proc` (confirmar `DRIVER_PATH`), rodar `call_iamroot`, conferir `id` virou uid=0, capturar `dmesg | tail`.
**Saída:** salvar o código (~16 chars) em `deliver/resposta.txt`.
**Validação:** `resposta.txt` contém matrícula correta + código.
**Commit:** `docs: 📝 resposta final do dmesg`

---

## Step 7 — Patch e entrega 🚀
**Depende de:** Step 6 · **Runbook:** Fase 8
**Ações:** `git -C build/linux-stable diff > deliver/iamroot-syscall.patch`; conferir com `colordiff`.
**Ações finais (git):**
```bash
cd "$WORKDIR"
git add deliver/ src/ docs/
git commit -m "chore: 🚀 entregaveis (patch + resposta)"
git checkout main && git merge --no-ff develop -m "release: 🎓 T03 concluido"
git push origin main develop
```
**Validação — checklist Moodle:**
- [ ] `resposta.txt` com código ~16 chars + matrícula correta
- [ ] `iamroot-syscall.patch` (3 arquivos)
- [ ] driver = `driver-atividade.ko` em kernel modificado
- [ ] uid=0 confirmado no QEMU

---

## Mapa de dependências
```
Step 0 ─┬─> Step 1 ──> Step 2 ──> Step 3 ──> Step 4 ──> Step 5 ──> Step 6 ──> Step 7
        └── (github sempre ativo: commit ao fim de cada step)
```