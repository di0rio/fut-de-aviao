# Futebol de Avião — Design

Data: 2026-09-05

## Visão geral

Jogo de futebol aéreo 2v2, estilo Rocket League com aviões no lugar de carros. Times pilotam aviões em um campo 3D fechado (paredes e teto fazem parte da jogabilidade), tentando empurrar uma bola para o gol adversário. O diferencial central é o **combustível limitado**: cada avião tem um tanque que só é consumido ao acelerar/dar boost — voo de cruzeiro é "de graça". Gerenciar o boost é a decisão tática principal: usar agora para alcançar a bola, ou guardar para não ficar sem combustível.

## Mecânicas centrais

- **Pilotagem:** movimento livre em 3D — aceleração, curvas, subir/descer, freio ("airbrake") para manobras.
- **Boost:** consome combustível, usado para arrancadas rápidas e disputas de bola.
- **Combustível zerado:** o avião **explode e respawna** em um ponto seguro do campo — penalidade forte, jogador fica fora da jogada por alguns segundos.
- **Reabastecimento:** dois sistemas coexistindo:
  - Regeneração passiva contínua (o tanque enche sozinho aos poucos com o tempo).
  - Pickups de combustível espalhados pelo campo em posições randomizadas, quantidade escalando com o número de jogadores (nem escasso, nem excessivo) — bônus extra além da regeneração.
- **Bola e gol:** física de colisão entre avião e bola; bola entra no gol adversário = ponto.

## Formato de partida

- 2v2 (duas duplas).
- Campo único fechado no protótipo (múltiplos mapas ficam fora do escopo por agora).
- Vitória por gols ao fim do tempo de partida.

## Controles (protótipo inicial)

Teclado + mouse ou gamepad: aceleração, guinada/inclinação para direção, botão de boost, botão de freio/airbrake. Sensibilidade fina fica para depois de validar a física básica.

## Arquitetura técnica

- **Engine:** Unreal Engine (última versão estável 5.x), C++ para lógica central (física, replicação, sistema de combustível) + Blueprints para tuning de gameplay e UI.
- **Rede:** replicação nativa do Unreal (Actors replicados: avião, bola, placar). **Servidor autoritativo** desde o início — o cliente só envia inputs (acelerar, virar, boost); o servidor decide posição real, consumo de combustível e gols. Isso evita a maioria dos cheats óbvios (velocidade/combustível infinito).
- **Validação de input:** o servidor valida que os comandos recebidos são fisicamente possíveis (limites de aceleração/ângulo) antes de aplicar.
- **Ferramentas necessárias:**
  - Unreal Engine 5.x (Epic Games Launcher ou build via source).
  - Conta Epic Games (gratuita).
  - MSVC Build Tools + Windows SDK instalados (necessários para compilar o C++ do Unreal no Windows, independente do editor de texto usado).
  - Cursor como editor de código (baseado em VS Code — a Unreal gera arquivos de projeto compatíveis). Cursor substitui o *editor*, não o compilador.
  - Opcional, recomendado: gamepad para testar a pilotagem.

## Fases de desenvolvimento

Cada fase só começa depois que a anterior estiver jogável e "parecer divertida" — evita gastar semanas em rede antes de saber se pilotar o avião é legal.

1. **Protótipo de voo:** um avião controlável, física de voo (aceleração, curvas, altitude), sem bola nem combustível ainda.
2. **Combustível + boost:** tanque, consumo no boost, regeneração passiva, explosão/respawn ao zerar.
3. **Bola e gol:** física da bola, colisão com aviões, detecção de gol, placar.
4. **2v2 local:** 4 avatares controláveis localmente (ou com bots), pickups de combustível no campo, partida completa jogável numa máquina só (Listen Server local).
5. **Online real:** servidor dedicado, matchmaking simples (código de sala/IP direto — sem ranking ainda), testes em rede de verdade (máquinas diferentes).

## Otimização

- Replicar só o essencial (posição/rotação da bola e aviões, quantidade de combustível), aproveitando as ferramentas nativas de replicação do Unreal (relevância/frequência ajustável por ator).
- Client-side prediction para o avião do próprio jogador, para a pilotagem parecer responsiva mesmo com latência.
- Usar o Chaos Physics de forma econômica — evitar colisões complexas desnecessárias na malha dos aviões/bola; arena geometricamente simples no protótipo.
- Otimizações mais pesadas (culling agressivo, LOD) só entram se o perfil de performance mostrar necessidade, não preventivamente.

## Segurança

- Servidor autoritativo (ver Arquitetura técnica) cobre a maioria dos cheats óbvios.
- Validação de input no servidor antes de aplicar qualquer comando do cliente.
- Anti-cheat de mercado (ex.: Easy Anti-Cheat) fica fora do escopo do protótipo — a autoridade do servidor cobre o essencial desta fase.

## Testes

- O teste principal é jogar: cada fase termina com uma sessão de playtest antes de avançar para a próxima.
- Testes automatizados fazem sentido só para lógica isolada (cálculo de consumo/regeneração de combustível, detecção de gol) — não para validar se a física é divertida, o que só se valida jogando.

## Fora do escopo (por agora)

- Sistema de ranking/temporada, progressão, cosméticos.
- Replay/espectador.
- Múltiplos mapas.
- Formatos além de 2v2 (3v3, 4v4, etc.).
- Anti-cheat avançado além da autoridade do servidor.
