# Escala, sensação e estádio — Design

Spec nascida do playtest da Fase 3. O jogo funciona ponta a ponta, mas quatro coisas incomodaram: o avião escorrega, está rápido demais para o campo, a bola foge do toque, e o mapa não parece o jogo que ele é.

Três dessas quatro têm a mesma causa e um conserto só.

## O diagnóstico

Os números do jogo nunca foram definidos em relação uns aos outros. Cada fase escolheu os seus isoladamente, e o resultado é uma escala incoerente:

| | Hoje | Em unidades reais |
|---|---|---|
| Arena | 20000 × 12000 × 5000 | 200m × 120m × 50m |
| Avião cruzeiro | 6000 | 60 m/s |
| Avião boost | 9000 | 90 m/s |
| Teto da bola | 12000 | **120 m/s** |

Disso saem, mecanicamente, três das quatro queixas:

- **A bola foge porque o teto dela é o dobro da velocidade do avião.** Um acerto de frente em cruzeiro produz `6000 × 1,6 + 500 = 10100` — 101 m/s contra os seus 60 (90 boostando). É impossível perseguir o próprio chute. Não é a bola ser leve; é ela ser mais rápida que o avião por construção.
- **A velocidade não cabe no campo:** atravessar os 200m leva 3,3 segundos.
- **Controlar é difícil porque o campo é apertado**, não porque o avião responda mal: o raio de curva a 60 m/s é de ~31m numa arena de 120m de largura. Cabem duas curvas na largura inteira.

A quarta queixa — o avião escorregar — é separada, e é responsabilidade do Passo 2 do modelo de voo (inércia), introduzido depois do último playtest aprovado.

## A regra que amarra os números

> **A bola nunca é mais rápida que o avião no boost, e é mais rápida que o avião em cruzeiro.**

Essa é a decisão de design; os valores abaixo são a sua consequência. Se algum número mudar no tuning, é essa relação que tem que continuar valendo.

O efeito colateral é o que justifica a regra: **o boost ganha uma função.** Hoje ele é estritamente dominante — acelera mais que o acelerador normal e o único custo é combustível, sem nenhum trade-off de pilotagem (apontado na revisão da Fase 2). Com a bola a 60 m/s e o cruzeiro a 45, é preciso boostar para alcançar uma bola bem batida. O boost deixa de significar "ande mais rápido" e passa a significar "dispute a bola".

## Números novos

### Voo — `FFlightPhysicsParams`

| Campo | Hoje | Novo | Por quê |
|---|---|---|---|
| `MaxSpeed` | 6000 | **4500** | 45 m/s; atravessar o campo novo leva 8,9s |
| `BoostMaxSpeed` | 9000 | **6500** | 65 m/s; acima do teto da bola, para poder alcançá-la |
| `Acceleration` | 2600 | **1950** | mantém o tempo até a máxima que o playtest aprovou (~2,3s) |
| `Deceleration` | 2200 | **1650** | idem, proporcional |
| `Drag` | 700 | **525** | idem, proporcional |
| `BoostAcceleration` | 5000 | **3750** | idem, proporcional |
| `VelocityAlignPerSec` | 6 | **25** | atraso de ~4° em vez de ~13°; a derrapagem some sem sumir o peso |

As taxas de rotação (`PitchRateDegPerSec` 110, `YawRateDegPerSec` 110, `RollRateDegPerSec` 200) **não mudam**. Com o avião mais lento, o raio de curva cai sozinho de ~31m para ~23m — mais ágil sem tocar no controle.

### Bola — `FBallPhysicsParams`

| Campo | Hoje | Novo | Por quê |
|---|---|---|---|
| `MaxSpeed` | 12000 | **6000** | 60 m/s: abaixo do boost (65), acima do cruzeiro (45) |
| `HitTransfer` | 1.6 | **1.1** | acerto em cruzeiro produz 5250, abaixo do teto; só chute boostado satura |
| `MinKick` | 500 | **300** | o toque de raspão continua existindo, sem catapultar |
| `Drag` | 0.35 | **0.20** | num campo o dobro do tamanho, a bola precisa carregar mais longe |
| `Radius` | 150 | **400** | 8m de diâmetro: visível a 400m de distância |
| `Gravity` | 980 | 980 | sem mudança |
| `Restitution` | 0.75 | 0.75 | sem mudança |

### Arena — `FArenaGeometry`

| Campo | Hoje | Novo | Em metros |
|---|---|---|---|
| `ArenaHalfX` | 10000 | **20000** | 400m de comprimento |
| `ArenaHalfY` | 6000 | **12000** | 240m de largura |
| `ArenaCeilingZ` | 5000 | **12000** | 120m de altura |
| `GoalHalfWidthY` | 1500 | **3000** | boca de 60m |
| `GoalHeightZ` | 2000 | **4000** | 40m de altura |

### Escala visual

Um avião de 2m e uma bola de 3m somem num campo de 400m. Crescem junto:

| | Hoje | Novo |
|---|---|---|
| Escala do cone do avião | (2, 1, 1) | **(12, 6, 6)** — ~12m |
| Raio de colisão do avião | 300 | **600** |
| Braço da câmera (`TargetArmLength`) | 800 | **2500** |
| `PlayerStart` em X | ±9000 | ±16000 |

