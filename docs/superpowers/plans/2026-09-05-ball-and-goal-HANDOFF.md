# Fase 3 — Handoff de status

Documento de retomada da Fase 3 (bola e gol). Para ambiente, build e gotchas de máquina, ver `2026-09-05-flight-prototype-HANDOFF.md`; para combustível e boost, `2026-09-05-fuel-boost-HANDOFF.md`.

**Status: código completo e revisado. Falta o playtest (Task 6).**

## Cadeia de branches

```
master ─ fuel-boost (Fase 2, PR aberto)
           └─ flight-inertia (Passo 2 do modelo de voo)
                └─ ball-and-goal (Fase 3)  <- tem tudo
```

`ball-and-goal` está 35 commits à frente da `master`. Para jogar tudo junto, é essa.

## O que existe agora

| Arquivo | Responsabilidade |
|---|---|
| `Flight/ArenaGeometry.h` | **fonte da verdade** das dimensões: arena, boca do gol, teto |
| `Flight/PureMath.h` | vetor `FPureVector` e operações, `ClampValue`, `WrapDegrees` |
| `Flight/BallPhysics.{h,cpp}` | gravidade, arrasto, quique nas 6 faces, buraco na boca do gol, impacto do avião |
| `Flight/MatchRules.{h,cpp}` | detecção de gol e placar |
| `Flight/FlightPhysics.{h,cpp}` | voo, boost, inércia (velocidade atrasa o nariz) |
| `Flight/FuelSystem.{h,cpp}` | tanque, boost, explosão, respawn |
| `Flight/BallActor.{h,cpp}` | adapter da bola |
| `FutebolAviaoGameModeBase.{h,cpp}` | dono do placar, checa gol, reseta a bola |
| `Tools/run-tests.ps1` | roda as 4 suítes; falha alto em suíte órfã |

Todas as classes puras têm **zero header da Unreal**. Os três adapters são o único lugar com tipo da engine. Isso não é estética: a Fase 5 exige client-side prediction, que precisa re-simular determinísticamente a partir de um estado.

**33 testes em 4 suítes**, todos verdes.

## Pendências com gatilho nomeado

- **Faixa morta X ∈ [9850, 10000].** A bola quica em `ArenaHalfX - Raio` e o gol conta em `ArenaHalfX`. Uma bola cuja velocidade em X decai dentro dessa faixa fica estacionada na boca sem marcar, até alguém acertar nela. Precisa de menos de ~52 cm/s ao entrar.
- **Gol cuspido de volta no mesmo frame.** A condição de "pular o quique" é avaliada só na posição de fim de frame. Uma bola que cruza X=10000 mas cujo Y ou Z saiu do envelope até o fim do frame é clampada de volta e nunca vira gol. A ~10000 cm/s e 60fps, um frame são ~167 cm, então exige uma bola quase raspando o poste. **Conserto durável: transformar o cruzamento num teste varrido dentro do `Update`, reportado pela própria classe** — que é o que a Fase 5 vai querer de qualquer forma.
- **Desempate de contato multi-avião é escolha arbitrária.** Com vários aviões sobrepostos, o push-out usa o de menor `GetUniqueID()`. É determinístico (que era o requisito), mas nada valida que seja o *certo* — não há cenário multi-avião no jogo ainda. **Revisitar na Fase 4**, quando houver quatro.
- **Sequência pós-gol mora no adapter.** `FMatchRules` não sabe que existe reset. Delay de saída, comemoração e placar por time na Fase 4 vão se acumular no GameMode se isso não subir pra camada pura antes.
- **`FMatchState` conta por lado da arena, não por time.** Fase 4.
- **Aviões raspam nas paredes com a velocidade intacta.** O sweep para o ator mas não zera `FlightState.Velocity`, então o medidor mostra 6000 com o avião parado contra a parede. E dá pra sair da arena pelas bocas de gol.
- **`PureMath.h` e `ArenaGeometry.h` não têm suíte.** O harness descobre suítes pareando `Tools/<Nome>Tests/` com `Source/Flight/<Nome>.cpp`, e header-only não tem `.cpp`. São testados só transitivamente.
- **Penetração visual.** Nem o C++ nem o script do nível descontam a espessura do cubo (100) nem o raio da bola (150): a bola afunda ~50 em cada parede, senta 1/6 do diâmetro no chão, e clipa pelos postes ao entrar. Consistente e inofensivo — não é bug de física.

## Armadilha de duplicação

As dimensões da arena existem em **dois programas**: `ArenaGeometry.h` e `Tools/LevelBuilder/BuildTestFlightMap.py`. Não dá pra unificar (são linguagens e processos diferentes). Se divergirem, a bola quica no nada ou atravessa parede visível.

**Mudou um, regenerar o mapa:**

```bash
"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\cauad\Desktop\dev\jogao\FutebolAviao.uproject" -run=pythonscript -script="C:\Users\cauad\Desktop\dev\jogao\Tools\LevelBuilder\BuildTestFlightMap.py" -unattended -nosplash -nop4
```

## Duas APIs da Unreal que mentem em modo headless

Descobertas apanhando nesta fase, ambas retornam sucesso sem fazer nada:

1. **`EditorAssetLibrary.delete_asset`** não apaga o arquivo num commandlet `-run=pythonscript`. O script usa `new_blank_map` + `save_map` por causa disso.
2. **`LevelEditorSubsystem.new_level`** recusa asset existente — daí o item anterior.

**Regra que saiu disso: nunca confiar em "script executed successfully".** Verificar o resultado carregando o nível de volta e listando os atores. Foi assim que se confirmou o teto e as 14 posições.

## Rodar os testes

```powershell
powershell -ExecutionPolicy Bypass -File Tools\run-tests.ps1
```

Descobre as 4 suítes, **falha alto se achar teste órfão** (arquivo de teste sem `.cpp` pareado — antes ele pulava em silêncio e saía com 0), e sai com código != 0 em qualquer falha.

## Task 6 — playtest, o que falta

Abrir o editor, Alt+P. Texto ciano = placar, amarelo = combustível e velocidade.

Conferir:
- A linha ciana aparece? Se não, o `Tick` do GameMode não está rodando e nada sobre placar significa nada.
- Bola cai, quica no chão perdendo energia, quica nas paredes e no teto.
- Voar em cima empurra na direção do voo; passar rente dá toque leve.
- Entrar na boca conta ponto e a bola volta pro centro; por cima do travessão ou por fora do poste **não** conta.
- O avião não fica carregando a bola grudada.

Tabela de tuning:

| Se... | Ajustar |
|---|---|
| bola pesada/morta | `Restitution` (0.75), `Drag` (0.35) em `FBallPhysicsParams` |
| difícil acertar a bola | `GetCollisionRadius` (300) em `PlanePawn.h` |
| todo chute sai no talo | **`MaxSpeed` (12000) antes de `HitTransfer`** — um acerto de frente já produz ~10100, então o teto mascara o `HitTransfer` |
| bola cai rápido demais | `Gravity` (980) |
| gol fácil/difícil demais | `GoalHalfWidthY` (1500), `GoalHeightZ` (2000) em `ArenaGeometry.h` — **regenerar o mapa depois** |
| avião pesado/escorregadio demais | `VelocityAlignPerSec` (6) em `FlightPhysics.h` |

## Próximo passo

**Fase 4 — 2v2 local.** Sem plano escrito. Antes dela, decidir: migrar pra Enhanced Input (split-screen com quatro jogadores é onde mapeamento legado incomoda) e revisitar o desempate de contato multi-avião.
