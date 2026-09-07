# Modelos de avião — Design

O avião é um cone. Serviu para as Fases 1 a 3, mas com a arena reescalada para 400m e a bola em 8m, o que está no ar é um cone de 12m: não dá para saber para onde ele aponta, nem de quem ele é.

Esta spec troca o cone por três modelos montados com primitivas da engine, dimensionados **em proporção à bola**, e reorganiza a colisão para que o visual e a física parem de ser a mesma coisa.

## A regra que amarra o tamanho

> **O avião mede 1,5 diâmetro de bola em comprimento e 1,5 em envergadura. O raio de colisão é metade disso.**

Com a bola de hoje (`Radius = 400`, logo `D = 800`), isso dá 12m × 12m e raio 600 — exatamente o `DefaultCollisionRadius` que já está no código. Os números atuais não mudam; ganham uma razão de ser.

A esfera **aproxima** o avião, não o circunscreve. Ponta de nariz, ponta de asa e canto de cauda escapam dela em alguns pontos por até uns 15% — é geometricamente inevitável: se a traseira toca a esfera no eixo, qualquer leme com altura naquele ponto fica para fora. A esfera é generosa no miolo e apertada nas quinas, e acertar a bola com a ponta da asa continua sendo aproximado. Isso já é verdade hoje com o cone e o raio 600; a spec mantém a mesma troca, agora com um número que tem origem.

Nada disso é medido em centímetros. As peças são descritas em **diâmetros de bola**, e o adapter multiplica na fronteira. Mexer em `fa.Ball.Radius` durante o Play reescala os três aviões junto — é isso que faz "proporcional à bola" ser uma propriedade verificável do dado, em vez de um comentário que envelhece.

## `PlaneModel` — os modelos como dado puro

Classe pura em `Source/FutebolAviao/Flight/PlaneModel.h` + `.cpp`, sem nenhum header da Unreal, no padrão de `FlightPhysics` e `BallPhysics`.

O `.cpp` não é opcional: `Tools/run-tests.ps1` só descobre uma suíte `Tools/<Nome>Tests/` se existir `Source/FutebolAviao/Flight/<Nome>.cpp` para parear, e reporta órfã (derrubando o exit code) se não existir. Um `PlaneModel` só-header, no estilo de `ArenaGeometry.h`, ficaria sem teste.

```cpp
enum class EPlaneShape { Cube, Cylinder, Cone, Sphere };
enum class EPlaneTint  { Team, Dark, Light };
enum class EPlaneModel { Delta, Warbird, Arcade };

struct FPlanePart
{
    EPlaneShape Shape;
    PureMath::FPureVector Offset;   // centro da peca, em diametros de bola
    PureMath::FPureVector Size;     // tamanho total da peca, em diametros de bola
    float PitchDeg, YawDeg, RollDeg;
    EPlaneTint Tint;
    bool bSpins;                    // helice: gira no eixo X local
};

struct FPlaneCollisionSphere
{
    PureMath::FPureVector Offset;   // em diametros de bola
    float Radius;                   // em diametros de bola
};
```

Sem STL e sem alocação, como o resto do código puro: as consultas devolvem um struct com array de tamanho fixo mais uma contagem, por valor.

```cpp
struct FPlaneParts     { FPlanePart Parts[16]; int Count = 0; };
struct FPlaneCollision { FPlaneCollisionSphere Spheres[4]; int Count = 0; };

namespace FPlaneModel
{
    FPlaneParts     GetParts(EPlaneModel Model);
    FPlaneCollision GetCollision(EPlaneModel Model);
}
```

Eixos locais: **X para o nariz, Y para a asa direita, Z para cima** — a mesma convenção que `FFlightPhysicsState` já usa. Origem no centro do avião.

### As três tabelas

Os offsets abaixo são o ponto de partida. O que é **normativo** são as asserções da suíte de testes (comprimento, envergadura, folga na esfera); os valores individuais são livres para se ajustarem no playtest desde que continuem satisfazendo aquelas.

