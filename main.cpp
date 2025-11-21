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
#include <cmath>

SDL_Renderer* rend;
SDL_Window* win;
TTF_Font* arial;
SDL_Texture* plrtxt;
float dt=0;

//Loades frames from folder and reads meta.txt
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
    //default one
    Vid(){};
    ~Vid(){
        std::cout<<"DESTROYED VID OBJECT"<<std::endl;
        for (auto i:txts)
            SDL_DestroyTexture(i);
    }

    //returns a next frame or nullptr if out of range
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
    //returns requested frame and nullptr if out of range
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
    //size
    int size(){
        return txts.size();
    }
    //where is the cursor
    int GetCursor(){
        return curr;
    }
    //the fps from meta.txt
    float getFPS(){
        std::ifstream file(name+"/meta.txt");
        if (!file.is_open())
            return 0.f;
        float a;
        float b;
        std::string t;
        std::getline(file,t,'/');
        a=std::stof(t);
        std::getline(file,t);
        b=std::stof(t);
        return a/b;
    }
    //sets fps
    void setFPS(float fps){
        this->fps=fps;
    }
};

//the video (plranim)
Vid v;

//basic Sprite class
class Sprite{
    public:
    bool alive=true;
    SDL_Rect rect;
    //for dummies
    Sprite(SDL_Rect r) : rect(r){}
    Sprite(){};
    //update
    virtual void update(){};
    //to handle events
    virtual void evupdate(SDL_Event &e){};
    virtual ~Sprite()=default;
};

//basic Scene class
class Scene{
    public:
    //last scene
    inline static Scene* lscene=nullptr;
    inline static std::vector<Scene*> scenes;
    //for delta time
    inline static int start=0;
    inline static int finish=0;
    //active scene or not
    bool active=false;
    Sprite* player;
    //sprites
    std::vector<Sprite*> sprites;
    virtual void update(){};
    //when scene turned on...
    virtual void On(std::initializer_list<void*> args){};
    //or off
    virtual void Off(){};
    //scene reset
    virtual void reset(){};
    virtual ~Scene()=default;
};

//current loop (Scene)
Scene* currloop;

//scene manager
void switch_to(void* s,std::initializer_list<void*> args){
    currloop->Off();
    Scene::lscene=currloop;
    currloop=(Scene*)s;
    currloop->On(args);
}

//move rect to targetX,targetY
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

//Bullet is Sprite's... son?
class Bullet: public Sprite{
    public:
    //parametres
    int speed;
    std::tuple<int,int> coords;
    int damage;
    //for collision
    std::vector<Sprite*> *walls;
    Bullet( std::vector<Sprite*> *w,SDL_Rect r,int s,std::tuple<int,int> c,int d) : speed{s},coords{c},damage{d}{rect=r;walls=w;alive=true;}
    //update
    void update() override{
        if (!alive){
            return;
        }
        //difference with coordinates
        float dx=std::get<0>(coords)-rect.x;
        float dy=std::get<1>(coords)-rect.y;
        if (sqrt(dx*dx+dy*dy)<10.f){
            alive=false;
        }
        //check for walls
        for (auto& i:*walls){
            if (SDL_HasIntersection(&rect,&i->rect))
                alive=false;
        }

        //other thingies
        move(&rect,std::get<0>(coords),std::get<1>(coords),speed,dt);
        SDL_SetRenderDrawColor(rend,255,0,0,255);
        SDL_RenderFillRect(rend,&rect);
    }
};

//class Weapon, something that shoots
class Weapon{
    public:
    //i need this one
    Scene** scene;
    int inmag;
    int speed;
    float current_cooldown;
    float cooldown;
    size_t mag_size;
    int ammos;
    float reltime;
    int damage;
    Weapon(Scene** h){
        scene=h;
        damage=1;
    }

    //for check
    bool operator==(const Weapon& o) const {
            return speed == o.speed &&
            mag_size == o.mag_size &&
            cooldown == o.cooldown &&
            reltime == o.reltime &&
            damage == o.damage;
        }

    //virtual reload method
    virtual void reload(){
        current_cooldown=reltime;
        std::cout<<"COOLDOWN\n";
        inmag=mag_size;
    };