A escala da malha da bola continua derivando de `Radius / 50`, como já faz.

## O estádio

Estádio-hangar coberto, em escala de avião. A escolha é por **legibilidade**: a 160 km/h dentro de uma caixa vazia de 400m você não sabe para onde está voando. O campo precisa responder três perguntas de relance — onde é o gol, onde acaba o campo, e para que lado eu estou virado.

Tudo é construído com cubos e cilindros da engine pelo gerador de nível em Python, como já é hoje. Não há arte nova.

**Elementos:**

1. **Piso** cobrindo os 400m × 240m.
2. **Teto** em `ArenaCeilingZ` — hoje ele existe na física e não existe no nível, o que faz a bola quicar no nada.
3. **Paredes de fundo** nas duas pontas, com o buraco da boca do gol, como já são.
4. **Arquibancadas inclinadas** nas laterais longas, no lugar das paredes retas.

   **A inclinação é puramente visual.** A física da bola quica numa caixa alinhada aos eixos (`BounceAxis` em `BallPhysics.cpp`), e continua quicando no plano reto em `±ArenaHalfY`. Fazer a física acompanhar a rampa exigiria colisão com plano inclinado, o que está fora do escopo desta spec. As arquibancadas ficam *atrás* do limite físico, não em cima dele — quem implementar não deve tentar casar as duas coisas.
5. **Marcações no chão** — linha de meio-campo, círculo central, e as duas grandes áreas. Blocos finos e baixos, apenas para dar referência de posição e velocidade.
6. **Traves** visíveis nas duas bocas, como já são.
7. **Torres nos quatro cantos**, para ancorar a orientação.
8. **Pontas com silhuetas diferentes** — a ponta Oeste ganha duas torres altas e finas atrás do gol; a Leste, um bloco largo e baixo. Isso é o que responde "para que lado estou virado" sem depender de cor.

Cor fica fora do escopo: exigiria instâncias de material, e silhueta resolve o problema de orientação sozinha.

## Tuning ao vivo — console variables

O tuning até agora exigia editar um header e recompilar o editor. Com o jogo em ajuste fino, isso é o gargalo.

Cada parâmetro ganha uma **console variable**, ajustável com `~` **durante o Play**, sem rebuild e sem reiniciar o PIE:

```
fa.Plane.MaxSpeed 4000
fa.Ball.HitTransfer 0.8
fa.Plane.VelocityAlign 1000
```

**Convenção que evita duplicar os defaults:** cada CVar nasce em `-1` (não definida). O adapter lê as CVars a cada tick e sobrescreve **apenas** os campos cujo valor seja `>= 0`. Os headers puros continuam sendo a fonte única da verdade dos defaults.

Isso não é preciosismo: duplicar dimensões entre dois lugares foi exatamente o que produziu o bug do gol inalcançável na Fase 3, onde `ArenaHalfX` existia em dois headers e a boca do gol em apenas um.

As CVars moram nos adapters (`PlanePawn.cpp`, `BallActor.cpp`), então **as classes puras continuam sem nenhum header da Unreal** — a fronteira que sustenta as suítes standalone e a prediction da Fase 5 fica intacta.

| Prefixo | Campos |
|---|---|
| `fa.Plane.` | `MaxSpeed`, `BoostMaxSpeed`, `Acceleration`, `BoostAcceleration`, `Drag`, `TurnRate`, `VelocityAlign`, `CollisionRadius` |
| `fa.Fuel.` | `TankCapacity`, `BoostDrain`, `Regen`, `RespawnSeconds` |
| `fa.Ball.` | `MaxSpeed`, `Gravity`, `Drag`, `Restitution`, `HitTransfer`, `MinKick`, `Radius` |

`fa.Plane.TurnRate` ajusta pitch e yaw juntos — eles são mantidos iguais de propósito, para mirar em 3D ser simétrico.

`fa.Plane.VelocityAlign 1000` desliga a inércia por completo, devolvendo a sensação da Fase 2.

`fa.Ball.Radius` reescala a malha junto, senão o visual e a colisão divergem.

## O que não muda

- As classes puras seguem sem header da Unreal. As CVars são camada de adapter.
- Nenhuma regra nova. Isto é reescala e tuning, não mecânica.
- Taxas de rotação, gravidade da bola e restituição ficam como estão.
- Sem times, sem 2v2, sem pickups, sem rede. Continuam sendo Fases 4 e 5.

## Riscos conhecidos

- **A duplicação C++/Python das dimensões da arena piora com o estádio**, que passa a ter mais elementos derivados das mesmas medidas. O gerador já avisa em comentário; com o crescimento, vale ele **ler `ArenaGeometry.h` e falhar alto** se os números divergirem, em vez de só avisar.
- **A faixa morta entre o quique e a linha de gol cresce junto com o raio da bola** — de 150 para 400 unidades. Uma bola lenta parando na boca sem marcar fica mais provável, não menos. O conserto durável continua sendo o teste varrido dentro do `Update`, registrado no handoff da Fase 3.
- **Os números são chute informado.** A relação entre eles é a decisão; os valores são ponto de partida para o playtest. É por isso que as CVars vêm junto e não depois.