**`Size` é sempre o tamanho final da peça nos eixos do avião**: X é comprimento, Y é largura, Z é altura. Uma fuselagem `Size = (1.10, 0.42, 0.42)` mede 1,10 diâmetro de bola do nariz à cauda, e pronto.

Cilindro e cone de `/Engine/BasicShapes` nascem com o eixo em Z, e deitá-los na direção do nariz é **problema do adapter**, não do autor da tabela: ele aplica esse conserto de eixo internamente, a partir do `Shape`. Os campos `PitchDeg/YawDeg/RollDeg` ficam livres para rotação de **design** — hoje, só o enflechamento da asa do delta.

Conflacionar as duas coisas na mesma coluna era um convite a erro: com o conserto de eixo escrito na tabela, o `Size` de um cilindro passaria a ser lido no frame do mesh, e `(1.10, 0.42, 0.42)` deixaria de significar 1,10 de comprimento. Separadas, o sinal do `FRotator` vira detalhe de uma função só, testável de uma vez, em vez de um campo que cada linha da tabela pode errar.

**C — Arcade gordinho (padrão)**

| Peça | Forma | Offset (X, Y, Z) | Size (X, Y, Z) | Tint | Gira |
|---|---|---|---|---|---|
| Fuselagem | Cylinder | (0, 0, 0) | (1.10, 0.42, 0.42) | Team | |
| Nariz | Cone | (0.62, 0, 0) | (0.20, 0.40, 0.40) | Dark | |
| Cabine | Sphere | (0.10, 0, 0.22) | (0.34, 0.30, 0.24) | Light | |
| Asa | Cube | (0.02, 0, 0.04) | (0.38, 1.50, 0.07) | Team | |
| Estabilizador | Cube | (−0.58, 0, 0.06) | (0.22, 0.60, 0.06) | Team | |
| Leme | Cube | (−0.60, 0, 0.26) | (0.24, 0.06, 0.36) | Team | |
| Hélice | Cube | (0.69, 0, 0) | (0.03, 0.06, 0.56) | Dark | sim |
| Spinner | Sphere | (0.70, 0, 0) | (0.10, 0.10, 0.10) | Light | |

**B — Monomotor a hélice**

| Peça | Forma | Offset (X, Y, Z) | Size (X, Y, Z) | Tint | Gira |
|---|---|---|---|---|---|
| Fuselagem | Cylinder | (0, 0, 0) | (1.15, 0.32, 0.32) | Team | |
| Capô | Cylinder | (0.62, 0, 0) | (0.20, 0.34, 0.34) | Team | |
| Hélice | Cube | (0.70, 0, 0) | (0.03, 0.06, 0.48) | Dark | sim |
| Spinner | Cone | (0.71, 0, 0) | (0.06, 0.12, 0.12) | Light | |
| Cabine | Sphere | (0.05, 0, 0.18) | (0.32, 0.24, 0.18) | Light | |
| Asa | Cube | (0.05, 0, −0.02) | (0.34, 1.50, 0.05) | Team | |
| Estabilizador | Cube | (−0.55, 0, 0.02) | (0.20, 0.52, 0.04) | Team | |
| Leme | Cube | (−0.60, 0, 0.22) | (0.26, 0.05, 0.32) | Team | |

**A — Caça delta**

| Peça | Forma | Offset (X, Y, Z) | Size (X, Y, Z) | Yaw | Tint | Gira |
|---|---|---|---|---|---|---|
| Fuselagem | Cylinder | (0, 0, 0) | (1.30, 0.30, 0.30) | 0 | Team | |
| Nariz | Cone | (0.68, 0, 0) | (0.16, 0.28, 0.28) | 0 | Dark | |
| Cabine | Sphere | (0.28, 0, 0.16) | (0.30, 0.22, 0.18) | 0 | Light | |
| Asa esquerda | Cube | (−0.10, −0.33, 0) | (0.50, 0.70, 0.05) | −18 | Team | |
| Asa direita | Cube | (−0.10, 0.33, 0) | (0.50, 0.70, 0.05) | 18 | Team | |
| Leme | Cube | (−0.55, 0, 0.26) | (0.30, 0.05, 0.38) | 0 | Team | |
| Bocal | Cylinder | (−0.68, 0, 0) | (0.12, 0.26, 0.26) | 0 | Dark | |

