#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <emscripten.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <tuple>
#include <typeinfo>
#include <initializer_list>

SDL_Renderer* rend;
SDL_Window* win;
TTF_Font* arial;
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
    int GetCursor(){
        return curr;
    }
};

Vid v;

class Sprite{
    public:
    bool alive=true;
    SDL_Rect rect;
    Sprite(SDL_Rect r) : rect(r){}
    Sprite(){};
    virtual void update(){};
    virtual void evupdate(SDL_Event &e){};
};

class Scene{
    public:
    inline static Scene* lscene=nullptr;
    inline static std::vector<Scene*> scenes;
    inline static int start=0;
    inline static int finish=0;
    bool active=false;
    Sprite* player;
    std::vector<Sprite*> sprites;
    virtual void update(){};
    virtual void On(std::initializer_list<void*> args){};
    virtual void Off(){};
    virtual void reset(){};
};

Scene* currloop;

void switch_to(void* s,std::initializer_list<void*> args){
    currloop->Off();
    Scene::lscene=currloop;
    currloop=(Scene*)s;
    currloop->On(args);
}

void move(SDL_Rect* rect, int targetX, int targetY, float speed, float delta) {
    float dx = targetX - rect->x;
    float dy = targetY - rect->y;
    float dist = SDL_sqrtf(dx*dx + dy*dy);

    if (dist < 1.0f) return; 

    dx /= dist;
    dy /= dist;

    rect->x += dx * speed * delta;
    rect->y += dy * speed * delta;
}

class Weapon{
    public:
    int inmag;
    int speed;
    float current_cooldown;
    float cooldown;
    size_t mag_size;
    int ammos;
    float reltime;
    inline static std::vector<std::tuple<SDL_Rect,std::tuple<int,int>,int>> bullets;
    static void update_all(std::vector<Sprite*>& walls){
        std::vector<std::tuple<SDL_Rect,std::tuple<int,int>,int>> real;
    for (auto& i:bullets){
        SDL_Rect r=std::get<0>(i);
        std::tuple<int,int> coords=std::get<1>(i);
        int speed=std::get<2>(i);
        SDL_Rect coordsrect{std::get<0>(coords)-5,std::get<1>(coords)-5,15,15};
        bool push=true;
        for (auto j:walls)
        if (SDL_HasIntersection(&j->rect,&r))
        push=false;
        if (SDL_HasIntersection(&r,&coordsrect))
        push=false;
        if (push)
        real.push_back(i);
    }
    bullets=real;
    for (int i=0;i<bullets.size();i++){
        move(&std::get<0>(bullets[i]),std::get<0>(std::get<1>(bullets[i])),std::get<1>(std::get<1>(bullets[i])),std::get<2>(bullets[i]),dt);
        SDL_SetRenderDrawColor(rend,255,0,0,255);
        SDL_RenderFillRect(rend,&std::get<0>(bullets[i]));
    }
    }
    virtual void reload(){
        current_cooldown=reltime;
        std::cout<<"COOLDOWN\n";
        inmag=mag_size;
    };
    virtual void shoot(SDL_Rect who,int x,int y){
        SDL_Rect shrect{who.x+who.w/2,who.y+who.h/2,10,10};
        if (ammos<=0 || current_cooldown>0)
        return;
        if (inmag==0){
            reload();
            return;
        }
        ammos-=1;
        inmag-=1;
        std::cout<<ammos<<std::endl;
        auto i=std::make_tuple(shrect,std::make_tuple(x,y),speed);
        bullets.push_back(i);
        current_cooldown+=cooldown;
    };
    void update(float dt){
        if (current_cooldown>0)
            current_cooldown-=dt;
        if (current_cooldown<0)
            current_cooldown=0;
    }
};

void loop(){
    currloop->update();
}

struct PlayerSave{
    SDL_Rect rect;
    std::vector<Weapon*> *weapons;
};

class Main;
class Second;
class GameOver;
class ShopS;

Main* m;
Second* s;
GameOver* gs;
ShopS* ss;

