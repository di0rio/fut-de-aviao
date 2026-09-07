r"""Cria (ou recria) o nivel de teste da Fase 1: Content/Maps/TestFlightMap.umap.

Rodar headless (o Unreal Editor precisa estar FECHADO):

    "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" ^
        "<caminho>\FutebolAviao.uproject" ^
        -run=pythonscript -script="<caminho>\Tools\LevelBuilder\BuildTestFlightMap.py" ^
        -unattended -nosplash -nop4

O nivel e criado sem World Partition de proposito: assim vira um unico .umap
versionavel, em vez de uma arvore de __ExternalActors__.
"""

import unreal

LEVEL_PATH = "/Game/Maps/TestFlightMap"
CUBE_MESH = "/Engine/BasicShapes/Cube.Cube"

# ATENCAO: estas constantes de arena/gol sao uma copia manual dos defaults de
# FArenaGeometry (Source/FutebolAviao/Flight/ArenaGeometry.h). Esse arquivo e
# a fonte de verdade -- este script Python e um programa separado que a
# engine nao consegue incluir, entao a duplicacao aqui nao tem como ser
# eliminada. Se voce mudar um, mude o outro TAMBEM e rode este script de novo
# pra regenerar Content/Maps/TestFlightMap.umap, ou as paredes da arena e a
# boca do gol no nivel salvo ficam fora de sincronia com a fisica pura.
ARENA_HALF_X = 20000.0
ARENA_HALF_Y = 12000.0
ARENA_CEILING_Z = 12000.0
GOAL_HALF_WIDTH_Y = 3000.0
GOAL_HEIGHT_Z = 4000.0

# Profundidade da rede atras de cada gol (decoracao de nivel, sem equivalente
# em C++ -- MatchRules.cpp so olha a boca do gol via matematica pura, entao
# GOAL_DEPTH nao entra em check_matches_cpp()). ~40m: da pra ler como gol e
# ainda sobra espaco pro aviao desacelerar antes do fundo.
GOAL_DEPTH = 4000.0

# Espessura real (em unidades de mundo) de uma "parede fina" feita com
# spawn_box usando escala 1.0 no eixo perpendicular -- o cubo do Engine tem
# 100 de lado. Usado para deslocar os paineis da rede pra FORA da boca do gol
# em vez de centra-los em cima do limite (que e o que Parede_Norte/Sul e
# Fundo_*/Travessao_* fazem, e que deixaria metade do painel invadindo o vao).
THIN_WALL_WORLD_THICKNESS = 100.0

# Altura dos PlayerStarts: o aviao nasce ja no ar, sem gravidade (ver FFlightPhysics).
PLAYER_START_Z = 1500.0

# Dois spawns fixos, um de cada lado, virados um pro outro -- e assim que o 2v2
# da Fase 4 vai funcionar. Com um jogador so, o segundo fica sobrando de
# proposito: e o que prova que o respawn respeita o heading em que foi colocado
# (APlanePawn semeia o FlightState a partir do transform do spawn).
#
# Derivado de ARENA_HALF_X (nao um literal solto): um literal separado escapa
# de check_matches_cpp() e nao acompanha a arena se ela encolher -- foi
# exatamente assim que os PlayerStarts foram parar dentro da parede de fundo
# numa revisao anterior. Com 4000 de folga, o aviao nasce bem antes da linha
# de gol em qualquer escala testada ate agora.
PLAYER_START_X = ARENA_HALF_X - 4000.0

# Chao gigante so pra dar referencia visual de velocidade durante o playtest --
# precisa cobrir os 400m x 240m da arena nova (o cubo da engine tem 100 de lado).
FLOOR_SCALE = unreal.Vector(ARENA_HALF_X * 2 / 100.0, ARENA_HALF_Y * 2 / 100.0, 1.0)

import os
import re

def check_matches_cpp():
    """Falha alto se as constantes daqui divergirem de ArenaGeometry.h.

    A duplicacao nao tem como ser eliminada (Python e C++ sao programas
    separados), entao o proximo melhor e detecta-la na hora em vez de
    descobrir jogando que a bola quica no nada.
    """
    header = os.path.join(os.path.dirname(__file__), "..", "..",
                          "Source", "FutebolAviao", "Flight", "ArenaGeometry.h")
    with open(header, "r") as f:
        text = f.read()

    expected = {
        "ArenaHalfX": ARENA_HALF_X,
        "ArenaHalfY": ARENA_HALF_Y,
        "ArenaCeilingZ": ARENA_CEILING_Z,
        "GoalHalfWidthY": GOAL_HALF_WIDTH_Y,
        "GoalHeightZ": GOAL_HEIGHT_Z,
    }

    for name, value in expected.items():
        match = re.search(r"float\s+%s\s*=\s*([0-9.]+)f" % name, text)
        if not match:
            raise RuntimeError("Nao achei %s em ArenaGeometry.h" % name)
        found = float(match.group(1))
        if abs(found - value) > 0.001:
            raise RuntimeError(
                "%s divergente: Python tem %.1f, ArenaGeometry.h tem %.1f. "
                "Alinhe os dois antes de regenerar o mapa." % (name, value, found))