O delta é o único com rotação de design: as asas em Yaw ±18°. Rotação enviesa a caixa — a envergadura efetiva de uma peça girada não é o seu `Size.Y`. Os testes de comprimento e envergadura medem a **caixa envolvente depois da rotação**, não o campo cru, senão o delta passa medindo a coisa errada.

## Como o pawn monta

O `MeshComponent` (o cone) sai de `APlanePawn`. No lugar:

- **Root: `USphereComponent`**, o único componente com colisão. Raio vindo de `FPlaneModel::GetCollision`, convertido para centímetros. É ele que bate nas paredes, no teto e no chão.
- **Peças visuais: `UStaticMeshComponent`** criados em runtime (`NewObject` + `RegisterComponent`), anexados ao root, todos com `SetCollisionEnabled(ECollisionEnabled::NoCollision)`.

A separação é o ponto: **o visual não participa de nenhuma física.** Trocar de modelo, engordar uma asa ou acrescentar uma peça nunca muda como o avião bate em nada.

Conversão na fronteira, o único lugar que conhece centímetros:

```
D     = RaioDaBolaAtual * 2
Escala = Size * D / 100     // todas as primitivas de /Engine/BasicShapes tem 100 de lado
Local  = Offset * D
```

`RaioDaBolaAtual` sai de `fa.Ball.Radius` quando ela está definida (`>= 0`), com fallback em `FBallPhysicsParams().Radius`. Não é preciosismo: a CVar já reescala a malha da bola ao vivo, e sem ler a mesma fonte o avião mantém o tamanho antigo e a proporção quebra no meio do playtest. Ler a CVar aqui evita duplicar o default — a mesma convenção que `Tuning::Apply` já estabeleceu.

`DefaultCollisionRadius` deixa de ser o literal `600` e passa a ser derivado do modelo. `fa.Plane.CollisionRadius` continua sobrescrevendo por cima, sem mudança.

### Trocar de modelo sem recompilar

CVar `fa.Plane.Model`: `0` = Delta, `1` = Warbird, `2` = Arcade. **Default `2`.**

Ela é `int32` e não segue a convenção do `-1` das outras — aqui `-1` não significa nada, o default é um modelo de verdade. `Tuning::Apply` continua servindo só para os `float`.

`ApplyTuningCVars` compara o modelo e o `D` atuais com os do frame anterior; se qualquer um mudou, destrói as peças e remonta. Isso é o loop de iteração: trocar de avião e reescalar durante o PIE, sem rebuild.

Peças com `bSpins` acumulam rotação em torno do X local a cada Tick. A hélice girando é o que dá sensação de motor num avião parado no ar.

## Cor de time

`UMaterialInstanceDynamic` sobre `/Engine/BasicShapes/BasicShapeMaterial`, escrevendo no VectorParameter `Color` (verificado por inspeção do `.uasset`; se a criação falhar, a peça fica com o material padrão — degrada para cinza, não quebra).

Público em `APlanePawn`:

```cpp
void SetTeamColor(FLinearColor NewColor);
```

É o gancho para a Fase 4 escolher a cor por time. Para testar hoje, `fa.Plane.Team`: `0` = azul (Oeste), `1` = vermelho (Leste).

**Só as peças `Tint::Team` recebem a cor.** `Dark` (nariz, hélice, bocal) e `Light` (cabine, spinner) são fixas. Pintar o avião inteiro de uma cor chapada apaga a silhueta a 200m — exatamente o que a cor deveria estar ajudando a ler.