    //virtual shoot method
    virtual void shoot(std::vector<Sprite*> *w,SDL_Rect who,int x,int y){
        SDL_Rect shrect{who.x+who.w/2,who.y+who.h/2,10,10};
        if (ammos<=0 || current_cooldown>0)
        return;
        if (inmag==0){
            reload();
            return;
        }
        ammos-=1;
        inmag-=1;

        auto i=new Bullet{w,shrect,speed,std::make_tuple(x,y),damage};
        (*scene)->sprites.push_back(i);


        int buls=0;
        current_cooldown+=cooldown;
    };
    //update delta and etc.
    void update(float dt){
        if (current_cooldown>0)
            current_cooldown-=dt;
        if (current_cooldown<0)
            current_cooldown=0;
    }
    //Polymorphism is hard stuff...
    virtual Weapon* clone() const{
        return new Weapon(*this);
    }
};

//base loop for emscripten_set_main_loop
void loop(){
    currloop->update();
}

//for data sync
struct PlayerSave{
    int hp;
    std::vector<Weapon*> weapons;
    void load_to(PlayerSave& obj){
        obj.weapons.clear();
        for (int i=0;i<weapons.size();i++)
            obj.weapons.push_back(new Weapon(*weapons[i]));
        obj.hp=hp;
    }
    void load_from(PlayerSave& obj){
        weapons.clear();
        for (int i=0;i<obj.weapons.size();i++)
            weapons.push_back(new Weapon(*(obj.weapons[i])));
        hp=obj.hp;
    }
};

//global save
PlayerSave sv;


//pre defenitions
class Main;
class Second;
class GameOver;
class ShopS;
class Third;

Main* m;
Second* s;
GameOver* gs;
ShopS* ss;
Third* t;

//the shop class
class ShopS:public Scene{
    public:
    ShopS(){}

    

    void update() override{
        Scene::start=SDL_GetTicks();
        dt=(Scene::start-Scene::finish)/1000.f;
        Scene::finish=Scene::start;
        SDL_Event e;
        while (SDL_PollEvent(&e)){
            if (e.type==SDL_QUIT)
                emscripten_cancel_main_loop();
            if (e.type==SDL_KEYDOWN){
                if (e.key.keysym.sym==SDLK_e){
                    //if tryes to purchase
                    bool has=false;
                    Weapon* n=new Weapon(&currloop);
                    n->mag_size = 10;
                    n->inmag = 30;
                    n->ammos = 999;
                    n->speed = 1500;
                    n->cooldown = 0.1f;
                    n->current_cooldown = 0;
                    n->reltime=2.f;
                    for (auto i:sv.weapons)
                        if (*i==*n)
                            has=true;
                    //if he doesn't has
                    if (!has){
                        sv.weapons.push_back(n);
                        std::cout<<"PURCHASED"<<std::endl;
                    }
                    //he already has
                    else
                        delete n;
                }   
            }
            //leaving
            if (e.type==SDL_KEYDOWN){
                if (e.key.keysym.sym==SDLK_ESCAPE){
                    std::cout<<"ESCAPE"<<std::endl;
                    switch_to(lscene,{});
                }
            }
            //just base stuff that other scenes have
            std::vector<Sprite*> real;
            for (auto& i:sprites){
                if (!i->alive)
                    continue;
                real.push_back(i);
            }
            sprites=real;
        }
        //render
        SDL_SetRenderDrawColor(rend,0,100,0,255);
        SDL_RenderClear(rend);

        SDL_RenderPresent(rend);
    }
};

//the main scene
class Main : public Scene{
    public:

    std::vector<Sprite*> walls;

    //(candy) shop
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


