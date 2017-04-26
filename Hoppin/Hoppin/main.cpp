#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <math.h>
#include <thread>
#include <chrono>
#include <cstdlib>

// If Windows
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <SDL2/SDL_mixer.h>
#else
#include <SDL2_mixer/SDL_mixer.h>
#endif

using namespace std;
const int MAXWIDTH = 640;
const int MAXHEIGHT = 480;
bool endGame = false;

class TextureInfo
{
public:
    SDL_Texture *texture;
    int w, h;
};

class MediaManager
{
    map<string,TextureInfo *> images;
public:
    TextureInfo *load(SDL_Renderer *ren, string imagePath)
    {
        if (images.count(imagePath) == 0)
        {
            SDL_Surface *bmp = SDL_LoadBMP(imagePath.c_str());
            if (bmp == NULL){
                cout << "SDL_LoadBMP Error: " << SDL_GetError()  << endl;
                SDL_Quit();
            }
            else
            {
                cout << "Success reading " << imagePath  << endl;
            }
            SDL_SetColorKey(bmp,SDL_TRUE,SDL_MapRGB(bmp->format,0,255,0));
            TextureInfo *t = new TextureInfo();
            t->w = bmp->w;
            t->h = bmp->h;
            t->texture = SDL_CreateTextureFromSurface(ren, bmp);
            SDL_FreeSurface(bmp);
            if (t->texture == NULL)
            {
                cout << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << endl;
                SDL_Quit();
            }
            images[imagePath] = t;
        }
        return images[imagePath];
    }
    
    void destroy(TextureInfo *t)
    {
        map<string,TextureInfo *>::iterator it;
        for (it=images.begin(); it!=images.end(); it++)
        {
            if (it->second == t)
            {
                images.erase(it->first);
            }
        }
    }
};

class AnimationFrame
{
    MediaManager media;
    TextureInfo *frame;
    int time; // ms
public:
    int getW() { return frame->w; }
    int getH() { return frame->h; }
    
    AnimationFrame(TextureInfo *newFrame, int newTime=100)
    {
        frame = newFrame;
        time = newTime;
    }
    
    AnimationFrame(SDL_Renderer *ren, const char *imagePath, int newTime=100)
    {
        frame = media.load(ren, imagePath);
        time = newTime;
    }
    
    void show(SDL_Renderer *ren, int x=0, int y=0)
    {
        SDL_Rect src,dest;
        dest.x=x;  dest.y=y; dest.w=frame->w; dest.h=frame->h;
        src.x=0;  src.y=0; src.w=frame->w; src.h=frame->h;
        SDL_RenderCopy(ren, frame->texture, &src, &dest);
    }
    
    int getTime()
    {
        return time;
    }
    