## Colisão: o que entra agora e o que fica preparado

Duas camadas separadas de propósito.

**Contra o mundo** — a `USphereComponent`. Sem mudança de comportamento: `AddActorWorldOffset` continua com sweep, e o avião continua deslizando ao raspar na parede. Bater e quicar, ou bater e explodir, ficam de fora desta spec.

**Contra a bola** — `GetCollision` devolve uma **lista** de esferas em espaço local. **Hoje cada um dos três modelos devolve exatamente uma**, centrada na origem, raio `0.75 D`. `GetCollisionRadius()` passa a ler dessa esfera única, `ABallActor` não muda uma linha, e o comportamento é idêntico ao atual.

O que a spec entrega é a assinatura plural e a esfera como entidade de dado própria. Dar à asa uma esfera separada da fuselagem vira acrescentar duas linhas na tabela do modelo e um `for` no `ABallActor`. Fazer o avião quicar na parede vira usar um raio que já existe como dado em vez de um literal. Nenhuma dessas duas mudanças mexe em arquitetura — que é o único objetivo desta seção.

## Testes — `Tools/PlaneModelTests/`

Suíte standalone nova, no padrão das outras (compila `PlaneModelTests.cpp` + `PlaneModel.cpp` com o `cl.exe` do Build Tools, sem a engine). Para cada um dos três modelos:

1. Comprimento total (caixa envolvente em X, já rotacionada) = `1.5 D`, tolerância 3%.
2. Envergadura total (caixa envolvente em Y, já rotacionada) = `1.5 D`, tolerância 3%.
3. Nenhum canto de peça passa de **1,15 × o raio de colisão**. Não é "cabe dentro": como a seção da regra explica, quina de cauda e ponta de asa escapam da esfera por construção, e exigir encaixe perfeito reprovaria qualquer avião com leme. O que este teste pega é o caso que importa — uma asa desenhada muito maior que a colisão, atravessando a bola sem tocá-la.
4. Pelo menos uma esfera de colisão, com raio maior que zero.
5. Pelo menos uma peça `Tint::Team` — sem isso o time não aparece.
6. `GetParts`/`GetCollision` respeitam a capacidade dos arrays fixos (`Count` dentro do limite).

As asserções são expressas em diâmetros de bola, então valem em qualquer escala — é assim que a proporção com a bola vira uma invariante travada por teste, e não uma coincidência que dura até o próximo tuning.

## O que não muda

- Física de voo, física da bola, regras de gol. Nenhum número de gameplay.
- As classes puras seguem sem nenhum header da Unreal; CVars e materiais vivem no adapter.
- Nenhum asset novo, nenhum arquivo binário no git.
- Sem times de verdade, sem 2v2, sem HUD. Continuam sendo as Fases 4 e 5.

## Riscos conhecidos

- **Draw calls por peça.** Oito peças por avião, quatro aviões, sem instancing: 32 componentes. Irrelevante nesta escala, mas é o custo de montar modelo com primitivas em vez de uma malha só. Se um dia pesar, o conserto é gerar um `UStaticMesh` de verdade a partir da mesma tabela — o dado já está separado do desenho.
- **A tabela de peças é um chute informado.** As proporções entre peças saem de olho, não de playtest. É por isso que a troca de modelo é CVar e não recompilação: a spec assume que os números vão mudar depois da primeira captura.
- **O sinal das rotações de `FRotator`** para deitar cilindro e cone só se confirma vendo. Se vier invertido na primeira captura, é ajuste de sinal, não de design.
- **A esfera de colisão não é o avião.** Ela sobra no miolo (o jogador acerta a bola com o ar entre a asa e a fuselagem) e falta nas quinas (a ponta do leme fica de fora). Isso já é verdade hoje com o cone e o raio 600; a spec não piora nem conserta. A lista de esferas é o caminho para consertar quando incomodar — e a esfera única de hoje é o caso degenerado dessa lista, não um obstáculo a ela.