class ShopS:public Scene{
    public:
    std::vector<Weapon*> *weps;
    ShopS(std::vector<Weapon*> *w) : weps(w){}
    void update() override{
        SDL_Event e;
        while (SDL_PollEvent(&e)){
            if (e.type==SDL_QUIT)
                emscripten_cancel_main_loop();
            if (e.type==SDL_KEYDOWN){
                if (e.key.keysym.sym==SDLK_e){
                    bool has=false;
                    Weapon* n=new Weapon;
                    n->mag_size = 10;
                    n->inmag = 30;
                    n->ammos = 999;
                    n->speed = 1500;
                    n->cooldown = 0.1f;
                    n->current_cooldown = 0;
                    n->reltime=2.f;
                    for (auto i:*weps)
                        if (i==n)
                            has=true;
                    if (!has){
                        weps->push_back(n);
                        std::cout<<"PURCHASED"<<std::endl;
                    }
                }   
            }
            if (e.type==SDL_KEYDOWN){
                if (e.key.keysym.sym==SDLK_ESCAPE){
                    std::cout<<"ESCAPE"<<std::endl;
                    switch_to(lscene,{});
                }
            }
        }
        SDL_SetRenderDrawColor(rend,0,100,0,255);
        SDL_RenderClear(rend);

        SDL_RenderPresent(rend);
    }
};

class Main : public Scene{
    public:

    std::vector<Sprite*> walls;


    class Shop:public Sprite{
        public:
        Shop(SDL_Rect r){
            rect=r;
        }
        void update() override{
            SDL_SetRenderDrawColor(rend,255,0,255,255);
            SDL_RenderFillRect(rend,&rect);
        };
    };


    void On(std::initializer_list<void*> args) override{
        if (dynamic_cast<ShopS*>(Scene::lscene)){
            std::cout<<"LEFT SHOP"<<std::endl;
            player->rect.x=0;
            player->rect.y=0;
        }
    }


    class Player : public Sprite{
        public:
        Main* m;
        std::vector<Weapon*> weapons;
        int currwep=0;
        Player(Main* s){
            rect={0,0,100,100};
            Weapon* wep=new Weapon;
            weapons.push_back(wep);
            wep->mag_size = 10;
            wep->inmag = 10;
            wep->ammos = 50;
            wep->speed = 1500;
            wep->cooldown = 0.5;
            wep->current_cooldown = 0;
            wep->reltime=2.f;
            m=s;
        }
        void update() override{
            if (!alive)
                switch_to(gs,{});
            const Uint8* kstate=SDL_GetKeyboardState(NULL);
            int x,y;
            Uint32 mstate=SDL_GetMouseState(&x,&y);
            weapons[currwep]->update(dt);
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
            if (mstate & SDL_BUTTON_LMASK)
                weapons[currwep]->shoot(rect,x,y);
            SDL_SetRenderDrawColor(rend,255,0,0,255);
            SDL_RenderFillRect(rend,&rect);
            for (auto i:m->sprites){
                if (dynamic_cast<Shop*>(i)){
                    if (SDL_HasIntersection(&rect,&i->rect)){
                        switch_to(ss,{});
                    }
                }
            }
        }
        void evupdate(SDL_Event &e) override{
            if (e.type==SDL_KEYDOWN)
                if(e.key.keysym.sym==SDLK_r){
                    weapons[currwep]->reload();
                }
            if (e.type==SDL_KEYDOWN)
                if(e.key.keysym.sym==SDLK_f){
                    currwep = (currwep + 1) % weapons.size();
                }
            
        }
    };


    class Enemy:public Sprite{
        Sprite* p;
        public:
        Enemy(Sprite* player,SDL_Rect r) : p(player){
            rect=r;
            alive=true;
        }
        void update() override{
            if (!alive)
                return;
            std::vector<std::tuple<SDL_Rect,std::tuple<int,int>,int>> real;
            for (auto& i:Weapon::bullets){
                if (SDL_HasIntersection(&std::get<0>(i),&rect)){
                    alive=false;
                    continue;
                }
                real.push_back(i);
            }
            Weapon::bullets=real;
            move(&rect,p->rect.x,p->rect.y,200,dt);
            if (SDL_HasIntersection(&rect,&p->rect))
                p->alive=false;
            SDL_SetRenderDrawColor(rend,0,255,0,255);
            SDL_RenderFillRect(rend,&rect);
        }
    };


