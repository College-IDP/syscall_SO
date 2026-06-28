# T03 — Syscall `iamroot`

<img width="816" height="657" alt="anatomia-syscall" src="https://github.com/user-attachments/assets/5769c9c6-275c-4ea0-9b26-de068d846013" />

Implementação de uma chamada de sistema no Kernel Linux 6.6.13 que imprime
matrícula e hora, interage com um char driver e eleva privilégios para root.
Disciplina de Sistemas Operacionais — IDP, 2026/1.

> Obs.: o enunciado cita "6.6.137", mas o commit indicado e o `vermagic` dos
> drivers correspondem à **v6.6.13** (confirmado com o professor).

## Organização
- `docs/` — enunciado e runbook de execução (`requirements.md`, `executor.md`, `orchestrator.md`)
- `src/` — código (syscall, programa de usuário, init)
- `scripts/` — automação por step
- `build/` — artefatos pesados (kernel, busybox, initramfs) — ignorado no Git
- `drivers/` — `driver-teste.ko` e `driver-atividade.ko` — ignorado no Git
- `deliver/` — patch e resposta final

Execução guiada pelo orquestrador em `docs/orchestrator.md`.