    class Player : public Sprite{
        public:
        float damage_cd=0.f;
        int hp=5;
        Main* m;
        std::vector<Weapon*> weapons;
        int currwep=0;
        bool moving=false;
        Player(Main* s){
            rect={0,0,100,100};
            //setting base weapon
            Weapon* wep=new Weapon(&currloop);
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
        //update
        void update() override{
            //checks
            if (hp<=0)
                alive=false;
            if (!alive)
                switch_to(gs,{});
            if (damage_cd>0)
                damage_cd-=dt;
            if (damage_cd<0)
                damage_cd=0;
            if (hp<0)
                hp=0;
            //move logic
            const Uint8* kstate=SDL_GetKeyboardState(NULL);
            int x,y;
            Uint32 mstate=SDL_GetMouseState(&x,&y);
            //weapin update;
            weapons[currwep]->update(dt);
            moving=false;
            if (kstate[SDL_SCANCODE_W]){
                moving=true;
                rect.y-=400*dt;
                for (auto& i: m->walls){
                    if (SDL_HasIntersection(&rect,&i->rect)){
                        auto crect=i->rect;
                        rect.y=crect.y+crect.h;
                    }
                }
            }
            if (kstate[SDL_SCANCODE_S]){
                moving=true;
                rect.y+=400*dt;
                for (auto& i: m->walls){
                    if (SDL_HasIntersection(&rect,&i->rect)){
                        auto crect=i->rect;
                        rect.y=crect.y-rect.h;
                    }
                }
            }
            if (kstate[SDL_SCANCODE_A]){
                moving=true;
                rect.x-=400*dt;
                for (auto& i: m->walls){
                    if (SDL_HasIntersection(&rect,&i->rect)){
                        auto crect=i->rect;
                        rect.x=crect.x+crect.w;
                    }
                }
            }
            if (kstate[SDL_SCANCODE_D]){
                moving=true;
                rect.x+=400*dt;
                for (auto& i: m->walls){
                    if (SDL_HasIntersection(&rect,&i->rect)){
                        auto crect=i->rect;
                        rect.x=crect.x-rect.w;
                    }
                }
            }
            if (mstate & SDL_BUTTON_LMASK){
                weapons[currwep]->shoot(&m->walls,rect,x,y);
                int buls=0;
                for (auto i:m->sprites){
                    if (dynamic_cast<Bullet*>(i))
                        buls++;
                }
            }
            //render
            //if damaged
            if (damage_cd>0){
                SDL_SetRenderDrawColor(rend,255,0,0,255);
                SDL_RenderFillRect(rend,&rect);
            }
            //if he walking
            else if (moving){
                SDL_Texture* tt=v.Get();
                if (!tt){
                    v.setCursor(0);
                    tt=v.Get();
                }
                SDL_RenderCopy(rend,tt,nullptr,&rect);
            }
            //if he is not walking
            else if(!moving){
                v.setCursor(0);
                SDL_RenderCopy(rend,plrtxt,nullptr,&rect);
            }
            //is colided with shop?
            for (auto i:m->sprites){
                if (dynamic_cast<Shop*>(i)){
                    if (SDL_HasIntersection(&rect,&i->rect))
                        switch_to(ss,{});
                }
            }
        }
        //reload and weapon swap stuff
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
        //free memory
        ~Player(){
            for (auto i:weapons)
                delete i;
        }
    };

    //i need this one too
    Player* plr;

    //Main::On()
    void On(std::initializer_list<void*> args) override{
        PlayerSave s;
        for (auto i:m->sprites){
            if (dynamic_cast<Shop*>(i)){
                if (SDL_HasIntersection(&plr->rect,&i->rect))
                    plr->rect.x=0;
                    plr->rect.y=0;
            }
        }
        s.load_from(sv);
        plr->weapons.clear();
        plr->hp=sv.hp;
        for (int i=0;i<s.weapons.size();i++)
            plr->weapons.push_back(new Weapon(*(s.weapons[i])));
    }

    //Main::Off()
    void Off() override{
        PlayerSave s;
        s.weapons.clear();
        s.hp=plr->hp;
        for (int i=0;i<plr->weapons.size();i++)
            s.weapons.push_back(new Weapon(*(plr->weapons[i])));
        s.load_to(sv);
    }


    //Enemy
    class Enemy:public Sprite{
        Sprite* p;
        int damage;
        Main* m;
        int speed;
        public:
        Enemy(Main* mm,Sprite* player,SDL_Rect r,int s,int d=1) : p(player),speed(s){
            rect=r;
            alive=true;
            damage=d;
            m=mm;
        }
        //updates and attacks player
        void update() override{
            if (!alive)
                return;
            for (auto& i:m->sprites){
                if (auto j=dynamic_cast<Bullet*>(i)){
                    if (SDL_HasIntersection(&i->rect,&rect)){
                        alive=false;
                        j->alive=false;
                    }
                } 
            }
            move(&rect,p->rect.x,p->rect.y,speed,dt);
            if (SDL_HasIntersection(&rect,&p->rect)){
                if (!(m->plr->damage_cd>0)){
                    m->plr->hp-=damage;
                    m->plr->damage_cd=2.f;
                }
            }
            SDL_SetRenderDrawColor(rend,0,255,0,255);
            SDL_RenderFillRect(rend,&rect);
        }
    };


