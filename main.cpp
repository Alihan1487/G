#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <emscripten.h>
#include <vector>
#include <iostream>

SDL_Renderer* rend;
SDL_Window* win;

class Vid{
    std::string name;
    std::vector<SDL_Texture*> txts;
    SDL_Renderer* r;
    int curr=0;
    public:
    Vid(std::string foldername,SDL_Renderer* ren){
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
    SDL_Texture* Get(){
        if (curr>=txts.size())
            return nullptr;
        auto frame=txts[curr];
        curr++;
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
};

Vid v;

void loop(){
    SDL_Event ev;
    while (SDL_PollEvent(&ev)){
        if (ev.type==SDL_QUIT)
            emscripten_cancel_main_loop();
    }

    SDL_SetRenderDrawColor(rend,0,0,0,255);
    SDL_RenderClear(rend);
    {
        SDL_Rect d{0,0,300,300};
        SDL_RenderCopy(rend,v.Get(),nullptr,&d);
    }
    SDL_RenderPresent(rend);
}

int main(){
    SDL_Init(SDL_INIT_EVERYTHING);
    IMG_Init(IMG_INIT_PNG);
    win=SDL_CreateWindow("hah",0,0,1000,800,SDL_WINDOW_SHOWN);
    rend=SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED);
    v=Vid("frames",rend);
    std::cout<<"loaded:"<<v.size();
    emscripten_set_main_loop(loop,0,1);
    return 0;
}