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

# Altura dos PlayerStarts: o aviao nasce ja no ar, sem gravidade (ver FFlightPhysics).
PLAYER_START_Z = 500.0

# Dois spawns fixos, um de cada lado, virados um pro outro -- e assim que o 2v2
# da Fase 4 vai funcionar. Com um jogador so, o segundo fica sobrando de
# proposito: e o que prova que o respawn respeita o heading em que foi colocado
# (APlanePawn semeia o FlightState a partir do transform do spawn).
PLAYER_START_X = 9000.0

# Chao gigante so pra dar referencia visual de velocidade durante o playtest.
FLOOR_SCALE = unreal.Vector(400.0, 400.0, 1.0)


def spawn(actor_subsystem, actor_class, location, rotation=None):
    return actor_subsystem.spawn_actor_from_class(
        actor_class,
        location,
        rotation if rotation is not None else unreal.Rotator(0.0, 0.0, 0.0),
    )


def build():
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    # new_level recusa sobrescrever, entao apagar antes e o que torna o script
    # realmente idempotente -- rodar de novo recria o nivel do zero.
    if unreal.EditorAssetLibrary.does_asset_exist(LEVEL_PATH):
        unreal.EditorAssetLibrary.delete_asset(LEVEL_PATH)

    if not level_editor.new_level(LEVEL_PATH, False):
        raise RuntimeError("new_level falhou para {}".format(LEVEL_PATH))

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

    if not level_editor.save_current_level():
        raise RuntimeError("save_current_level falhou")

    unreal.log("TestFlightMap criado com {} atores".format(len(actors.get_all_level_actors())))


build()
