set(MOSAIC_APP_NAME holo_partner)
set(MOSAIC_APP_MODULE_SOURCE holo_partner_app.c)
set(MOSAIC_APP_EXTRA_SOURCES holo_partner_runtime.c)
set(MOSAIC_APP_MODULE_SYMBOL mosaic_holo_partner_app)
set(MOSAIC_APP_BUNDLE generated/holo_partner.gspb)
set(MOSAIC_APP_SCENE_DIR scene)
set(MOSAIC_APP_SCENE_JSON scene/holo_partner_480.json)
set(MOSAIC_APP_GENERATOR scene/gen_scene.py)
set(MOSAIC_APP_SCENE_SOURCES
    scene/holo_partner_layout.py
    scene/assets/fatfish_idle.png)
set(MOSAIC_APP_GENERATED_HEADERS
    holo_partner_actions.h
    holo_partner_objects.h)
set(MOSAIC_APP_LOGIC NATIVE)
