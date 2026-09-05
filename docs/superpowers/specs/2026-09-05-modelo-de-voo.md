# Modelo de voo — pesquisa e plano de evolução

Documento de referência. Escrito depois do playtest da Fase 2, quando a pergunta virou "como funciona um avião de verdade, e o que disso vale trazer".

Não é um plano de implementação — é o mapa que explica *por que* as próximas mudanças no voo têm a ordem que têm. O plano de cada passo é escrito na hora de fazer.

## O modelo de hoje, descrito honestamente

`FFlightPhysics` é **cinemático, não dinâmico**. Ele não calcula forças; ele integra ângulos e uma velocidade escalar:

```
Speed    += aceleração ou drag, clampado entre MinSpeed e MaxSpeed
PitchDeg += PitchInput * PitchRateDegPerSec   (clampado em ±MaxPitchDeg)
YawDeg   += YawInput   * YawRateDegPerSec     (wrap 360)
RollDeg  += RollInput  * RollRateDegPerSec    (wrap 360)
```

E `APlanePawn` move o ator assim:

```cpp
const FVector Forward = FRotator(PitchDeg, YawDeg, RollDeg).Vector();
AddActorWorldOffset(Forward * Speed * DeltaSeconds, true);
```

Três consequências, todas deliberadas até agora:

1. **A velocidade está soldada ao nariz.** O avião se move exatamente para onde aponta, no mesmo frame. Não existe inércia, deriva, nem atraso.
2. **Roll é decorativo.** `FRotator::Vector()` depende só de pitch e yaw — a componente de roll não entra na conta. `A`/`D` giram o desenho e nada mais.
3. **Não existe gravidade nem sustentação.** Soltar o acelerador desacelera, mas não faz cair. `MinSpeed = 0` (decidido em playtest) significa que o avião pode ficar parado no ar.

Isso é uma escolha arcade defensável, e o playtest da Fase 2 aprovou a sensação. O que segue não é uma condenação do modelo — é o mapa de para onde ele pode crescer.

## Como um avião de verdade funciona

### Curva é inclinação, não leme

Num avião real o leme (yaw) não é o que faz curva. A curva vem de **inclinar**: ao rolar, o vetor de sustentação sai da vertical, e a componente horizontal dele é a força centrípeta que puxa o avião pela curva. A componente vertical continua segurando o peso.

Daí saem três relações, todas simples o bastante para caber no código:

| Grandeza | Fórmula | Leitura |
|---|---|---|
| Fator de carga | `n = 1 / cos(φ)` | 30° de inclinação = 1,15G; 60° = 2G; 80° = 5,76G |
| Raio da curva | `r = v² / (g · tan φ)` | **cresce com o quadrado da velocidade** |
| Taxa de curva | `ω = g · tan(φ) / v` | quanto mais rápido, mais devagar você gira |
| Velocidade de estol | `∝ √n` | curva fechada exige mais velocidade para não estolar |

O fator de carga depende **só** do ângulo de inclinação — não da velocidade, do peso ou da altitude. É por isso que 60° custa 2G em qualquer avião.

### Sustentação e ângulo de ataque

Sustentação depende do **ângulo de ataque** (AoA): o ângulo entre para onde o avião *aponta* e para onde ele *se move*. AoA zero produz sustentação zero; mais AoA produz mais sustentação — até um limite (tipicamente ~15°), além do qual a sustentação despenca e o avião estola.

Repara que AoA só existe se a velocidade **não** estiver soldada ao nariz. No modelo atual o AoA é sempre exatamente zero, por construção.

### O que os próprios devs de simulador recomendam

O princípio declarado por quem escreve modelo de voo para jogos: *"fake as much as possible"* — fórmulas simples com coeficientes ajustados à mão, em vez de simular fluxo de ar. Até simuladores respeitáveis omitem densidade do ar e área de asa, deixando só coeficientes adimensionais para tunar. Simuladores de ponta usam blade element theory (fatiar a asa em elementos e somar), mas isso está muito além do que um jogo de futebol de avião precisa.

## O achado que importa para o game design

