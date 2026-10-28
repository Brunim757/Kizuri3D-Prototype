add_library(KizuriGame STATIC
  src/Animator.cpp
  src/GameLoop.cpp
  src/GameWorld.cpp
  src/GameApp.cpp
  src/GameMaterial.cpp
  src/HardwareSpec.cpp
)
target_include_directories(KizuriGame PUBLIC include)
target_link_libraries(KizuriGame PUBLIC KizuriCore KizuriRender KizuriPhysics KizuriAnimation KizuriAudio KizuriScene KizuriAssets d3d11 dxgi)
