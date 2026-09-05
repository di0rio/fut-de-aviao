# Fase 1 — Handoff de status

Documento de retomada. Cobre o que está pronto, o que vem depois, e os obstáculos de ambiente já resolvidos (pra não repetir troubleshooting).

**Status: Fase 1 COMPLETA.** Todas as 4 tasks feitas, playtest aprovado, branch mergeada na `master`.

Referências:
- Spec completa: `docs/superpowers/specs/2026-09-05-futebol-aviao-design.md`
- Plano de implementação (Fase 1, com código completo de cada task): `docs/superpowers/plans/2026-09-05-flight-prototype.md`

## Ambiente já configurado nesta máquina

- **Unreal Engine 5.8** instalada em `C:\Program Files\Epic Games\UE_5.8`.
- **Visual Studio Build Tools 2022** em `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools`, com:
  - Workload **"Desktop development with C++"** (MSVC v143 + Windows SDK). *Não* existe workload "Game development with C++" na SKU Build Tools (só nas edições Community/Professional) — não perder tempo tentando instalar essa.
  - Componente individual **.NET Framework 4.8.1 SDK** — obrigatório à parte, senão o UnrealBuildTool falha com `Unable to instantiate module 'SwarmInterface': Could not find NetFxSDK install dir`.
- Editor de código: **Cursor** (baseado em VS Code). Ele só edita texto — quem compila é o MSVC via linha de comando ou o próprio editor da Unreal.

## Repositório

- Repo local: `C:\Users\cauad\Desktop\dev\jogao`.
- **Remote:** `https://github.com/di0rio/mudarnomeainda.git` (`origin`). *(Versões anteriores deste doc diziam que não havia remote — estava errado.)*
- Todo o trabalho da Fase 1 está na `master`. A branch `flight-prototype` e a worktree em `.worktrees/` foram removidas depois do merge — não existem mais.
- O remote ainda tem `refs/heads/flight-prototype` apontando pra um commit antigo; a `master` local está à frente do `origin/master` e ainda não foi pushada.

## Status das tasks (Fase 1)

| Task | Status | Commit |
|---|---|---|
| Task 1 — Física de voo pura + testes standalone | ✅ | `f287875` |
| Fix — construtor padrão em `FFlightPhysics` (necessário pro UHT) | ✅ | `89b4e65` |
| Task 2 — Scaffold do projeto Unreal (compila) | ✅ | `47f3026` |
| Task 3 — `APlanePawn` (liga física + input + câmera) | ✅ | `68e8da1` |
| Task 4 — Nível de teste + playtest manual | ✅ | `fb66dc6` |

### Como a Task 4 foi feita (importante)

O plano original dizia que criar o `.umap` exigia a GUI do editor. **Não exige.** O nível é gerado headless por um commandlet Python:

```bash
"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" \
  "C:\Users\cauad\Desktop\dev\jogao\FutebolAviao.uproject" \
  -run=pythonscript \
  -script="C:\Users\cauad\Desktop\dev\jogao\Tools\LevelBuilder\BuildTestFlightMap.py" \
  -unattended -nosplash -nop4
```

O script (`Tools/LevelBuilder/BuildTestFlightMap.py`) cria `Content/Maps/TestFlightMap.umap` com chão 400×400, DirectionalLight, SkyLight, SkyAtmosphere e PlayerStart em Z=500. É idempotente — rodar de novo recria o nível do zero.

Dois detalhes de design que valem manter:
- O nível é criado com `new_level(path, bIsPartitionedWorld=False)`. **Sem World Partition de propósito:** sai um `.umap` único de ~17 KB em vez de uma árvore `__ExternalActors__` com dezenas de arquivos, o que é muito mais sadio de versionar.
- `PythonScriptPlugin` e `EditorScriptingUtilities` estão habilitados no `.uproject` com `"TargetAllowList": ["Editor"]` — não entram em build de jogo.

## Gotchas resolvidos (não repetir)

