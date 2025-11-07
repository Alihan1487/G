#include <SDL2/SDL.h>
#include <vector>

class Sprite{
    public:
    SDL_Rect rect;
    virtual void update(){};
    virtual void evupdate(){};
};

class Scene{
    public:
    Sprite* player;
    std::vector<Sprite*> sprites;
    virtual void update(){};
};