    Main(){
        plr=new Player(this);
        player=plr;
        walls.push_back(new Sprite{{300,300,200,200}});
        sprites.push_back(new Enemy(this,player,{600,600,50,50},200));
        sprites.push_back(new Shop{{800,200,100,100}});
    }

    //updates and connects all
    void update() override{
        static int lastspawn=0;
        SDL_Event e;
        start=SDL_GetTicks();
        dt=(start-finish)/1000.f;
        finish=start;
        if (start-lastspawn>2000){
            sprites.push_back(new Enemy(this,player,{600,600,50,50},200));
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
        SDL_SetRenderDrawColor(rend,0,255,0,255);
        for (auto& i:walls)
            SDL_RenderFillRect(rend,&i->rect);
        std::vector<Sprite*> real;
        for (auto& i:sprites){
            if (!i->alive)
                continue;
            i->update();
            real.push_back(i);
        }
        sprites=real;

        SDL_RenderPresent(rend);

    }
    //dtor
    ~Main(){
        for (auto i:walls)
            delete i;
        for (auto i:sprites)
            delete i;
    }
};

//Third scene
class Third : public Scene{
    public:
    std::vector<Sprite*> walls;
    //Same with Main
    class Player : public Sprite{
        bool moving=false;
        public:
        float damage_cd=0.f;
        int hp=5;
        Third* m;
        std::vector<Weapon*> weapons;
        int currwep=0;
        Player(Third* s){
            alive=true;
            rect={0,0,100,100};
            Weapon* wep=new Weapon(&currloop);
            weapons.push_back(wep);
            wep->mag_size = 50;
            wep->inmag = 10;
            wep->ammos = 50;
            wep->speed = 1500;
            wep->cooldown = 0.1;
            wep->current_cooldown = 0;
            wep->reltime=2.f;
            m=s;
        }
        void update() override{
            if (hp<=0)
                alive=false;
            if (!alive)
                switch_to(gs,{});
            if (damage_cd>0)
                damage_cd-=dt;
            if (damage_cd<0)
                damage_cd=0;
            if (hp<0)
                hp=0;
            const Uint8* kstate=SDL_GetKeyboardState(NULL);
            int x,y;
            Uint32 mstate=SDL_GetMouseState(&x,&y);
            weapons[currwep]->update(dt);
            moving=false;
            if (kstate[SDL_SCANCODE_W]){
                moving=true;
                rect.y-=400*dt;
                for (auto& i: m->walls){
                    if (SDL_HasIntersection(&rect,&i->rect)){
                        auto crect=i->rect;
                        rect.y=crect.y+crect.h;
                    }
                }
            }
            if (kstate[SDL_SCANCODE_S]){
                moving=true;
                rect.y+=400*dt;
                for (auto& i: m->walls){
                    if (SDL_HasIntersection(&rect,&i->rect)){
                        auto crect=i->rect;
                        rect.y=crect.y-rect.h;
                    }
                }
            }
            if (kstate[SDL_SCANCODE_A]){
                moving=true;
                rect.x-=400*dt;
                for (auto& i: m->walls){
                    if (SDL_HasIntersection(&rect,&i->rect)){
                        auto crect=i->rect;
                        rect.x=crect.x+crect.w;
                    }
                }
            }
            if (kstate[SDL_SCANCODE_D]){
                moving=true;
                rect.x+=400*dt;
                for (auto& i: m->walls){
                    if (SDL_HasIntersection(&rect,&i->rect)){
                        auto crect=i->rect;
                        rect.x=crect.x-rect.w;
                    }
                }
            }
            if (mstate & SDL_BUTTON_LMASK)
                weapons[currwep]->shoot(&m->walls,rect,x,y);

            if (damage_cd>0){
                SDL_SetRenderDrawColor(rend,255,0,0,255);
                SDL_RenderFillRect(rend,&rect);
            }
            else if (moving){
                SDL_Texture* tt=v.Get();
                if (!tt){
                    v.setCursor(0);
                    tt=v.Get();
                }
                SDL_RenderCopy(rend,tt,nullptr,&rect);
            }
            else if(!moving){
                v.setCursor(0);
                SDL_RenderCopy(rend,plrtxt,nullptr,&rect);
            }

        }
        void evupdate(SDL_Event &e) override{
            if (e.type==SDL_KEYDOWN)
                if(e.key.keysym.sym==SDLK_r){
                    weapons[currwep]->reload();
                }
            if (e.type==SDL_KEYDOWN)
                if(e.key.keysym.sym==SDLK_f)
                    currwep = (currwep + 1) % weapons.size();
            
        }
        ~Player(){
            for (auto i:weapons)
                delete i;
        }
    };
    Player* plr;