def spawn_box(actors, label, location, scale):
    """Cubo estatico usado como parede. O cubo do Engine tem 100 de lado."""
    box = spawn(actors, unreal.StaticMeshActor, location)
    box.set_actor_label(label)
    box.set_actor_scale3d(scale)
    box.static_mesh_component.set_static_mesh(unreal.EditorAssetLibrary.load_asset(CUBE_MESH))
    return box


def spawn(actor_subsystem, actor_class, location, rotation=None):
    return actor_subsystem.spawn_actor_from_class(
        actor_class,
        location,
        rotation if rotation is not None else unreal.Rotator(0.0, 0.0, 0.0),
    )


def build():
    check_matches_cpp()

    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    # LevelEditorSubsystem.new_level recusa escrever num path que ja tem asset
    # -- e falha mesmo logo depois de EditorAssetLibrary.delete_asset, porque
    # nesta chamada headless (-run=pythonscript) o delete_asset devolve True
    # sem de fato remover o pacote/arquivo (confirmado apagando e conferindo o
    # disco: o .umap continua la). EditorLoadingAndSavingUtils.new_blank_map
    # cria o mundo novo em memoria sem tocar em nenhum asset existente, e
    # save_map grava (sobrescrevendo) diretamente no path de destino -- isso
    # sim funciona de forma idempotente aqui.
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)

    floor = spawn(actors, unreal.StaticMeshActor, unreal.Vector(0.0, 0.0, 0.0))
    floor.set_actor_label("Floor")
    floor.set_actor_scale3d(FLOOR_SCALE)
    floor.static_mesh_component.set_static_mesh(unreal.EditorAssetLibrary.load_asset(CUBE_MESH))

    sun = spawn(actors, unreal.DirectionalLight, unreal.Vector(0.0, 0.0, 1000.0),
                unreal.Rotator(pitch=-46.0, yaw=0.0, roll=0.0))
    sun.set_actor_label("DirectionalLight")

    sky_light = spawn(actors, unreal.SkyLight, unreal.Vector(0.0, 0.0, 1000.0))
    sky_light.set_actor_label("SkyLight")

    sky = spawn(actors, unreal.SkyAtmosphere, unreal.Vector(0.0, 0.0, 0.0))
    sky.set_actor_label("SkyAtmosphere")

    # Lado oeste, nariz apontando pro leste.
    west = spawn(actors, unreal.PlayerStart, unreal.Vector(-PLAYER_START_X, 0.0, PLAYER_START_Z),
                 unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0))
    west.set_actor_label("PlayerStart_Oeste")

    # Lado leste, nariz apontando pro oeste.
    east = spawn(actors, unreal.PlayerStart, unreal.Vector(PLAYER_START_X, 0.0, PLAYER_START_Z),
                 unreal.Rotator(pitch=0.0, yaw=180.0, roll=0.0))
    east.set_actor_label("PlayerStart_Leste")

    # Parede de contencao: exatamente em +-ARENA_HALF_Y, do chao ao teto. E ela
    # que impede o aviao de sair voando da arena -- o aviao colide com malha de
    # verdade (AddActorWorldOffset com sweep), diferente da bola, que e limitada
    # so por matematica em BallPhysics.cpp. Parede baixa demais = aviao escapa
    # por cima.
    for side, sign in (("Norte", 1.0), ("Sul", -1.0)):
        spawn_box(actors, "Parede_" + side,
                  unreal.Vector(0.0, sign * ARENA_HALF_Y, ARENA_CEILING_Z / 2.0),
                  unreal.Vector(ARENA_HALF_X * 2 / 100.0, 1.0, ARENA_CEILING_Z / 100.0))

    # Arquibancadas: tres degraus, ATRAS da parede de contencao (|Y| maior que
    # ARENA_HALF_Y), nunca em cima dela. Puramente visuais -- o limite fisico
    # continua sendo o plano reto em +-ARENA_HALF_Y.
    for side, sign in (("Norte", 1.0), ("Sul", -1.0)):
        for tier in range(3):
            tier_height = 3000.0 + tier * 3000.0
            tier_offset = 1000.0 + tier * 2000.0
            spawn_box(actors, "Arquibancada_%s_%d" % (side, tier),
                      unreal.Vector(0.0,
                                    sign * (ARENA_HALF_Y + tier_offset),
                                    tier_height / 2.0),
                      unreal.Vector(ARENA_HALF_X * 2 / 100.0, 8.0, tier_height / 100.0))

    # Fundos: laterais da boca do gol, do chao ao teto.
    for side, sign in (("Oeste", -1.0), ("Leste", 1.0)):
        side_width = ARENA_HALF_Y - GOAL_HALF_WIDTH_Y
        for edge, edge_sign in (("A", 1.0), ("B", -1.0)):
            center_y = edge_sign * (GOAL_HALF_WIDTH_Y + side_width / 2.0)
            spawn_box(actors, "Fundo_%s_%s" % (side, edge),
                      unreal.Vector(sign * ARENA_HALF_X, center_y, ARENA_CEILING_Z / 2.0),
                      unreal.Vector(1.0, side_width / 100.0, ARENA_CEILING_Z / 100.0))

        # Travessao: fecha do topo da boca ate o teto.
        beam_height = ARENA_CEILING_Z - GOAL_HEIGHT_Z
        spawn_box(actors, "Travessao_" + side,
                  unreal.Vector(sign * ARENA_HALF_X, 0.0, GOAL_HEIGHT_Z + beam_height / 2.0),
                  unreal.Vector(1.0, GOAL_HALF_WIDTH_Y * 2 / 100.0, beam_height / 100.0))

    # Rede do gol: caixa fechada atras de cada boca, aberta so para o campo.
    # Achado da revisao: Marco_Leste bloqueava a boca do gol Leste e o gol
    # Oeste nao tinha nada -- um aviao que entrava pelo gol saia voando pra
    # sempre (o GameModeBase so reresseta a bola no gol, nunca o aviao). A
    # bola continua marcando gol por matematica pura (MatchRules.cpp nao olha
    # malha nenhuma); a rede so para o aviao.
    #
    # Cada painel e deslocado para FORA do limite que fecha (metade da
    # espessura de THIN_WALL_WORLD_THICKNESS), ao contrario de
    # Parede_Norte/Sul e Fundo_*/Travessao_*, que centralizam em cima do
    # limite -- aqui isso invadiria a boca do gol ou baixaria o teto da rede
    # abaixo de GOAL_HEIGHT_Z, exatamente os dois jeitos de a rede quebrar a
    # propria funcao.
    half_thin_wall = THIN_WALL_WORLD_THICKNESS / 2.0
    for side, sign in (("Oeste", -1.0), ("Leste", 1.0)):
        depth_center_x = sign * (ARENA_HALF_X + GOAL_DEPTH / 2.0)
        back_x = sign * (ARENA_HALF_X + GOAL_DEPTH)

        # Fundo da rede: fecha a caixa por tras, do chao ate a altura do gol.
        # A face interna fica em back_x - sinal*half_thin_wall, sempre alem de
        # ARENA_HALF_X (GOAL_DEPTH e muito maior que a espessura da parede).
        spawn_box(actors, "Rede_Fundo_" + side,
                  unreal.Vector(back_x, 0.0, GOAL_HEIGHT_Z / 2.0),
                  unreal.Vector(1.0, GOAL_HALF_WIDTH_Y * 2 / 100.0, GOAL_HEIGHT_Z / 100.0))

        # Paredes laterais: ligam a linha do gol (ARENA_HALF_X) ao fundo da
        # rede. Centralizadas em GOAL_HALF_WIDTH_Y + half_thin_wall, entao a
        # face interna cai exatamente em GOAL_HALF_WIDTH_Y -- nunca menos,
        # nunca invadindo o vao por onde a bola/aviao entram.
        for edge, edge_sign in (("A", 1.0), ("B", -1.0)):
            spawn_box(actors, "Rede_Lateral_%s_%s" % (side, edge),
                      unreal.Vector(depth_center_x,
                                    edge_sign * (GOAL_HALF_WIDTH_Y + half_thin_wall),
                                    GOAL_HEIGHT_Z / 2.0),
                      unreal.Vector(GOAL_DEPTH / 100.0, 1.0, GOAL_HEIGHT_Z / 100.0))

        # Teto da rede: fecha por cima cobrindo toda a profundidade GOAL_DEPTH.
        # Centralizado em GOAL_HEIGHT_Z + half_thin_wall, entao a face de baixo
        # cai exatamente em GOAL_HEIGHT_Z -- nunca desce abaixo disso e reduz
        # o vao livre da boca.
        spawn_box(actors, "Rede_Teto_" + side,
                  unreal.Vector(depth_center_x, 0.0, GOAL_HEIGHT_Z + half_thin_wall),
                  unreal.Vector(GOAL_DEPTH / 100.0, GOAL_HALF_WIDTH_Y * 2 / 100.0, 1.0))

        # Chao da rede: a caixa da rede nao tinha piso -- o chao geral da
        # arena (FLOOR_SCALE) para exatamente na linha de gol (X = +-
        # ARENA_HALF_X), entao os 4000 de profundidade da caixa (X entre
        # ARENA_HALF_X e back_x) ficavam sem nada embaixo. Um aviao entrando
        # na boca numa picada rasa atravessava o vao e caia pra fora do
        # nivel, sem que o GameMode resetasse nada (ele so reresseta a
        # bola). Mesma convencao do Rede_Teto, espelhada: centralizado em
        # -half_thin_wall, entao a face de CIMA cai exatamente em Z=0 --
        # nao fica mais baixa que o chao da arena nem sobe pra dentro do
        # vao.
        spawn_box(actors, "Rede_Chao_" + side,
                  unreal.Vector(depth_center_x, 0.0, -half_thin_wall),
                  unreal.Vector(GOAL_DEPTH / 100.0, GOAL_HALF_WIDTH_Y * 2 / 100.0, 1.0))

    MARK_Z = 70.0
    MARK_THICKNESS = 0.4   # em unidades de cubo (100), ou seja 40 unidades

    # Linha de meio-campo, cruzando a largura.
    spawn_box(actors, "Marca_MeioCampo",
              unreal.Vector(0.0, 0.0, MARK_Z),
              unreal.Vector(MARK_THICKNESS, ARENA_HALF_Y * 2 / 100.0, 0.2))

    # Circulo central aproximado por 16 blocos, raio de 4000.
    import math
    for i in range(16):
        angle = (2.0 * math.pi * i) / 16.0
        spawn_box(actors, "Marca_Circulo_%d" % i,
                  unreal.Vector(math.cos(angle) * 4000.0, math.sin(angle) * 4000.0, MARK_Z),
                  unreal.Vector(6.0, MARK_THICKNESS, 0.2))

    # Grandes areas: retangulo aberto na frente de cada gol.
    AREA_DEPTH = 6000.0
    AREA_HALF_WIDTH = GOAL_HALF_WIDTH_Y + 3000.0
    for side, sign in (("Oeste", -1.0), ("Leste", 1.0)):
        # Linha paralela a linha de fundo.
        spawn_box(actors, "Marca_Area_%s_Frente" % side,
                  unreal.Vector(sign * (ARENA_HALF_X - AREA_DEPTH), 0.0, MARK_Z),
                  unreal.Vector(MARK_THICKNESS, AREA_HALF_WIDTH * 2 / 100.0, 0.2))
        # Duas linhas perpendiculares fechando a area.
        for edge, edge_sign in (("A", 1.0), ("B", -1.0)):
            spawn_box(actors, "Marca_Area_%s_%s" % (side, edge),
                      unreal.Vector(sign * (ARENA_HALF_X - AREA_DEPTH / 2.0),
                                    edge_sign * AREA_HALF_WIDTH, MARK_Z),
                      unreal.Vector(AREA_DEPTH / 100.0, MARK_THICKNESS, 0.2))

    # Torres nos quatro cantos: referencia de orientacao a distancia.
    #
    # Achado da revisao: ficavam em |Y| = ARENA_HALF_Y exatamente, com escala
    # 12 no eixo Y -- ou seja, 600 unidades de meia-escala invadindo PARA
    # DENTRO da arena (ate Y = 11400). A parede de contencao (Parede_Norte/
    # Sul) e so 100 de espessura, entao o aviao colidia com a malha da torre
    # (que tem colisao de verdade), enquanto a bola -- limitada so por
    # matematica pura em BallPhysics.cpp, que conhece apenas o plano em
    # ARENA_HALF_Y -- quicava exatamente ali e atravessava por baixo da
    # malha sem nada a impedir. Essa assimetria era sentida nos quatro
    # cantos. Empurradas por sua propria meia-escala em Y, a face interna da
    # torre cai sobre a parede em vez de invadir o espaco de jogo.
    TOWER_SCALE = unreal.Vector(12.0, 12.0, 180.0)
    TOWER_CLEARANCE_Y = TOWER_SCALE.y * 50.0   # meia-escala do cubo, em unidades de mundo
    for x_side, x_sign in (("O", -1.0), ("L", 1.0)):
        for y_side, y_sign in (("N", 1.0), ("S", -1.0)):
            spawn_box(actors, "Torre_%s%s" % (x_side, y_side),
                      unreal.Vector(x_sign * ARENA_HALF_X,
                                    y_sign * (ARENA_HALF_Y + TOWER_CLEARANCE_Y),
                                    9000.0),
                      TOWER_SCALE)

    # Marcos de identidade: dizem de relance pra que lado voce esta voando.
    #
    # Achado da revisao: a arena e uma caixa fechada de seis faces opacas
    # (Parede_Norte/Sul, Fundo_*/Travessao_*, Teto) -- entao qualquer coisa
    # do lado de FORA dela (as arquibancadas, e os marcos antigos em
    # ARENA_HALF_X + GOAL_DEPTH + 4000) fica atras de paredes opacas e nunca
    # aparece durante o jogo, so no editor olhando o nivel de fora. Isso
    # matava a unica pista de orientacao "pra que lado eu estou indo" que a
    # Fase 3 tentou dar.
    #
    # Fix: presos do lado de DENTRO do casco, na face interna da parede de
    # fundo de cada ponta, com uma protrusao rasa (MARKER_DEPTH) onde o
    # aviao raramente voa -- nao comem espaco de jogo de verdade. Formas
    # deliberadamente diferentes por ponta (sem cor, que exigiria instancia
    # de material):
    #   Oeste = dois pilares altos e finos, flanqueando a boca do gol, fora
    #           de |Y| < GOAL_HALF_WIDTH_Y para nunca bloquear um chute.
    #   Leste = uma faixa larga e baixa, acima do travessao (Z a partir de
    #           GOAL_HEIGHT_Z) para nunca fechar a boca do gol por cima.
    MARKER_DEPTH = 600.0   # protrusao para dentro da arena, a partir da parede de fundo

    WEST_PILLAR_Y = GOAL_HALF_WIDTH_Y + 1000.0     # fora da boca, com folga
    WEST_PILLAR_WIDTH_Y = 400.0
    WEST_PILLAR_HEIGHT = ARENA_CEILING_Z - 2000.0  # alcanca quase todo o teto
    for edge_sign in (1.0, -1.0):
        spawn_box(actors, "Marco_Oeste_%d" % int(edge_sign),
                  unreal.Vector(-ARENA_HALF_X + MARKER_DEPTH / 2.0,
                                edge_sign * WEST_PILLAR_Y,
                                WEST_PILLAR_HEIGHT / 2.0),
                  unreal.Vector(MARKER_DEPTH / 100.0, WEST_PILLAR_WIDTH_Y / 100.0, WEST_PILLAR_HEIGHT / 100.0))

    EAST_BAND_HEIGHT = 2000.0
    EAST_BAND_HALF_WIDTH_Y = ARENA_HALF_Y - 500.0  # quase toda a largura da parede
    spawn_box(actors, "Marco_Leste",
              unreal.Vector(ARENA_HALF_X - MARKER_DEPTH / 2.0,
                            0.0,
                            GOAL_HEIGHT_Z + EAST_BAND_HEIGHT / 2.0),
              unreal.Vector(MARKER_DEPTH / 100.0, EAST_BAND_HALF_WIDTH_Y * 2 / 100.0, EAST_BAND_HEIGHT / 100.0))

    # Teto: fecha a arena por cima em Z = ArenaCeilingZ, cobrindo toda a
    # planta da arena (mesmo padrao das paredes: cubo de 100 escalado pra
    # cobrir o vao inteiro). Sem isto FBallPhysics::Update ainda quicava a
    # bola em ArenaCeilingZ e o aviao voava reto pra fora dali, porque nao
    # havia nenhuma geometria no nivel bloqueando aquela altura -- so o
    # numero existia, na fisica pura.
    spawn_box(actors, "Teto",
              unreal.Vector(0.0, 0.0, ARENA_CEILING_Z),
              unreal.Vector(ARENA_HALF_X * 2 / 100.0, ARENA_HALF_Y * 2 / 100.0, 1.0))

    if not unreal.EditorLoadingAndSavingUtils.save_map(world, LEVEL_PATH):
        raise RuntimeError("save_map falhou para {}".format(LEVEL_PATH))

    unreal.log("TestFlightMap criado com {} atores".format(len(actors.get_all_level_actors())))


build()
