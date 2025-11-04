#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <emscripten.h>
#include <vector>

class Vid{
    std::string name;
    std::vector<SDL_Texture*> txts;
    SDL_Renderer* r;
    int curr=0;
    public:
    Vid(std::string foldername,SDL_Renderer* ren){
        name=foldername;
        int count=0;
        r=ren;
        while (true){
            SDL_Surface* surf;
            surf=IMG_Load((name+"/frame"+std::to_string(count)+".png").c_str());
            if (!surf){
                break;
            }
            SDL_Texture* t=SDL_CreateTextureFromSurface(r,surf);
            if (!t)
                break;
            txts.push_back(t);
            count+=1;
            SDL_FreeSurface(surf);
        }
    }
    SDL_Texture* Get(){
        if (curr>=txts.size())
            return nullptr;
        curr++;
        return txts[curr];
    }
    SDL_Texture* Get(int index){
        if (!(index>=txts.size() || index<0))
            return txts[index];
        else
            return nullptr;
    }
    void setCursor(int where){
        if (where<0 || where>=txts.size())
            return;
        curr=where;
    }
    int size(){
        return txts.size();
    }
};