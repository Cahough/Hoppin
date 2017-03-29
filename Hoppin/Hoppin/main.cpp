#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <math.h>
#include <SDL2_mixer/SDL_mixer.h>

using namespace std;
const int MAXWIDTH = 640;
const int MAXHEIGHT = 480;
bool endGame = false;

class AnimationFrame
{
    SDL_Texture *frame;
    int time, w, h; // time is in ms
    
public:
    int getW() {return w;}
    int getH() {return h;}
    
    AnimationFrame(SDL_Texture *newFrame, int newTime=100)
    {
        frame = newFrame;
        time = newTime;
    }
    
    AnimationFrame(SDL_Renderer *ren, const char *imagePath, int newTime=100)
    {
        SDL_Surface *bmp = SDL_LoadBMP(imagePath);
        if (bmp == NULL)
        {
            cout << "SDL_LoadBMP Error: " << SDL_GetError() << endl;
            SDL_Quit();
        }
        
        SDL_SetColorKey(bmp, SDL_TRUE, SDL_MapRGB(bmp->format,0,255,0));
        w = bmp->w;
        h = bmp->h;
        
        frame = SDL_CreateTextureFromSurface(ren, bmp);
        SDL_FreeSurface(bmp);
        if (frame == NULL)
        {
            std::cout << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
            SDL_Quit();
        }
        time = newTime;
    }
    
    void show(SDL_Renderer *ren, int x=0, int y=0)
    {
        SDL_Rect src, dest;
        dest.x = x;
        dest.y = y;
        dest.w = w;
        dest.h = h;
        src.x = 0;
        src.y = 0;
        src.w = w;
        src.h = h;
        SDL_RenderCopy(ren, frame, &src, &dest);
    }
    
    int getTime()
    {
        return time;
    }
    
    void destroy()
    {
        SDL_DestroyTexture(frame);
    }
};

class Animation
{
protected:
    vector<AnimationFrame *> frames;
    int totalTime;
    
public:
    int getW()
    {
        if (frames.size()>0) return frames[0]->getW();
        return 0;
    }
    
    int getH()
    {
        if (frames.size()>0) return frames[0]->getH();
        return 0;
    }
    
    Animation()
    {
        totalTime = 0;
    }
    
    void addFrame(AnimationFrame *c)
    {
        frames.push_back(c);
        totalTime += c->getTime();
    }
    
    virtual void show(SDL_Renderer *ren, int time /*ms*/, int x=0, int y=0)
    {
        int aTime = time % totalTime;
        int tTime = 0;
        unsigned int i = 0;
        for (i = 0; i < frames.size(); i++)
        {
            tTime += frames[i]->getTime();
            if (aTime <= tTime) break;
        }
        frames[i]->show(ren, x, y);
    }
    
    virtual void destroy()
    {
        for (unsigned int i = 0; i < frames.size(); i++)
            frames[i]->destroy();
    }
};


class Sprite : public Animation
{
public:
    float x, dx, ax, y, dy, ay;
    
    void set(float newX=0.0, float newY=0.0, float newDx=0.0, float newDy=0.0, float newAx=0.0, float newAy=0.0)
    {
        // position in pixels
        // speed in pixels per second
        // acceleration in pixels per second^2
        x = newX, y = newY;
        dx = newDx, dy = newDy;
        ax = newAx, ay = newAy;
    }
    
    Sprite(float newX=0.0, float newY=0.0, float newDx=0.0, float newDy=0.0, float newAx=0.0, float newAy=0.0) : Animation()
    {
        set(newX, newY, newDx, newDy, newAx, newAy);
    }
    
    void addFrames(SDL_Renderer *ren, const char *imagePath, int count, int timePerFrame=100)
    {
        for (int i = 1; i <= count; i++)
        {
            stringstream ss;
            ss << imagePath << i << ".bmp";
            addFrame(new AnimationFrame(ren, ss.str().c_str(), timePerFrame));
        }
    }
    
    void show (SDL_Renderer *ren, int time)
    {
        Animation::show(ren, time, (int)x, (int)y);
    }
    
    virtual void update(const float &dt)
    {
        x += dx*dt;
        y += dy*dt;
        dx += ax*dt;
        dy += ay*dt;
    }
};

class BounceSprite:public Sprite
{
public:
    virtual void update(const float &dt)
    {
        Sprite::update(dt);
        //if (y < 315) dy = -dy;
        if (y > 362) dy = 0; // Virtual "floor"
    }
};


