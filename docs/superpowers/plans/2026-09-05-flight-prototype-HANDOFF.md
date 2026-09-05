# Fase 1 — Handoff de status

Documento de retomada. Escrito porque a implementação vai continuar em outra sessão/conta. Cobre: o que já está pronto, o que falta, e todo obstáculo de ambiente já resolvido (pra não repetir troubleshooting).

Referências:
- Spec completa: `docs/superpowers/specs/2026-09-05-futebol-aviao-design.md`
- Plano de implementação (Fase 1, com código completo de cada task): `docs/superpowers/plans/2026-09-05-flight-prototype.md`

## Ambiente já configurado nesta máquina

- **Unreal Engine 5.8** instalada em `C:\Program Files\Epic Games\UE_5.8`.
- **Visual Studio Build Tools 2022** instalado em `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools`, com:
  - Workload **"Desktop development with C++"** (traz MSVC v143 + Windows SDK). *Não* existe workload "Game development with C++" na SKU Build Tools (só na Community/Professional completas) — não perder tempo tentando instalar essa.
  - Componente individual **.NET Framework 4.8.1 SDK** — obrigatório à parte, senão o UnrealBuildTool falha com `Unable to instantiate module 'SwarmInterface': Could not find NetFxSDK install dir`.
- Editor de código usado: **Cursor** (baseado em VS Code). Ele só edita texto — quem compila é o MSVC via linha de comando ou via o próprio editor da Unreal.

## Repositório e branch

- Repo local: `C:\Users\cauad\Desktop\dev\jogao` (sem remote configurado — só local).
- Todo o trabalho de código está na branch `flight-prototype`, feita numa worktree em `C:\Users\cauad\Desktop\dev\jogao\.worktrees\flight-prototype`.
- **Se for continuar de outra conta/máquina:** a branch `flight-prototype` já existe no histórico do git (`git log flight-prototype` mostra os commits abaixo). Não precisa recriar a worktree do zero — ou dá `git worktree add` de novo apontando pra essa branch, ou só faz `git checkout flight-prototype` direto se preferir trabalhar sem worktree.
- Histórico de commits da branch (mais recente primeiro):
  ```
  68e8da1 feat: add PlanePawn wiring flight physics to actor movement and input
  89b4e65 fix: give FFlightPhysics a default constructor for UHT vtable-helper compatibility
  47f3026 feat: scaffold Unreal project with compiling editor target
  8d812d9 Ignore Unreal build output and standalone test binaries
  f287875 feat: add pure flight physics module with standalone tests
  ee177e3 Ignore local worktrees directory
  ecd0b57 Add implementation plan for Phase 1 flight prototype
  9bb5ff0 Add design spec for futebol de aviao game
  ```

## Status das tasks (plano de Fase 1)

| Task | Status | Commit |
|---|---|---|
| Task 1 — Física de voo pura + testes standalone | ✅ Feito, testes passando | `f287875` |
| Fix — construtor padrão em `FFlightPhysics` (necessário pro UHT) | ✅ Feito | `89b4e65` |
| Task 2 — Scaffold do projeto Unreal (compila) | ✅ Feito, build "Succeeded" | `47f3026` |
| Task 3 — `APlanePawn` (liga física + input + câmera) | ✅ Feito, build "Succeeded" | `68e8da1` |
| Task 4 — Nível de teste + playtest manual | ⏳ **Pendente** — só dá pra fazer na interface gráfica do editor |

## Gotchas resolvidos (não repetir)