    //load
    virtual void On(std::initializer_list<void*> list) override{
        plr->hp=sv.hp;
        std::cout<<"HP:"<<plr->hp<<std::endl;
        plr->weapons.clear();
        for (int i=0;i<sv.weapons.size();i++)
            plr->weapons.push_back(new Weapon(*(sv.weapons[i])));
    }

    //save
    virtual void Off() override{
        sv.hp=plr->hp;
        sv.weapons.clear();
        for (int i=0;i<plr->weapons.size();i++)
            sv.weapons.push_back(new Weapon(*(plr->weapons[i])));
    }

    //Enemy
    class Enemy:public Sprite{
        Sprite* p;
        int damage;
        Third* m;
        int speed;
        public:
        Enemy(Third* mm,Sprite* player,SDL_Rect r,int s,int d=1) : p(player),speed(s){
            rect=r;
            alive=true;
            damage=d;
            m=mm;
        }
        void update() override{
            if (!alive)
                return;
            for (auto& i:m->sprites){
                if (auto j=dynamic_cast<Bullet*>(i)){
                    if (SDL_HasIntersection(&i->rect,&rect)){
                        alive=false;
                        j->alive=false;
                    }
                } 
            }
            move(&rect,p->rect.x,p->rect.y,200,dt);
            if (SDL_HasIntersection(&rect,&p->rect)){
                if (!(m->plr->damage_cd>0)){
                    m->plr->hp-=damage;
                    m->plr->damage_cd=2.f;
                }
            }
            SDL_SetRenderDrawColor(rend,0,255,0,255);
            SDL_RenderFillRect(rend,&rect);
        }
    };
    //ctor
    Third(){
        plr=new Player(this);
        walls.push_back(new Sprite({400,400,200,300}));
    }
    //dtor
    ~Third(){
        delete plr;
        for (auto i:sprites)
            delete i;
    }
    int lastspawn=0;
    //nothing new
    void update() override{
        Scene::start=SDL_GetTicks();
        dt=(Scene::start-Scene::finish)/1000.f;
        Scene::finish=Scene::start;
        SDL_Event e;
        while (SDL_PollEvent(&e)){
            if (e.type==SDL_QUIT)
                emscripten_cancel_main_loop();
            if (e.type==SDL_KEYDOWN)
                if (e.key.keysym.sym==SDLK_q)
                    switch_to(s,{});
            for (auto& i:sprites)
                i->evupdate(e);
            plr->evupdate(e);
        }

        if (start>lastspawn+2000){
            sprites.push_back(new Enemy(this,plr,{1000,0,200,200},300,2));
            lastspawn=start;
        }

        SDL_SetRenderDrawColor(rend,0,0,0,255);
        SDL_RenderClear(rend);

        std::vector<Sprite*> real;
        for (auto& i:sprites){
            if (!i->alive)
                continue;
            i->update();
            real.push_back(i);
        }
        sprites=real;
        plr->update();
        for (auto i:walls){
            SDL_SetRenderDrawColor(rend,0,255,0,255);
            SDL_RenderFillRect(rend,&i->rect);
        }

        SDL_RenderPresent(rend);

    }
};

//nothing to explain at all
class Second : public Scene{


    std::vector<Sprite*> walls;


