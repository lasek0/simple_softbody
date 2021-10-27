#include <SDL2/SDL.h>
#include <math.h>
#include <vector>

//TODO: collision between points
//TODO: collision between lines and points
//TODO: implement spring damping force
//TODO: implement point mass

//window

class Window {
  SDL_Window *w;
  SDL_Surface *ws;
  SDL_Event e;
  uint32_t t1;
  bool running;
public:
  Window();
  ~Window();
  void UpdateTitleFps();
  void Events();
  bool IsRunning() const;
  void UpdateWindow();
  void Clear();
  void PutPixel(int x, int y, int color);
  void DrawLine(int x1, int y1, int x2, int y2, int color);
  void DrawCircle(int x, int y, int r, int color);
};

Window::Window():w(NULL),ws(NULL),e({}),t1(0){
  SDL_Init(SDL_INIT_VIDEO);
  w = SDL_CreateWindow("test SDL2",
       SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 512, 512, 0);
  ws = SDL_GetWindowSurface(w);
  running = true;
}

Window::~Window(){
  SDL_DestroyWindow(w);
  SDL_Quit();
}

void Window::Events(){
  SDL_PollEvent(&e);
  if(e.type == SDL_QUIT) running = false;
}

bool Window::IsRunning() const{
  return running;
}

void Window::UpdateTitleFps(){
  uint32_t t2 = SDL_GetTicks();
  char buf[128];
  sprintf(buf, "frame duration: %d ms", (t2-t1));
  SDL_SetWindowTitle(w, buf);
  const unsigned int frames_per_milisecond = 10;
  if((t2 - t1) < frames_per_milisecond){
    SDL_Delay(frames_per_milisecond - (t2 - t1));
  }
  t1 = t2;
}

void Window::UpdateWindow(){
  SDL_UpdateWindowSurface(w);
}

void Window::Clear(){
  memset(ws->pixels, 0, (ws->h)*ws->pitch);
}

void Window::PutPixel(int x, int y, int color){
  if(x >= ws->w || x < 0 || y >= ws->h || y < 0)return;
  *(uint32_t*)((char*)ws->pixels + y*ws->pitch +  x*sizeof(uint32_t)) = color;
}


void Window::DrawLine(int x1, int y1, int x2, int y2, int color){
  float dx=x2-x1;
  float dy=y2-y1;
  if(fabs(dx) - fabs(dy) > 0 && fabs(dx) + fabs(dy) > 0 ){
    if(x1>x2){for(int i=x2;i<=x1;i++)PutPixel(i, y2+(i-x2)*dy/dx, color);}
    else     {for(int i=x1;i<=x2;i++)PutPixel(i, y1+(i-x1)*dy/dx, color);}
  }else{
    if(y1>y2){for(int i=y2;i<=y1;i++)PutPixel(x2+(i-y2)*dx/dy, i, color);}
    else     {for(int i=y1;i<=y2;i++)PutPixel(x1+(i-y1)*dx/dy, i, color);}
  }
}

void Window::DrawCircle(int x, int y, int r, int color){
  for(int i=-r; i<r; i++){
    for(int j=-r; j<r; j++){
      if(i*i + j*j < r*r) PutPixel(x+i, y+j, color);
    }
  }
}


//point

const float dt = 0.003;
const float friction = 0.994;
const float gravity = 9.81/2;
const float point_size = 4;

class Vector {
public:
  float x;
  float y;
};

class Point {
public:
  Vector position;
  Vector velocity;
  Vector acceleration;
  void Draw(Window& w);
  void Update();
};

void Point::Update(){

  velocity.x += acceleration.x;
  velocity.y += acceleration.y;
  position.x += velocity.x * dt;
  position.y += velocity.y * dt;

  velocity.x *= friction;//friction
  velocity.y *= friction;//friction
  acceleration.x = 0;
  acceleration.y = 0;

  // simple gravity
  if(position.y >= 512){
    position.y = 512;
    velocity.y *= -1;
  }
  acceleration.y = gravity;
}