1. **`winget install ... --override "--add Microsoft.VisualStudio.Workload.NativeGame"`** não funciona na SKU Build Tools — esse workload não existe nela. Usar `Microsoft.VisualStudio.Workload.NativeDesktop` (ou simplesmente instalar via GUI marcando "Desktop development with C++").
2. **`winget install` num pacote já instalado não reaplica `--override`** a não ser que use `--force` junto — senão ele vê "already installed" e não faz nada.
3. **NetFxSDK ausente:** erro `Could not find NetFxSDK install dir` ao compilar `FutebolAviaoEditor`. Resolvido instalando o componente individual **.NET Framework 4.8.1 SDK** pelo Visual Studio Installer (aba "Individual components", buscar ".NET Framework").
4. **"Unable to build while Live Coding is active":** se o Unreal Editor estiver aberto (mesmo minimizado), a build por linha de comando (`Build.bat`) falha. Fechar o `UnrealEditor.exe`/`LiveCodingConsole.exe` antes de compilar via terminal.
5. **`vswhere.exe` not recognized** ao chamar `VsDevCmd.bat`: não afeta o resultado, é só um aviso — o `vcvarsall.bat`/`cl.exe` funcionam normalmente mesmo com esse erro aparecendo. Preferir chamar `vcvars64.bat` direto (mais simples que `VsDevCmd.bat`).
6. **`FFlightPhysics` sem construtor padrão** quebrava a compilação do `APlanePawn` com `error C2512: no appropriate default constructor available`, porque o UHT gera um "vtable-helper constructor" pra classes `UCLASS` que exige que todo membro C++ comum (não-UPROPERTY) seja default-construtível. Corrigido adicionando `FFlightPhysics() : Params() {}` em `FlightPhysics.h`.

## Comandos de build que funcionam (confirmados nesta máquina)

**Compilar e rodar os testes standalone da física de voo** (não depende da Unreal, só do MSVC):
```bash
cmd /c '"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && cd /d "C:\Users\cauad\Desktop\dev\jogao\.worktrees\flight-prototype" && cl /EHsc /std:c++17 /Fe:Tools\FlightPhysicsTests\FlightPhysicsTests.exe Tools\FlightPhysicsTests\FlightPhysicsTests.cpp Source\FutebolAviao\Flight\FlightPhysics.cpp && Tools\FlightPhysicsTests\FlightPhysicsTests.exe'
```

**Compilar o editor do jogo** (fechar o Unreal Editor antes, se estiver aberto):
```bash
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" FutebolAviaoEditor Win64 Development -Project="C:\Users\cauad\Desktop\dev\jogao\.worktrees\flight-prototype\FutebolAviao.uproject" -WaitMutex
```

Ambos rodaram do diretório da worktree (`C:\Users\cauad\Desktop\dev\jogao\.worktrees\flight-prototype`) — ajustar o caminho se estiver rodando de outro checkout.

## Task 4 — o que falta (só via editor gráfico)

1. Abrir `FutebolAviao.uproject` (duplo clique, ou `UnrealEditor.exe "<caminho>\FutebolAviao.uproject"`). Confirmar compilação de módulos se perguntado.
2. **File > New Level** → template "Basic". Adicionar um `PlayerStart` numa altura razoável (ex: Z = 500) pra começar já no ar. Salvar como `Content/Maps/TestFlightMap.umap`.
3. **Edit > Project Settings > Maps & Modes:** setar "Editor Startup Map" e "Game Default Map" para `TestFlightMap`. Confirmar que "Default GameMode" já mostra `FutebolAviaoGameModeBase` (herdado do `DefaultEngine.ini`).
4. Apertar **Play** (Alt+P) e testar:
   - `W` acelera, `S` desacelera/freia.
   - Mouse (X/Y) vira o nariz do avião (yaw/pitch).
   - `A`/`D` rola o avião (roll).
   - Câmera (spring arm) segue atrás do avião sem travar.
5. Commit final da Task 4:
   ```bash
   git add Content/Maps/TestFlightMap.umap Config/DefaultEngine.ini
   git commit -m "feat: add test flight level as project default map"
   ```

## Depois da Task 4

Fase 1 completa = protótipo de voo em mãos. Ajustar sensação de voo (aceleração, velocidade máxima, taxas de rotação) editando os defaults em `FFlightPhysicsParams` (`Source/FutebolAviao/Flight/FlightPhysics.h`) — isolado do resto do código, seguro de mexer sem quebrar nada.

Próxima fase (Fase 2 — combustível + boost) ainda não tem plano escrito; a spec já cobre o design dela (ver seção "Mecânicas centrais" e "Fases de desenvolvimento" em `docs/superpowers/specs/2026-09-05-futebol-aviao-design.md"). Ao retomar, invocar a skill `writing-plans` de novo pra gerar o plano da Fase 2 antes de codar.