class Game
{
protected:
    SDL_Window *win;
    SDL_Renderer *ren;
    int ticks;
    float dt;
    bool finished = false;
    
public:
    virtual void init(const char *gameName, int maxW=640, int maxH=480, int startX=100, int startY=100)
    {
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
            return;
        }
        
        win = SDL_CreateWindow(gameName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, maxW, maxH, SDL_WINDOW_SHOWN);
        if (win == NULL)
        {
            std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return;
        }
        
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (ren == NULL)
        {
            SDL_DestroyWindow(win);
            std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return;
        }
    }
    
    virtual void done()
    {
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
    }
    
    virtual void run()
    {
        int start = SDL_GetTicks();
        int oldTicks = start;
        finished = false;
        while (!finished)
        {
            SDL_Event event;
            if (SDL_PollEvent(&event))
            {
                if (event.type == SDL_WINDOWEVENT)
                {
                    if (event.window.event == SDL_WINDOWEVENT_CLOSE)
                    {
                        finished = true;
                        endGame = true;
                    }
                }
                if (event.type == SDL_KEYDOWN)
                {
                    if (event.key.keysym.sym == SDLK_ESCAPE)
                    {
                        finished = true;
                        endGame = true;
                    }
                }
                if (!finished) handleEvent(event);
            }
            ticks = SDL_GetTicks();
            dt = (float) (ticks-oldTicks)/1000.0; // s
            oldTicks = ticks;
            SDL_RenderClear(ren);
            show();
            SDL_RenderPresent(ren);
        }
        int end = SDL_GetTicks();
        cout << "FPS: " << (300.0*1000.0/float(end-start)) << endl;
    }
    virtual void show() = 0;
    virtual void handleEvent(SDL_Event &event) = 0;
};

class StartGame:public Game
{
    Animation background;
public:
    void init(const char *gameName = "Hoppin", int maxW=MAXWIDTH, int maxH=MAXHEIGHT, int startX=100, int startY=100)
    {
        Game::init(gameName);
        background.addFrame(new AnimationFrame(ren, "Img/startscreen1.bmp", 500));
        background.addFrame(new AnimationFrame(ren, "Img/startscreen2.bmp", 1000));
    }
    
    void run()
    {
        int start = SDL_GetTicks();
        int oldTicks = start;
        finished = false;
        while (!finished)
        {
            SDL_Event event;
            if (SDL_PollEvent(&event))
            {
                if (event.type == SDL_WINDOWEVENT)
                {
                    if (event.window.event == SDL_WINDOWEVENT_CLOSE)
                    {
                        finished = true;
                        endGame = true;
                    }
                }
                if (event.type == SDL_KEYDOWN)
                {
                    finished = true;
                }
                if (!finished) handleEvent(event);
            }
            ticks = SDL_GetTicks();
            dt = (float) (ticks-oldTicks)/1000.0; // s
            oldTicks = ticks;
            SDL_RenderClear(ren);
            show();
            SDL_RenderPresent(ren);
        }
        int end = SDL_GetTicks();
        cout << "FPS: " << (300.0*1000.0/float(end-start)) << endl;
    }
    
    void show()
    {
        background.show(ren, ticks);
    }
    
    void handleEvent(SDL_Event &event)
    {
    }
    
    void done()
    {
        background.destroy();
        Game::done();
    }
};

class HoppinGame:public Game
{
    Mix_Chunk *jumpSound;
    bool quitGame = false;
    Animation background;
    vector<Sprite> birds;
    vector<Sprite> spikes; //Temp: obstacles
    vector<Sprite> bricks;
    vector<Sprite> jumpBlocks;
    Sprite cloud;
    Sprite happyCloud;
    Sprite us;
    BounceSprite rabbit;
    SDL_Rect *rabRect = new SDL_Rect;
    SDL_Rect *spikeRect = new SDL_Rect;
    SDL_Rect *blockRect = new SDL_Rect;
    float FLOOR_HEIGHT = 440.0;
    int x, y;
    int dx, dy;
    bool canJump = true;
public:
    void init(const char *gameName = "Hoppin", int maxW=MAXWIDTH, int maxH=MAXHEIGHT, int startX=100, int startY=100)
    {
        Game::init(gameName);
        background.addFrame(new AnimationFrame(ren, "Img/hillbg.bmp"));
        cloud.addFrames(ren, "Img/cloud", 1);
        cloud.set(5.0, 5.0);
        happyCloud.addFrames(ren, "Img/happycloud", 1);
        happyCloud.set(350.0, 20.0);
        Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ); //probably needs to be moved to media manager
        
