#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <emscripten.h>
#include <vector>
#include <iostream>
#include <fstream>

SDL_Renderer* rend;
SDL_Window* win;
float dt=0;

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

Scene* currloop;

void loop(){
    currloop->update();
}

class Main;
class Second;

Main* m;
Second* s;

class Main : public Scene{
    int start=0;
    int finish=0;
    std::vector<Sprite*> walls;
    class Player : public Sprite{
        Main* m;
        public:
        Player(Main* s){
            rect={0,0,100,100};
            m=s;
        }
        void update() override{
            const Uint8* kstate=SDL_GetKeyboardState(NULL);
            if (kstate[SDL_SCANCODE_W]){
                rect.y-=400*dt;
                for (auto& i: m->walls){
                    if (SDL_HasIntersection(&rect,&i->rect)){
                        auto crect=i->rect;
                        rect.y=crect.y+crect.h;
                    }
                }
            }
            if (kstate[SDL_SCANCODE_S]){
                rect.y+=400*dt;
                for (auto& i: m->walls){
                    if (SDL_HasIntersection(&rect,&i->rect)){
                        auto crect=i->rect;
                        rect.y=crect.y-rect.h;
                    }
                }
            }
            if (kstate[SDL_SCANCODE_A]){
                rect.x-=400*dt;
                for (auto& i: m->walls){
                    if (SDL_HasIntersection(&rect,&i->rect)){
                        auto crect=i->rect;
                        rect.x=crect.x+crect.w;
                    }
                }
            }
            if (kstate[SDL_SCANCODE_D]){
                rect.x+=400*dt;
                for (auto& i: m->walls){
                    if (SDL_HasIntersection(&rect,&i->rect)){
                        auto crect=i->rect;
                        rect.x=crect.x-rect.w;
                    }
                }
            }
            SDL_SetRenderDrawColor(rend,255,0,0,255);
            SDL_RenderFillRect(rend,&rect);
        }
    };
    class Wall : public Sprite{
        public:
        Wall(SDL_Rect r){
            rect=r;
        }
        void update() override{
            SDL_SetRenderDrawColor(rend,0,255,0,255);
            SDL_RenderFillRect(rend,&rect);
        }
    };
    public:
    Main(){
        player=new Player(this);
        walls.push_back(new Wall({300,300,200,200}));
    }
    void update() override{
        SDL_Event e;
        start=SDL_GetTicks();
        dt=(start-finish)/1000.f;
        finish=start;
        while (SDL_PollEvent(&e)){
            if (e.type==SDL_QUIT)
                emscripten_cancel_main_loop();
            if (e.type==SDL_KEYDOWN)
                if (e.key.keysym.sym==SDLK_e)
                    currloop=(Scene*)s;
        if (e.type==SDL_KEYDOWN)
                if (e.key.keysym.sym==SDLK_l){
                    EM_ASM(
                        _save();
                        FS.syncfs(false,function (err) {
                        if (err){
                            console.log("Error at syncing:",err)
                        }
                        else{
                            console.log("saved");
                        }
                    }));
                }
        };

        SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
        SDL_RenderClear(rend);

        player->update();
        for (auto& i:walls)
            i->update();
        SDL_RenderPresent(rend);
    }
};

class Second : public Scene{
    std::vector<Sprite*> walls;
    int start=0;
    int finish=0;
    class Player : public Sprite{
        Second* m;
        public:
        Player(Second* s){
            rect={0,0,100,100};
            m=s;
        }
        void update() override{
            const Uint8* kstate=SDL_GetKeyboardState(NULL);
            if (kstate[SDL_SCANCODE_W]){
                rect.y-=400*dt;
                for (auto& i: m->walls){
                    if (SDL_HasIntersection(&rect,&i->rect)){
                        auto crect=i->rect;
                        rect.y=crect.y+crect.h;
                    }
                }
            }
            if (kstate[SDL_SCANCODE_S]){
                rect.y+=400*dt;
                for (auto& i: m->walls){
                    if (SDL_HasIntersection(&rect,&i->rect)){
                        auto crect=i->rect;
                        rect.y=crect.y-rect.h;
                    }
                }
            }
            if (kstate[SDL_SCANCODE_A]){
                rect.x-=400*dt;
                for (auto& i: m->walls){
                    if (SDL_HasIntersection(&rect,&i->rect)){
                        auto crect=i->rect;
                        rect.x=crect.x+crect.w;
                    }
                }
            }
            if (kstate[SDL_SCANCODE_D]){
                rect.x+=400*dt;
                for (auto& i: m->walls){
                    if (SDL_HasIntersection(&rect,&i->rect)){
                        auto crect=i->rect;
                        rect.x=crect.x-rect.w;
                    }
                }
            }
            SDL_SetRenderDrawColor(rend,255,0,0,255);
            SDL_RenderFillRect(rend,&rect);
        }
    };
    class Wall : public Sprite{
        public:
        Wall(SDL_Rect r){
            rect=r;
        }
        void update() override{
            SDL_SetRenderDrawColor(rend,0,255,0,255);
            SDL_RenderFillRect(rend,&rect);
        }
    };
    public:
    Second(){
        player=new Player(this);
        walls.push_back(new Wall({500,600,150,300}));
    }
    void update() override{
        SDL_Event e;
        start=SDL_GetTicks();
        dt=(start-finish)/1000.f;
        finish=start;
        while (SDL_PollEvent(&e)){
            if (e.type==SDL_QUIT)
                emscripten_cancel_main_loop();
            if (e.type==SDL_KEYDOWN)
                if (e.key.keysym.sym==SDLK_q)
                    currloop=(Scene*)m;
        };

        SDL_SetRenderDrawColor(rend, 0, 100, 255, 255);
        SDL_RenderClear(rend);

        player->update();
        for (auto& i:walls)
            i->update();
        SDL_RenderPresent(rend);
    }
};

extern "C" void load(){
    std::ofstream f("save/soo.txt", std::ios::app);
    std::ifstream ff("/save/soo.txt");
    if (ff.is_open()){
        std::string g;
        ff>>g;
        std::cout<<"\nDATA IN FILE:"<<g<<std::endl;
    }
    else 
        std::cout<<"ERROR DURING OPENING s.txt"<<std::endl;
    ff.close();
}

extern "C" void save(){
    std::ofstream f("/save/soo.txt");
    if (f.is_open()){
    f<<"hello";
    f.close();
    }
    else
        std::cout<<"ERROR DURING WRITING"<<std::endl;
}

int main(){
    m=new Main;
    s=new Second;

    EM_ASM(
        FS.mkdir("/save");
        FS.mount(IDBFS,{},"/save");
        FS.syncfs(true,function (err){
            if (err){
                console.log("Error at maountin or smth");
            }
            else{
                _load();
                console.log("Loaded\n");
            }
        });
    );

    SDL_Init(SDL_INIT_EVERYTHING);
    IMG_Init(IMG_INIT_PNG);
    win=SDL_CreateWindow("hah",0,0,1000,800,SDL_WINDOW_SHOWN);
    rend=SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED);
    v=Vid("frames",rend);
    currloop=m;
    (*m).player->rect.x=200;

    emscripten_set_main_loop(loop,0,1);
    return 0;
}