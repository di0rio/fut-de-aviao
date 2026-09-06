# Fase 2 — Handoff de status

Documento de retomada da Fase 2 (combustível + boost). Para ambiente, build e gotchas de máquina, ver `2026-09-05-flight-prototype-HANDOFF.md` — tudo lá continua valendo.

**Status: código completo, playtestado e aprovado.**

Referências:
- Plano: `docs/superpowers/plans/2026-09-05-fuel-boost.md`
- Spec do jogo: `docs/superpowers/specs/2026-09-05-futebol-aviao-design.md`
- Pesquisa de modelo de voo: `docs/superpowers/specs/2026-09-05-modelo-de-voo.md`

## O que existe agora

| Arquivo | Responsabilidade |
|---|---|
| `Source/FutebolAviao/Flight/FuelSystem.{h,cpp}` | tanque, dreno no boost, regen passiva com delay, explosão, respawn. Puro, sem header da Unreal |
| `Source/FutebolAviao/Flight/FlightPhysics.{h,cpp}` | velocidade (vetor), ângulos, boost. Puro |
| `Source/FutebolAviao/Flight/PlanePawn.{h,cpp}` | único ponto onde os dois se juntam e onde aparece tipo da engine |
| `Tools/FuelSystemTests/`, `Tools/FlightPhysicsTests/` | 19 testes standalone (10 + 9) |
| `Tools/LevelBuilder/BuildTestFlightMap.py` | gera o mapa de teste. Idempotente |

Controles: `W`/`S` acelerador, mouse pitch/yaw, `A`/`D` roll (decorativo), `LeftShift` ou gatilho direito boost.

## Decisões tomadas em playtest (não reabrir sem motivo)

- **`MinSpeed = 0`.** Tentou-se 1400 ("avião nunca para no ar"); reprovado, porque o avião saía voando sozinho ao apertar Play. Se voltar, tem que vir junto com o avião nascendo em movimento de propósito.
- **Boost mantido como está.** O `bBoostActive` é o primeiro do if/else em `FFlightPhysics::Update`, então segurar o freio durante o boost não faz nada e boost com acelerador zerado acelera. Isso contradiz a spec, que lista airbrake como controle central — foi mantido mesmo assim porque o playtest aprovou a sensação. Travado por `Test_BoostOverridesBrakeInput`; se mudar, é lá que se registra.
- **Dois `PlayerStart` fixos**, em lados opostos, virados um pro outro. Com um jogador a engine escolhe entre eles, então dá pra nascer de qualquer lado — é o que exercita o respawn respeitando o heading.

## O que está corrigido mas NÃO protegido

**O loop de morte.** Segurar boost através do respawn re-explodia a cada ~5s, para sempre. A causa era `bBoostInput` persistir: ele é edge-driven por `IE_Pressed`/`IE_Released`, e quem *segura* a tecla nunca gera um `Released`. Corrigido com `bBoostInput = false` no bloco de respawn de `APlanePawn::Tick`.

**Apagar essa linha hoje não deixa nenhum teste vermelho.** As suítes são standalone e não conseguem instanciar um `AActor`. O teste `Test_ResolveBoostDoesNotReengageAfterRespawnWhenInputCleared` verifica a aritmética do `ResolveBoost`, e seus comentários dizem isso explicitamente — ele não finge cobrir o fix do pawn.

Cobrir de verdade exige um Automation test da Unreal tickando um `APlanePawn` real. Faz sentido montar isso quando a Fase 4 trouxer mais lógica de nível de ator para testar junto.

## Pendências registradas (nenhuma bloqueia a Fase 3)

- `ClampValue` está duplicado entre `FuelSystem.cpp` e `FlightPhysics.cpp` — consequência da regra "sem dependência da engine". A Fase 3 traz um terceiro sistema puro (bola), que é o gatilho combinado para extrair um `PureMath.h` com `ClampValue` e `WrapDegrees`.
- `FFlightPhysicsParams` e `FFuelParams` não são `UPROPERTY`, então cada iteração de tuning exige rebuild do editor. Expor como `EditDefaultsOnly` no pawn pagaria por si na próxima rodada de ajuste.
- Não há harness commitado para rodar as suítes — cada execução é um `cl.exe` na mão, copiado de um doc de plano. Um `.bat` em `Tools/` resolveria antes que apareça uma terceira suíte.
- O projeto declara Enhanced Input em `DefaultInput.ini` mas o código usa bindings legados via shim de compatibilidade. Funciona, mas a Fase 4 é split-screen local com quatro jogadores, que é exatamente onde mapeamento legado por jogador incomoda. Decidir antes da Fase 4, não durante.
- `SetActorTransform` no respawn usa flags de sweep padrão. Inofensivo enquanto o movimento é `AddActorWorldOffset` manual; vira problema se o avião ganhar corpo físico.
- Sem gravidade e sem colisão real com o chão além do sweep. O avião pode ficar preso no chão se você mergulhar — sair dá, puxando o nariz.

## Modelo de voo: onde paramos

O **Passo 1** do plano em `docs/superpowers/specs/2026-09-05-modelo-de-voo.md` está feito: `FFlightPhysicsState` guarda um vetor velocidade em vez de um escalar. **Não mudou sensação nenhuma** — provado por `Test_VelocityVectorMatchesLegacyNoseDirectionModel`, que reimplementa o modelo antigo longhand e compara trajetórias.

Isso destrava os passos 2 a 5 (inércia, bank-to-turn, raio por v², troca energia/altitude), cada um uma adição pequena com playtest próprio. Nenhum deles foi feito, e nenhum é pré-requisito da Fase 3.

O ponto que justifica encarar isso algum dia: hoje **o boost é estritamente dominante** — acelera mais que o acelerador normal e o único custo é combustível. Raio de curva crescendo com v² é o que transformaria boost numa decisão de verdade.

## Rodar os testes

```bash
cmd /c '"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && cd /d "C:\Users\cauad\Desktop\dev\jogao" && cl /nologo /EHsc /std:c++17 /Fe:Tools\FuelSystemTests\FuelSystemTests.exe Tools\FuelSystemTests\FuelSystemTests.cpp Source\FutebolAviao\Flight\FuelSystem.cpp && Tools\FuelSystemTests\FuelSystemTests.exe'
```

Trocar `FuelSystem` por `FlightPhysics` para a outra suíte. Esperado: 10 e 9 testes, `All tests passed` nas duas.

## Próximo passo

**Fase 3 — bola e gol.** Sem plano escrito ainda; a spec cobre o design. Ao retomar, invocar `writing-plans`.

O padrão que funcionou nas Fases 1 e 2 e vale repetir: regra em classe pura testável sem a engine, ator fino como adapter, e uma task final que é playtest humano.
