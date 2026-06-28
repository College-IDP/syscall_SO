# T03 — Syscall `iamroot`

Implementação de uma chamada de sistema no Kernel Linux 6.6.137 que imprime
matrícula e hora, interage com um char driver e eleva privilégios para root.
Disciplina de Sistemas Operacionais — IDP, 2026/1.

## Organização
- `docs/` — enunciado e runbook de execução (`requirements.md`, `executor.md`, `orchestrator.md`)
- `src/` — código (syscall, programa de usuário, init)
- `scripts/` — automação por step
- `build/` — artefatos pesados (kernel, busybox, initramfs) — ignorado no Git
- `drivers/` — `driver-teste.ko` e `driver-atividade.ko` — ignorado no Git
- `deliver/` — patch e resposta final

Execução guiada pelo orquestrador em `docs/orchestrator.md`.