        jumpSound = Mix_LoadWAV( "/audio/jumpsound.wav" );
        
//        birds.addFrames(ren, "Img/bird", 4);
//        birds.set(rand()%maxW, 5.0, -20.0, 0.0, 0.0, 0.0);
        for (int i = 0; i < 1000; i++)
        {
            Sprite f;
            f.addFrames(ren, "Img/brick",1);
            f.set(i*50,    FLOOR_HEIGHT, -150.0, 0.0, 0.0, 0.0);
            bricks.push_back(f);
        }

        for (int i = 0; i < 10; i++)
        {
            Sprite b;
            b.addFrames(ren, "Img/bird", 4);
            b.set(rand()%maxW, rand()%20, -20.0, 0.0, 0.0, 0.0);
            birds.push_back(b);
        }
        
        rabbit.addFrames(ren, "Img/rabbit", 4);
        rabbit.set(10.0, FLOOR_HEIGHT - rabbit.getH(), 0.0, 0.0, 0.0, 9.80 * pow(10, 2));

        
        
        // Temp: Obstacles
        for (int i = 0; i < 10; i++)
        {
            Sprite s;
            s.addFrames(ren, "Img/spikes", 1);
            s.set(rand()%(1000*i-500) + 500, 420.0, -15.0, 0.0, 0.0, 0.0);
            spikes.push_back(s);
        }
        for (int i = 0; i < 10; i++)
        {
            Sprite b;
            b.addFrames(ren, "Img/jumpblock", 1);
            b.set(rand()%(1000*i-500) + 500, rand()%200 + 200, -150.0, 0.0, 0.0, 0.0);
            jumpBlocks.push_back(b);
        }

    }
    

    void show()
    {
        background.show(ren, ticks);
        cloud.show(ren, ticks);
        happyCloud.show(ren, ticks);
//        birds.show(ren, ticks);
//        birds.update(dt);
        for (unsigned int i = 0; i < birds.size(); i++)
        {
            birds[i].show(ren, ticks);
            birds[i].update(dt);
        }
        for (unsigned int i = 0; i < bricks.size(); i++)
        {
            bricks[i].show(ren, ticks);
            bricks[i].update(dt);
        }
        
        rabbit.show(ren, ticks);
        rabbit.update(dt);
        
        //set rect properties for collision
        rabRect->x = rabbit.x;
        rabRect->y = rabbit.y + rabbit.getH() - 20;
        rabRect->h = 5;
        rabRect->w = rabbit.getW();
        for (unsigned int i = 0; i < jumpBlocks.size(); i++)
        {
            jumpBlocks[i].show(ren, ticks);
            jumpBlocks[i].update(dt);
            
            blockRect->x=jumpBlocks[i].x;
            blockRect->y=jumpBlocks[i].y;
            blockRect->h=jumpBlocks[i].getH();
            blockRect->w=jumpBlocks[i].getW();
            if(SDL_HasIntersection(rabRect, blockRect)){
                rabbit.dy = 0;
                rabbit.y = blockRect->y - rabbit.getH();
                canJump = true;
            }
        
        for (unsigned int i = 0; i < spikes.size(); i++)
        {

            spikes[i].show(ren, ticks);
            spikes[i].update(dt);
            
            //set rect properties for collision
            spikeRect->x=spikes[i].x;
            spikeRect->y=spikes[i].y;
            spikeRect->h=spikes[i].getW();
            spikeRect->w=spikes[i].getH();
            if(SDL_HasIntersection(rabRect, spikeRect)){
                death();
            }
        }
        }


        
    }
    void death(){
        finished = true;
    }
    void handleEvent(SDL_Event &event)
    {
        if (event.type == SDL_KEYDOWN)
        {
            if (event.key.keysym.sym == SDLK_SPACE)
            {
                if (rabbit.y > FLOOR_HEIGHT-rabbit.getH()-.01 || canJump){ // Make sure rabbit can't double bounce
                    rabbit.dy = -500.0;
                    canJump = false;
                    Mix_PlayChannel( -1, jumpSound, 0 );
                }
            }
            if (event.key.keysym.sym == SDLK_q)
            {
                if (rabbit.y > FLOOR_HEIGHT-rabbit.getH()-.01 || canJump){ // Make sure rabbit can't double bounce
                    rabbit.dy = -300.0;
                    canJump = false;
                }
            }
        }
    }
    virtual bool getExitStatus(){ return quitGame;}
    
    void done()
    {
        background.destroy();
        Game::done();
    }
};


int main(int argc, char **argv)
{
    while (endGame == false)
    {
        StartGame s;
        HoppinGame g;

        s.init();
        s.run();
        s.done();
        g.init();
        g.run();
        g.done();
    }

    
    return 0;
}