    class Player : public Sprite{
        Second* m;
        bool moving=false;
        public:
        Player(Second* s){
            rect={0,0,100,100};
            m=s;
        }
        void update() override{

            const Uint8* kstate=SDL_GetKeyboardState(NULL);
            moving=false;
            if (kstate[SDL_SCANCODE_W]){
                moving=true;
                rect.y-=400*dt;
                for (auto& i: m->walls){
                    if (SDL_HasIntersection(&rect,&i->rect)){
                        auto crect=i->rect;
                        rect.y=crect.y+crect.h;
                    }
                }
            }
            if (kstate[SDL_SCANCODE_S]){
                moving=true;
                rect.y+=400*dt;
                for (auto& i: m->walls){
                    if (SDL_HasIntersection(&rect,&i->rect)){
                        auto crect=i->rect;
                        rect.y=crect.y-rect.h;
                    }
                }
            }
            if (kstate[SDL_SCANCODE_A]){
                moving=true;
                rect.x-=400*dt;
                for (auto& i: m->walls){
                    if (SDL_HasIntersection(&rect,&i->rect)){
                        auto crect=i->rect;
                        rect.x=crect.x+crect.w;
                    }
                }
            }
            if (kstate[SDL_SCANCODE_D]){
                moving=true;
                rect.x+=400*dt;
                for (auto& i: m->walls){
                    if (SDL_HasIntersection(&rect,&i->rect)){
                        auto crect=i->rect;
                        rect.x=crect.x-rect.w;
                    }
                }
            }

            if (!moving){
                v.setCursor(0);
                SDL_RenderCopy(rend,plrtxt,nullptr,&rect);
            }
            if (moving){
                auto t=v.Get();
                if (!t){
                    v.setCursor(0);
                    t=v.Get();
                }
                SDL_RenderCopy(rend,t,nullptr,&rect);
            }
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
                if (e.key.keysym.sym==SDLK_q)
                    switch_to(m,{});
            if (e.type==SDL_KEYDOWN)
                if (e.key.keysym.sym==SDLK_e)
                    switch_to(t,{});
        };

        SDL_SetRenderDrawColor(rend, 0, 100, 255, 255);
        SDL_RenderClear(rend);

        player->update();
        for (auto& i:walls){
            SDL_SetRenderDrawColor(rend,0,255,0,255);
            SDL_RenderFillRect(rend,&i->rect);
        }
        std::vector<Sprite*> real;
        for (auto& i:sprites){
            if (!i->alive)
                continue;
            i->update();
            real.push_back(i);
        }
        sprites=real;
        SDL_RenderPresent(rend);
    }
    ~Second(){
        for (auto i:walls)
            delete i;
        for (auto i:sprites)
            delete i;
    }
};

//Game over
class GameOver:public Scene{
    SDL_Texture* rendtxt;
    public:

    //ctor
    GameOver(){
        SDL_Surface* surf=TTF_RenderText_Solid(arial,"Game Over",SDL_Color{255,255,255,255});
        rendtxt=SDL_CreateTextureFromSurface(rend,surf);
        SDL_FreeSurface(surf);
    }
    
    //just update delta and delete old scene
    void update() override{
        Scene::start=SDL_GetTicks();
        dt=(Scene::start-Scene::finish)/1000.f;
        Scene::finish=Scene::start;
        SDL_Event e;
        while (SDL_PollEvent(&e)){
            if (e.type==SDL_QUIT)
                emscripten_cancel_main_loop();  
            if (e.type == SDL_KEYDOWN){
                delete m;
                m = new Main;
                sv.hp=5;
                for (auto i:sv.weapons)
                    delete i;
                sv.weapons.clear();
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
    ~GameOver(){
        SDL_DestroyTexture(rendtxt);
    }
};

//load FS
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

//save FS
extern "C" void save(){
    std::ofstream f("/save/soo.txt");
    if (f.is_open()){
    f<<"hello";
    f.close();
    }
    else
        std::cout<<"ERROR DURING WRITING"<<std::endl;
}

//main
int main(){

    //sync system
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

    //initialization
    SDL_Init(SDL_INIT_EVERYTHING);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();

    //craete drawing stuff
    win=SDL_CreateWindow("hah",0,0,1000,800,SDL_WINDOW_SHOWN);
    rend=SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED);

    //loading assets
    arial=TTF_OpenFont("assets/arialmt.ttf",100);
    if (!arial){
        std::cerr<<"FAILED TO LOAD FONT ARIAL:"<<TTF_GetError()<<std::endl;
        return 1;
    }

    {
        SDL_Surface* ss=IMG_Load("assets/plr.png");
        if (!ss){
            std::cerr<<"COULDNT LOAD PLAYER IMAGE CAUSE:"<<IMG_GetError()<<std::endl;
            return 1;
        }
        plrtxt=SDL_CreateTextureFromSurface(rend,ss);
    }


    //initializing scenes
    m=new Main;
    s=new Second;
    gs=new GameOver;
    ss=new ShopS;
    t=new Third;

    //loading video
    Vid&& l=Vid("/assets/plranim",rend,10);
    v=l;
    std::cout<<"FPS:"<<v.getFPS()<<std::endl;
    currloop=m;
    (*m).player->rect.x=200;

    //START!!!!!
    emscripten_set_main_loop(loop,0,1);
    return 0;
}