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
    int fps;
    float acc=0.f;
    public:
    Vid(std::string foldername,SDL_Renderer* ren,int f=10){
        fps=f;
        name=foldername;
        int count=1;
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
    Vid(){};
    ~Vid(){
        std::cout<<"DESTROYED VID OBJECT"<<std::endl;
        for (auto i:txts)
            SDL_DestroyTexture(i);
    }

    SDL_Texture* Get(){
        if (curr>=txts.size())
            return nullptr;
        auto frame=txts[curr];
        acc+=dt;
        float frt=1.f/fps;
        while (acc>=frt){
            acc-=frt;
            curr++;
        }
        return frame;
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
    int GetCursor(){
        return curr;
    }
};