    void destroy()
    {
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
    float x, dx, ax, y, dy, ay, w, h;
    
    void set(float newX=0.0, float newY=0.0, float newDx=0.0, float newDy=0.0, float newAx=0.0, float newAy=0.0, float newW = 0.0, float newH = 0.0)
    {
        // position in pixels
        // speed in pixels per second
        // acceleration in pixels per second^2
        x = newX, y = newY;
        dx = newDx, dy = newDy;
        ax = newAx, ay = newAy;
        w = newW, h = newH;
    }
    Sprite(float newX=0.0, float newY=0.0, float newDx=0.0, float newDy=0.0, float newAx=0.0, float newAy=0.0, float newW = 0.0, float newH = 0.0) : Animation()
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
    void show(SDL_Renderer *ren, int time)
    {
        Animation::show(ren, time, (int)x, (int)y);
    }
    /*virtual bool side_collision(Sprite object) //Trying to get individual side collison working
    {
		int left, oleft;
		int right, oright;
		int top;
		int obottom;
		left = x; right = x + w;
		top = y;   
		oleft = object.x; oright = object.x + object.w;
		obottom = object.y + object.h;
		return (!(top >= obottom || right <= oleft || left >= oright));
	}
	virtual bool bottom_collision(Sprite object)
	{
		int bottom = y + h;
		int left = x;
		int right = x + w;
		int otop = object.y;
		int oleft = object.x;
		int oright = object.x + object.w;
		if(bottom == otop) return true;
		else return false;
	}*/
    virtual void update(const float &dt)
    {
        x += dx*dt;
        y += dy*dt;
        dx += ax*dt;
        dy += ay*dt;
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
    vector<Sprite> birds, spikes, bricks, jumpBlocks;
    Sprite cloud, happyCloud, us, rabbit;
    SDL_Rect *rabRect = new SDL_Rect;
    SDL_Rect *spikeRect = new SDL_Rect;
    SDL_Rect *blockRect = new SDL_Rect;
    SDL_Rect *floorRect = new SDL_Rect;
    float FLOOR_HEIGHT = 440.0;
    int x, y;
    int dx, dy;
    bool canJump = true;
    int stage1[1000];
public:
    void init(const char *gameName = "Hoppin", int maxW=MAXWIDTH, int maxH=MAXHEIGHT, int startX=100, int startY=100)
    {
        Game::init(gameName);
        background.addFrame(new AnimationFrame(ren, "Img/hillbg.bmp"));
        cloud.addFrames(ren, "Img/cloud", 1);
        cloud.set(rand()%5+5.0, 5.0);
        happyCloud.addFrames(ren, "Img/happycloud", 1);
        happyCloud.set(rand()%50+350.0, rand()%20+20.0);
        Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ); //probably needs to be moved to media manager
        
        jumpSound = Mix_LoadWAV( "/audio/jumpsound.wav" );
        
//        birds.addFrames(ren, "Img/bird", 4);
//        birds.set(rand()%maxW, 5.0, -20.0, 0.0, 0.0, 0.0);
        for (int i=0; i < 1000; i+=2)
        {
            int ran = rand()%10;
            stage1[i]=ran;          //level "blueprint" to base obstacles/danger zones on
            stage1[i+1]=ran;        //floor blocks/ pits currently in 2 block segments
        }
        for (int i = 0; i < 1000; i++)
        {
            Sprite f;
            f.addFrames(ren, "Img/brick",1);
            if (stage1[i] != 0){
                f.set(i*50, FLOOR_HEIGHT, -15.0, 0.0, 0.0, 0.0, 50, 50);
                bricks.push_back(f);
            }
        }
        for (int i = 0; i < 10; i++)
        {
            Sprite b;
            b.addFrames(ren, "Img/bird", 4);
            b.set(rand()%maxW, rand()%20, -20.0, 0.0, 0.0, 0.0);
            birds.push_back(b);
        }
        rabbit.addFrames(ren, "Img/rabbit", 4);
        rabbit.set(10.0, FLOOR_HEIGHT - rabbit.getH(), 0.0, 0.0, 0.0, 9.80 * pow(10, 2), 34, 78);
        // Temp: Working on a "setInterval" equiv. to run a loop or function every x amount of time.
//        chrono::seconds interval( 10 ) ; // 10 seconds
//        for( int i = 0 ; i < 10 ; ++i )
//        {
//            cout << "tick!\n" << std::flush ;
//            for (int i = 0; i < 10; i++)
//            {
//                Sprite s;
//                s.addFrames(ren, "Img/spikes", 1);
//                s.set(rand()%(1000*i-500) + 500, 420.0, -15.0, 0.0, 0.0, 0.0);
//                spikes.push_back(s);
//            }
//            this_thread::sleep_for( interval ) ;
//        }
        
        // Temp: Obstacles
		int randnum1 = rand()%(640);
        int randnum2 = randnum1;
        Sprite s;
        s.addFrames(ren, "Img/spikes", 1);
		s.set(randnum1, 420.0, -15.0, 0.0, 0.0, 0.0);
		spikes.push_back(s);
        for (int i = 0; i < 1000; i++)
        {
            Sprite s;
            randnum1 = randnum2;
            if(!spikes.empty())
            {
				randnum2 = rand()%(1000*i-500) + 500;
				if(randnum2 > randnum1)
				{
					s.addFrames(ren, "Img/spikes", 1);
					s.set(randnum2, 420.0, -15.0, 0.0, 0.0, 0.0);
					spikes.push_back(s);
				}
			}
        }
        for (int i = 0; i < 10; i++)
        {
            Sprite b;
            b.addFrames(ren, "Img/jumpblock", 1);
            b.set(rand()%(1000*i-500) + 500, rand()%200 + 200, -150.0, 0.0, 0.0, 0.0, 50, 20);
            jumpBlocks.push_back(b);
        }
    }
    void show()
    {
        backgroundParallax(20);       
        cloudParallax(40, cloud);
        cloudParallax(30, happyCloud);
        //cloud.show(ren, ticks);
        //happyCloud.show(ren, ticks);
		//birds.show(ren, ticks);
		//birds.update(dt);
        for (unsigned int i = 0; i < birds.size(); i++)
        {
            birds[i].show(ren, ticks);
            birds[i].update(dt);
        }
        rabbit.show(ren, ticks);
        rabbit.update(dt);
        //set rect properties for collision
        setCollision(rabRect, rabbit);
        rabRect->y = rabbit.y + rabbit.getH() -5;
        rabRect->h = 5;                             //modified hitbox
        for (unsigned int i = 0; i < jumpBlocks.size(); i++)
        {
            jumpBlocks[i].show(ren, ticks);
            jumpBlocks[i].update(dt);
            setCollision(blockRect, jumpBlocks[i]);
            if(SDL_HasIntersection(rabRect, blockRect))
            {
                rabbit.dy = 0;
                rabbit.y = blockRect->y - rabbit.getH();
                canJump = true;
            }
        for (unsigned int i = 0; i < bricks.size(); i++)
        {
            bricks[i].show(ren, ticks);
            bricks[i].update(dt);
            setCollision(floorRect, bricks[i]);
					//if(rabbit.side_collision(bricks[i])) rabbit.dx == bricks[i].dx;
					//if(rabbit.bottom_collision(bricks[i]))
            if(SDL_HasIntersection(rabRect, floorRect))
					{
						rabbit.dy = 0;
						rabbit.y = floorRect->y - rabbit.getH();
						canJump = true;
					}
				}
        for (unsigned int i = 0; i < spikes.size(); i++)
			{
				spikes[i].show(ren, ticks);
				spikes[i].update(dt);
				setCollision(spikeRect, spikes[i]);	
				if(SDL_HasIntersection(rabRect, spikeRect)) death();
			if(rabbit.y >= 480) death();
			}
        }
    }
    void setCollision(SDL_Rect *rect, Sprite s){
        rect->x=s.x;
        rect->y=s.y;
        rect->h = s.getH();
        rect->w = s.getW();
    }
    void backgroundParallax(int rate){
        int bgroundloc = -(ticks/rate)%background.getW();
        background.show(ren, ticks,bgroundloc,0);
        background.show(ren,ticks,bgroundloc+background.getW(),0);
    }
    void cloudParallax(int rate, Sprite s){
        int cloudloc=-(ticks/rate)%640;
        //int r = rand()%50;
        //cloud.y += r;
        
        s.Animation::show(ren,ticks,cloudloc + s.x,s.y);
        s.Animation::show(ren,ticks,cloudloc+640 + s.x,s.y);
        
//        if (r<10){
//            happyCloud.Animation::show(ren,ticks,cloudloc,cloud.y + r);
//            happyCloud.Animation::show(ren,ticks,cloudloc+640,cloud.y+r);
//        }
       // if(happyCloud.x < -happyCloud.getW()) happyCloud.x=640;
       // happyCloud.Animation::show(ren,ticks,cloudloc+happyCloud.x,happyCloud.y);
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
                if (rabbit.dy == 0 || canJump) // Make sure rabbit can't double bounce
                {
                    rabbit.dy = -500.0;
                    canJump = false;
                    Mix_PlayChannel( -1, jumpSound, 0 );
                }
            }
            if (event.key.keysym.sym == SDLK_q)
            {
                if (canJump)
                {
                    rabbit.dy = -300.0;
                    canJump = false;
                }
            }
        }
    }
    virtual bool getExitStatus(){ return quitGame; }
    
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
		if (endGame == false)
		{
			StartGame s;
			s.init();
			s.run();
			s.done();
		}
		if (endGame == false)
		{
			HoppinGame g;
			g.init();
			g.run();
			g.done();
		}
	}
	return 0;
}