1. **`winget install ... --override "--add Microsoft.VisualStudio.Workload.NativeGame"`** não funciona na SKU Build Tools — esse workload não existe nela. Usar `Microsoft.VisualStudio.Workload.NativeDesktop` (ou instalar via GUI marcando "Desktop development with C++").
2. **`winget install` num pacote já instalado não reaplica `--override`** a não ser que use `--force` junto.
3. **NetFxSDK ausente:** erro `Could not find NetFxSDK install dir` ao compilar `FutebolAviaoEditor`. Resolvido instalando o componente **.NET Framework 4.8.1 SDK** pelo Visual Studio Installer ("Individual components" → buscar ".NET Framework").
4. **"Unable to build while Live Coding is active":** se o Unreal Editor estiver aberto (mesmo minimizado), `Build.bat` falha. Fechar `UnrealEditor.exe`/`LiveCodingConsole.exe` antes de compilar via terminal. O mesmo vale pra rodar commandlets — o editor aberto segura o projeto.
5. **`vswhere.exe` not recognized** ao chamar `vcvars64.bat`: é só um aviso, o `cl.exe` funciona normalmente.
6. **`FFlightPhysics` sem construtor padrão** quebrava a compilação do `APlanePawn` com `error C2512: no appropriate default constructor available` — o UHT gera um "vtable-helper constructor" pra classes `UCLASS` que exige todo membro C++ comum (não-UPROPERTY) default-construtível. Corrigido com `FFlightPhysics() : Params() {}`.
7. **Git Bash converte caminhos `/Game/...`** em caminhos Windows (`C:\Program Files\Git\Game\...`) ao passar pra executáveis. Ao passar paths de asset da Unreal pela linha de comando no Git Bash, prefixar com `MSYS_NO_PATHCONV=1`.
8. **Docstring Python com caminhos Windows:** `"""... C:\Users ..."""` estoura `SyntaxError: truncated \UXXXXXXXX escape`. Usar raw string (`r"""..."""`).
9. **Diretório da worktree não deletava** ("Device or resource busy") mesmo com o editor fechado: eram processos `EOSOverlayRenderer-Win64-Shipping` órfãos, deixados pra trás pelas sessões do editor. Matar esses processos libera o diretório.

## Comandos de build que funcionam (confirmados nesta máquina)

**Testes standalone da física de voo** (não depende da Unreal, só do MSVC):
```bash
cmd /c '"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && cd /d "C:\Users\cauad\Desktop\dev\jogao" && cl /nologo /EHsc /std:c++17 /Fe:Tools\FlightPhysicsTests\FlightPhysicsTests.exe Tools\FlightPhysicsTests\FlightPhysicsTests.cpp Source\FutebolAviao\Flight\FlightPhysics.cpp && Tools\FlightPhysicsTests\FlightPhysicsTests.exe'
```
Esperado: 6 testes, `All tests passed`.

**Compilar o editor do jogo** (fechar o Unreal Editor antes):
```bash
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" FutebolAviaoEditor Win64 Development -Project="C:\Users\cauad\Desktop\dev\jogao\FutebolAviao.uproject" -WaitMutex
```

## Ajustar a sensação de voo

Todos os números ficam em `FFlightPhysicsParams` (`Source/FutebolAviao/Flight/FlightPhysics.h`), isolados do resto do código:

| Campo | Default |
|---|---|
| `Acceleration` | 2000 |
| `Deceleration` | 1500 |
| `Drag` | 300 |
| `MaxSpeed` | 6000 |
| `MinSpeed` | 0 |
| `PitchRateDegPerSec` | 60 |
| `YawRateDegPerSec` | 90 |
| `RollRateDegPerSec` | 120 |
| `MaxPitchDeg` | 85 |

Mexer aqui é seguro — os testes em `Tools/FlightPhysicsTests/` setam os próprios params, então não quebram quando os defaults mudam.

## Próximo passo

**Fase 2 — combustível + boost.** Ainda não tem plano escrito; a spec já cobre o design (seções "Mecânicas centrais" e "Fases de desenvolvimento" em `docs/superpowers/specs/2026-09-05-futebol-aviao-design.md`). Ao retomar, invocar a skill `writing-plans` pra gerar o plano da Fase 2 antes de codar.
