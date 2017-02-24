#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <sstream>

using namespace std;
const int MAXWIDTH = 640;
const int MAXHEIGHT = 480;

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
    
    void setVelocity(float newDx=0.0, float newDy=0.0)
    {
        dx = newDx;
        dy = newDy;
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
        if (x < 0 || x > MAXWIDTH-getW()) dx = -dx;
        if (y < 0 || y > MAXHEIGHT-getH()) dy = -dy;
    }
};


class Game
{
protected:
    SDL_Window *win;
    SDL_Renderer *ren;
    int ticks;
    float dt;
    bool finished;
    
public:
    virtual void init(const char *gameName, int maxW=640, int maxH=480, int startX=100, int startY=100)
    {
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
            return;
        }
        
        win = SDL_CreateWindow(gameName, startX, startY, maxW, maxH, SDL_WINDOW_SHOWN);
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
                        finished = true;
                }
                if (event.type == SDL_KEYDOWN)
                {
                    if (event.key.keysym.sym == SDLK_ESCAPE)
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
    virtual void show() = 0;
    virtual void handleEvent(SDL_Event &event) = 0;
};

class HoppinGame:public Game
{
    Animation background;
    Sprite us;
    vector<BounceSprite> npcs;
    int x, y;
    int dx, dy;
public:
    void init(const char *gameName = "Hoppin", int maxW=MAXWIDTH, int maxH=MAXHEIGHT, int startX=100, int startY=100)
    {
        Game::init(gameName);
        background.addFrame(new AnimationFrame(ren, "Img/hello.bmp"));
        background.addFrame(new AnimationFrame(ren, "Img/hello1.bmp", 500));
        us.addFrames(ren, "Img/Big Planets/planet", 8);
        us.set(0.0, 0.0);
        for (int i = 0; i < 100; i++)
        {
            BounceSprite s;
            s.addFrames(ren, "Img/planet", 8);
            s.set(rand()%maxW, rand()%maxH, rand()%20-10, rand()%20-10, 0.0, 10.0);
            npcs.push_back(s);
        }

    }
    
    void show()
    {
        background.show(ren, ticks);
        for (unsigned int i = 0; i < npcs.size(); i++)
        {
            npcs[i].show(ren, ticks);
            npcs[i].update(dt);
        }
        us.show(ren, ticks);
        us.update(dt);
    }
    
    void handleEvent(SDL_Event &event)
    {
        if (event.type == SDL_KEYDOWN)
        {
            if (event.key.keysym.sym == SDLK_SPACE)
                us.setVelocity(10.0, -10.0);
        }
    }
    
    void done()
    {
        background.destroy();
        Game::done();
    }
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
                        finished = true;
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


int main(int argc, char **argv)
{
    StartGame s;
    HoppinGame g;
    s.init();
    s.run();
    s.done();
    g.init();
    g.run();
    g.done();
     
    return 0;
}
