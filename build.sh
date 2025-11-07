em++ main.cpp -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS=["png"] \
 --embed-file frames -s ALLOW_MEMORY_GROWTH=1 -lidbfs.js -o index.html -s EXPORTED_FUNCTIONS=_main,_load,_save