void Point::Draw(Window& w){
  w.DrawCircle((int)position.x, (int)position.y, point_size, 0xff0000);
}

class Spring {
public:
  Point& p1;
  Point& p2;
  float l;//spring relax length
  float k;//spring constant
  float d;//damping force
public:
  Spring(Point& p1, Point& p2, float k, float d);
  void Apply();
  void Draw(Window& w);
};

Spring::Spring(Point &p1, Point &p2, float k, float d):p1(p1),p2(p2),k(k),d(d){
  //current distance as sprint relax length
  float dx = p1.position.x - p2.position.x;
  float dy = p1.position.y - p2.position.y;
  l = sqrt(dx*dx + dy*dy);
}

void Spring::Draw(Window& w){
  w.DrawLine((int)p1.position.x, (int)p1.position.y, 
             (int)p2.position.x, (int)p2.position.y, 
             0xffffff
  );
}

void Spring::Apply(){
  // calc points distance
  float dx = p1.position.x - p2.position.x;
  float dy = p1.position.y - p2.position.y;
  float cur_dist = sqrt(dx*dx + dy*dy);

  // calc angles
  float ca = dx/l;
  float sa = dy/l;

  //calc force
  float dd = l - cur_dist;
  float f = dd * k;

  //apply force
  p1.acceleration.x += f * ca;
  p1.acceleration.y += f * sa;
  p2.acceleration.x -= f * ca;
  p2.acceleration.y -= f * sa;
}

class Object {
public:
  std::vector<Point> points;
  std::vector<Spring> springs;
public:
  virtual void Apply() = 0;
  virtual void Update() = 0;
  virtual void Draw(Window& w) = 0;
};

class CubeObject : public Object {
public:
  CubeObject(int x, int y, int w, int h, float dist, float hardness);
  void Apply();
  void Update();
  void Draw(Window& w);
};

CubeObject::CubeObject(int x, int y, int w, int h, float dist, float hardness){
  // generate points
  for(int j=0; j<h; j++){
    for(int i=0; i<w; i++){
      points.push_back(Point{{x+i*dist, y+j*dist},{0,0},{0,0}});
    }
  }

  // generate connections in right left
  for(int i=0; i<w-1; i++){
    for(int j=0; j<h; j++){
      springs.push_back(Spring{points[w*j+i], points[w*j+i+1], hardness, 0.1});
    }
  }

  // generate connections in bottom top
  for(int i=0; i<w; i++){
    for(int j=0; j<h-1; j++){
      springs.push_back(Spring{points[w*j+i], points[w*j+i+w], hardness, 0.1});
    }
  }

  // generate connections to bottom right
  for(int i=0; i<w-1; i++){
    for(int j=0; j<h-1; j++){
      springs.push_back(Spring{points[w*j+i], points[w*j+i+w+1], hardness, 0.1});
    }
  }

  // generate connections to up left
  for(int i=1; i<w; i++){
    for(int j=0; j<h-1; j++){
      springs.push_back(Spring{points[w*j+i], points[w*j+i+w-1], hardness, 0.1});
    }
  }

}
void CubeObject::Apply(){
  for(auto& s : springs) s.Apply();
}
void CubeObject::Update(){
  for(auto& p : points) p.Update();
}
void CubeObject::Draw(Window& w){
  for(auto& s : springs) s.Draw(w);
  for(auto& p : points) p.Draw(w);
}


int main(){

  Window w;

  CubeObject o = CubeObject{156, 30, 5, 18, 17, 50};
 
  o.points[0].velocity.x=1200;
  //o.points[14*14-1].velocity.x=-1200;

  while(w.IsRunning()){
    w.Events();

    //logic
    o.Apply();
    o.Update();

    w.Clear();

    //draw
    o.Draw(w);

    w.UpdateTitleFps();
    w.UpdateWindow();
  }


  return 0;

}