    Main(){
        player=new Player(this);
        walls.push_back(new Sprite{{300,300,200,200}});
        sprites.push_back(new Enemy(player,{600,600,50,50}));
        sprites.push_back(new Shop{{800,200,100,100}});
    }

    void update() override{
        static int lastspawn=0;
        SDL_Event e;
        start=SDL_GetTicks();
        dt=(start-finish)/1000.f;
        finish=start;
        if (start-lastspawn>2000){
            sprites.push_back(new Enemy(player,{600,600,50,50}));
            lastspawn=start;
        }
        while (SDL_PollEvent(&e)){
            player->evupdate(e);
            if (e.type==SDL_QUIT)
                emscripten_cancel_main_loop();
            if (e.type==SDL_KEYDOWN)
                if (e.key.keysym.sym==SDLK_e){
                    switch_to(s,{});
                }
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
        for (auto& i:sprites)
            i->update();
        SDL_SetRenderDrawColor(rend,0,255,0,255);
        for (auto& i:walls)
            SDL_RenderFillRect(rend,&i->rect);
        {
            SDL_Texture* t=v.Get();
            if (!t)
            v.setCursor(1);
            SDL_RenderCopy(rend,t,nullptr,&player->rect);
        }
        Weapon::update_all(walls);
        SDL_RenderPresent(rend);
        std::vector<Sprite*> real;
        for (auto& i:sprites){
            if (!i->alive)
                continue;
            real.push_back(i);
        }
        sprites=real;
    }
};

class Second : public Scene{


    std::vector<Sprite*> walls;


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


    public:
    Second(){
        player=new Player(this);
        player->rect.x = 200; 
        player->rect.y = 200;
        walls.push_back(new Sprite{{500,600,150,300}});
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
                if (e.key.keysym.sym==SDLK_q){
                    switch_to(m,{});
                }
        };

        SDL_SetRenderDrawColor(rend, 0, 100, 255, 255);
        SDL_RenderClear(rend);

        player->update();
        for (auto& i:walls)
            i->update();
        SDL_RenderPresent(rend);
    }
};

class GameOver:public Scene{
    SDL_Texture* rendtxt;
    public:

    GameOver(){
        SDL_Surface* surf=TTF_RenderText_Solid(arial,"Game Over",SDL_Color{255,255,255,255});
        rendtxt=SDL_CreateTextureFromSurface(rend,surf);
        SDL_FreeSurface(surf);
    }
    
    void update() override{
        SDL_Event e;
        while (SDL_PollEvent(&e)){
            if (e.type==SDL_QUIT)
                emscripten_cancel_main_loop();  
            if (e.type == SDL_KEYDOWN){
                delete m;
                m = new Main;
                switch_to(m,{});
            }
        }
        SDL_SetRenderDrawColor(rend,0,0,0,255);
        SDL_RenderClear(rend);
        {
            SDL_Rect r{100,200,800,200};
            SDL_RenderCopy(rend,rendtxt,nullptr,&r);
        }
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

    EM_ASM(
        FS.mkdir("/save");
        FS.mount(IDBFS,{},"/save");
        FS.syncfs(true,function (err){
            if (err){
                console.log("Error at mountin or smth");
            }
            else{
                _load();
                console.log("Loaded\n");
            }
        });
    );

    SDL_Init(SDL_INIT_EVERYTHING);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();

    arial=TTF_OpenFont("assets/arialmt.ttf",100);
    if (!arial){
        std::cerr<<"FAILKED TO LOAD FONT ARIAL:"<<TTF_GetError()<<std::endl;
        return 1;
    }

    win=SDL_CreateWindow("hah",0,0,1000,800,SDL_WINDOW_SHOWN);
    rend=SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED);

    m=new Main;
    s=new Second;
    gs=new GameOver;
    {
        Main::Player* o=(Main::Player*)m->player;
        ss=new ShopS(&o->weapons);
    }
    v=Vid("/assets/frames",rend);
    currloop=m;
    (*m).player->rect.x=200;

    emscripten_set_main_loop(loop,0,1);
    return 0;
}