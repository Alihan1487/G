em++ main.cpp \
-s USE_SDL=2 \
-s USE_SDL_IMAGE=2 \
-s USE_SDL_MIXER=2 \
-s USE_SDL_TTF=2 \
-s SDL2_IMAGE_FORMATS='["png"]' \
-s SDL2_MIXER_FORMATS='["ogg"]' \
--embed-file assets@assets \
-s ALLOW_MEMORY_GROWTH=1 \
-s EXPORTED_FUNCTIONS='["_main","_load","_save"]' \
-lidbfs.js \
-o index.html &&
emrun index.html --no-browser