A revisão da Fase 2 apontou que **o boost é estritamente dominante**: `BoostAcceleration` (5000) é maior que `Acceleration` (2600), e o único custo é combustível. Não existe trade-off de pilotagem — em qualquer situação com combustível, boostar é melhor.

A aerodinâmica real resolve isso de graça. Como **raio de curva cresce com v²**, ir rápido significa não conseguir fechar curva. Boost passa a ser uma decisão: velocidade em troca de agilidade. Numa disputa de bola, isso é exatamente a tensão que falta — o adversário lento consegue virar dentro da sua curva.

Ou seja: o argumento para trazer aerodinâmica aqui **não é realismo**. É que ela entrega, como consequência natural, o trade-off que o design está pedindo.

## O gargalo estrutural

Nenhuma dessas mecânicas cabe enquanto o estado guardar `Speed` como escalar. Todas dependem de a velocidade ser um **vetor** independente da orientação:

- Inércia = o vetor velocidade atrasa em relação ao nariz
- AoA = ângulo entre o vetor velocidade e o nariz
- Sustentação = força perpendicular ao vetor velocidade
- Curva por inclinação = rotacionar o vetor velocidade pela componente horizontal da sustentação

Por isso o primeiro passo é estrutural e **não muda sensação nenhuma**.

### Passo 1 — vetor velocidade (sem mudança de comportamento)

Trocar `float Speed` por um vetor no `FFlightPhysicsState`. A classe é pura (nenhum header da Unreal), então precisa de um struct POD próprio — algo como `FFlightVector { float X, Y, Z; }`.

Equivalência exata com o modelo atual:

- `speed = comprimento(Velocity)`
- acelerador, drag, boost e clamps agem sobre esse escalar, exatamente como hoje
- `Velocity = Frente(PitchDeg, YawDeg) * speed`, reapontada todo frame

A função `Frente` tem que reproduzir `FRotator::Vector()` da Unreal exatamente, senão a sensação muda:

```
X = cos(pitch) * cos(yaw)
Y = cos(pitch) * sin(yaw)
Z = sin(pitch)
```

O pawn passa a fazer `AddActorWorldOffset(FVector(V.X, V.Y, V.Z) * DeltaSeconds, true)`.

**O teste que prova o passo:** rodar a mesma sequência de inputs pelos dois modelos e verificar que a trajetória bate. Sem esse teste, o passo é um chute.

### Passos seguintes, um por vez, cada um com playtest próprio

Cada um é uma adição pequena depois do Passo 1 — nenhum exige reescrita:

2. **Inércia.** A velocidade passa a girar em direção ao nariz por uma taxa finita, em vez de instantaneamente. Dá peso ao avião e cria deriva na curva. Primeira mudança que altera a sensação de verdade.
3. **Bank-to-turn.** Inclinação passa a gerar curva. `A`/`D` deixam de ser decorativos e viram o controle principal de direção.
4. **Raio por v².** Entrega o trade-off do boost.
5. **Troca energia/altitude.** Subir custa velocidade, descer devolve.

Gravidade e estol ficam de fora dessa lista de propósito: num jogo de futebol de avião, cair do céu por perder sustentação pune sem ensinar. Se um dia entrarem, entram depois do 5.

## Fontes

- [Aerodynamics and Aircraft Performance, cap. 8 — Accelerated Performance: Turns](https://pressbooks.lib.vt.edu/aerodynamics/chapter/chapter-8-accelerated-performance-turns/)
- [Turn Performance — CFI Notebook](https://www.cfinotebook.net/notebook/aerodynamics-and-performance/turn-performance)
- [Load Factors in Steep Turns](http://avstop.com/ac/flighttrainghandbook/loadfactorsinsteepturns.html)
- [Creating a Flight Simulator in Unity3D, Part 1: Flight — VAZGRIZ](https://vazgriz.com/346/flight-simulator-in-unity3d-part-1/)
- [How Flight Simulators Simulate Flight Physics](https://flyawaysimulation.com/news/5002/)
- [What Makes an Aircraft Turn? — King Schools](https://johnandmartha.kingschools.com/2026/08/12/how-an-airplane-turns/)
